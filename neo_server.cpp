#include "chat.pb.h"
#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <chrono>

std::mutex clients_mutex;
std::map<int, std::string> client_sessions;                               // Maps client socket to username
std::map<std::string, std::string> user_details;                          // Maps username to IP address
std::map<std::string, chat::UserStatus> user_status;                      // Maps username to status
std::map<std::string, std::chrono::system_clock::time_point> last_active; // User activity tracking

// Helper function to send a protobuf message to a socket
void sendProtoMessage(int client_sock, const google::protobuf::Message &message)
{
  std::string buffer;
  message.SerializeToString(&buffer);

  uint32_t size = htonl(buffer.size()); // Ensure network byte order for the size
  send(client_sock, &size, sizeof(size), 0);
  send(client_sock, buffer.data(), buffer.size(), 0);
}

// Broadcast a protobuf message to all connected clients
void broadcastProtoMessage(const google::protobuf::Message &message)
{
  std::string buffer;
  message.SerializeToString(&buffer);

  uint32_t size = htonl(buffer.size()); // Ensure network byte order for the size

  std::lock_guard<std::mutex> lock(clients_mutex);
  for (const auto &session : client_sessions)
  {
    int sock = session.first;
    send(sock, &size, sizeof(size), 0);          // Send the size of the message first
    send(sock, buffer.data(), buffer.size(), 0); // Send the actual message data
  }
}

/**
 * Handle User Registration (REGISTER_USER)
 */
void handle_registration(const chat::Request &request, int client_sock)
{
  auto user_request = request.register_user();
  std::lock_guard<std::mutex> lock(clients_mutex);
  if (user_details.find(user_request.username()) == user_details.end())
  {
    // Register user
    user_details[user_request.username()] = user_request.ip_address();
    user_status[user_request.username()] = chat::UserStatus::ONLINE; // Set status to online
    client_sessions[client_sock] = user_request.username();          // Link socket to username

    chat::Response response;
    response.set_message("User registered successfully.");
    response.set_status_code(200); // OK
    sendProtoMessage(client_sock, response);
  }
  else
  {
    chat::Response response;
    response.set_message("Username is already taken.");
    response.set_status_code(400); // Bad request
    sendProtoMessage(client_sock, response);
  }
}

/**
 * Handle Get Users (GET_USERS)
 */
void handle_get_users(const chat::Request &request, int client_sock)
{
  std::lock_guard<std::mutex> lock(clients_mutex);

  // Fetch the request for the user list
  auto user_list_request = request.get_users();
  chat::UserListResponse user_list_response;

  if (user_list_request.username().empty())
  {
    // Return all connected users
    for (const auto &user : user_details)
    {
      chat::User *user_proto = user_list_response.add_users();
      user_proto->set_username(user.first);
      user_proto->set_ip_address(user.second);
      // Get the status from the user_status map
      user_proto->set_status(user_status[user.first]);
    }
  }
  else
  {
    // Return only the specified user
    auto it = user_details.find(user_list_request.username());
    if (it != user_details.end())
    {
      chat::User *user_proto = user_list_response.add_users();
      user_proto->set_username(it->first);
      user_proto->set_ip_address(it->second);
      // Get the status from the user_status map
      user_proto->set_status(user_status[it->first]);
    }
  }

  sendProtoMessage(client_sock, user_list_response);
}

/**
 * Handle Send Message (SEND_MESSAGE)
 */
void handle_send_message(const chat::Request &request, int client_sock)
{
  auto message = request.send_message();
  chat::IncomingMessageResponse message_response;
  std::lock_guard<std::mutex> lock(clients_mutex); // Lock the clients mutex, for thread safety

  message_response.set_sender(client_sessions[client_sock]);
  message_response.set_content(message.content());

  if (message.recipient().empty())
  {
    // If no recipient is specified, broadcast the message to all connected users.
    broadcastProtoMessage(message_response);
  }
  else
  {
    // Find the recipient's socket
    int recipient_sock = -1;
    for (auto &session : client_sessions)
    {
      if (session.second == message.recipient())
      {
        recipient_sock = session.first;
        break;
      }
    }

    if (recipient_sock != -1)
    {
      sendProtoMessage(recipient_sock, message);
    }
    else
    {
      std::cerr << "Recipient not found." << std::endl;
    }
  }
}

/**
 * Handle Update Status (UPDATE_STATUS)
 */
void update_status(const chat::Request &request, int client_sock)
{
  auto status_request = request.update_status();
  std::lock_guard<std::mutex> lock(clients_mutex);
  user_status[client_sessions[client_sock]] = status_request.new_status();
  last_active[client_sessions[client_sock]] = std::chrono::system_clock::now();

  chat::Response response;
  response.set_message("Status updated successfully.");
  response.set_status_code(200); // OK
  sendProtoMessage(client_sock, response);
}

/**
 * Unregister a user
 */
void unregister_user(int client_sock)
{
  std::lock_guard<std::mutex> lock(clients_mutex);

  if (client_sessions.find(client_sock) != client_sessions.end())
  {
    std::string username = client_sessions[client_sock];

    // Erase user data from maps
    client_sessions.erase(client_sock);
    user_details.erase(username);
    user_status.erase(username);
    last_active.erase(username);

    // Prepare a response message
    chat::Response response;
    response.set_message("User unregistered successfully.");
    response.set_status_code(200); // OK

    // Send response to the client
    sendProtoMessage(client_sock, response);
  }
  else
  {
    // User not found or already unregistered, send error response
    chat::Response response;
    response.set_message("User not found or already unregistered.");
    response.set_status_code(404); // Not Found

    sendProtoMessage(client_sock, response);
  }
}

void handle_client(int client_sock)
{
  bool registered = false; // Flag to check if user is registered
  std::string username;    // Store username after registration

  try
  {
    while (true)
    { // Continuously handle requests
      uint32_t size;
      if (recv(client_sock, &size, sizeof(size), MSG_WAITALL) != sizeof(size))
      {
        throw std::runtime_error("Failed to read the size of the message.");
      }

      size = ntohl(size); // Network to host byte order
      std::vector<char> buffer(size);

      if (recv(client_sock, buffer.data(), size, MSG_WAITALL) != size)
      {
        throw std::runtime_error("Failed to read the full message.");
      }

      chat::Request request;
      google::protobuf::io::ArrayInputStream ais(buffer.data(), size);
      google::protobuf::io::CodedInputStream coded_input(&ais);
      if (!request.ParseFromCodedStream(&coded_input))
      {
        throw std::runtime_error("Failed to parse the incoming request.");
      }

      // Handling different types of requests
      switch (request.type())
      {
      case chat::Request::REGISTER_USER:
        if (!registered)
        {
          handle_registration(request, client_sock);
          username = request.register_user().username();
          registered = true;
        }
        else
        {
          send_error(client_sock, "User already registered.");
        }
        break;
      case chat::Request::SEND_MESSAGE:
        if (registered)
        {
          handle_send_message(request, client_sock);
        }
        else
        {
          send_error(client_sock, "User not registered.");
        }
        break;
      case chat::Request::UPDATE_STATUS:
        if (registered)
        {
          update_status(request, client_sock);
        }
        else
        {
          send_error(client_sock, "User not registered.");
        }
        break;
      case chat::Request::GET_USERS:
        if (registered)
        {
          handle_get_users(request, client_sock);
        }
        else
        {
          send_error(client_sock, "User not registered.");
        }
        break;
      case chat::Request::UNREGISTER_USER:
        if (registered && username == request.unregister_user().username())
        {
          unregister_user(client_sock);
          return; // Exit the loop after unregistering
        }
        else
        {
          send_error(client_sock, "User not registered or username mismatch.");
        }
        break;
      default:
        send_error(client_sock, "Unknown request type.");
        break;
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << " - Likely client disconnected. Cleaning up session..." << std::endl;
    if (registered)
    {
      unregister_user(client_sock);
    }
  }

  close(client_sock);
  std::cout << "Session ended and socket closed for client." << std::endl;
}

void send_error(int client_sock, const std::string &error_message)
{
  chat::Response response;
  response.set_message(error_message);
  response.set_status_code(400); // Example: Bad Request
  sendProtoMessage(client_sock, response);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <port> <server_name>\n";
    return 1;
  }
  int port = std::stoi(argv[1]);
  std::string server_name = argv[2];

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0)
  {
    perror("Socket creation failed");
    return 1;
  }

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("Bind failed");
    return 1;
  }

  if (listen(server_fd, 10) < 0)
  {
    perror("Listen failed");
    return 1;
  }

  std::cout << server_name << " listening on port " << port << std::endl;

  while (true)
  {
    int client_sock = accept(server_fd, NULL, NULL);
    if (client_sock < 0)
    {
      perror("Accept failed");
      continue;
    }

    std::thread client_thread(handle_client, client_sock);
    client_thread.detach();
  }

  return 0;
}
