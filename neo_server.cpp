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
void send_proto_message(int client_sock, const google::protobuf::Message &message)
{
  std::string buffer;
  message.SerializeToString(&buffer);

  uint32_t size = htonl(buffer.size()); // Ensure network byte order for the size

  send(client_sock, &size, sizeof(size), 0);
  send(client_sock, buffer.data(), buffer.size(), 0);
}

// Broadcast a protobuf message to all connected clients
void send_broadcast_proto_message(const google::protobuf::Message &message)
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
 * REGISTER_USER main function
 */
void handle_registration(const chat::Request &request, int client_sock, chat::Operation operation)
{
  auto user_request = request.register_user();
  const auto &username = user_request.username();

  std::lock_guard<std::mutex> lock(clients_mutex);

  chat::Response response;
  response.set_operation(operation);

  if (user_details.find(username) == user_details.end())
  {
    // Register user
    user_details.emplace(username, user_request.ip_address());
    user_status.emplace(username, chat::UserStatus::ONLINE); // Set status to online
    client_sessions.emplace(client_sock, username);          // Link socket to username

    response.set_message("User registered successfully.");
    response.set_status_code(chat::StatusCode::OK);
  }
  else
  {
    response.set_message("Username is already taken.");
    response.set_status_code(chat::StatusCode::BAD_REQUEST);
  }

  send_proto_message(client_sock, response);
}

/**
 * GET_USERS auxiliary function
 */
void add_user_to_response(const std::pair<std::string, std::string> &user, chat::UserListResponse &response)
{
  chat::User *user_proto = response.add_users();
  user_proto->set_username(user.first);
  user_proto->set_ip_address(user.second);
  // Get the status from the user_status map
  user_proto->set_status(user_status[user.first]);
}

/**
 * GET_USERS main function
 */
void handle_get_users(const chat::Request &request, int client_sock, chat::Operation operation)
{

  // Fetch the request for the user list
  auto user_list_request = request.get_users();
  const auto &username = user_list_request.username();

  std::lock_guard<std::mutex> lock(clients_mutex);

  chat::Response response;
  response.set_operation(operation);

  chat::UserListResponse user_list_response;

  if (username.empty())
  {
    // Return all connected users
    for (const auto &user : user_details)
    {
      add_user_to_response(user, user_list_response);
    }
    response.set_message("All users fetched successfully.");
    response.set_status_code(chat::StatusCode::OK);
  }
  else
  {
    // Return only the specified user
    auto it = user_details.find(username);
    if (it != user_details.end())
    {
      add_user_to_response(*it, user_list_response);
      response.set_message("User fetched successfully.");
      response.set_status_code(chat::StatusCode::OK);
    }
    else
    {
      response.set_message("User not found.");
      response.set_status_code(chat::StatusCode::NOT_FOUND);
    }
  }

  response.set_allocated_user_list(&user_list_response);
  send_proto_message(client_sock, user_list_response);
}

/**
 * SEND_MESSAGE auxiliary function
 */
chat::IncomingMessageResponse prepare_message_response(const chat::Request &request, int client_sock)
{
  auto message = request.send_message();
  chat::IncomingMessageResponse message_response;
  std::lock_guard<std::mutex> lock(clients_mutex); // Lock the clients mutex, for thread safety
  message_response.set_sender(client_sessions[client_sock]);
  message_response.set_content(message.content());
  return message_response;
}

/**
 * SEND_MESSAGE auxiliary function
 */
void send_broadcast_message(chat::Response &response_to_sender, chat::Response &response_to_recipient, chat::IncomingMessageResponse &message_response, int client_sock)
{
  message_response.set_type(chat::MessageType::BROADCAST);
  response_to_recipient.set_message("Broadcast message incoming.");
  response_to_recipient.set_status_code(chat::StatusCode::OK);
  response_to_recipient.set_allocated_incoming_message(&message_response);
  send_broadcast_proto_message(response_to_recipient);
  response_to_sender.set_message("Broadcast message sent successfully.");
  response_to_sender.set_status_code(chat::StatusCode::OK);
  send_proto_message(client_sock, response_to_sender);
}

/**
 * SEND_MESSAGE auxiliary function
 */
int find_recipient_socket(const std::string &recipient)
{
  int recipient_sock = -1;
  for (auto &session : client_sessions)
  {
    if (session.second == recipient)
    {
      recipient_sock = session.first;
      break;
    }
  }
  return recipient_sock;
}

/**
 * SEND_MESSAGE auxiliary function
 */
void send_direct_message(chat::Response &response_to_sender, chat::Response &response_to_recipient, chat::IncomingMessageResponse &message_response, int client_sock, int recipient_sock)
{
  message_response.set_type(chat::MessageType::DIRECT);
  response_to_recipient.set_message("Message incoming.");
  response_to_recipient.set_status_code(chat::StatusCode::OK);
  response_to_recipient.set_allocated_incoming_message(&message_response);
  send_proto_message(recipient_sock, response_to_recipient);
  response_to_sender.set_message("Message sent successfully.");
  response_to_sender.set_status_code(chat::StatusCode::OK);
  send_proto_message(client_sock, response_to_sender);
}

/**
 * SEND_MESSAGE main function
 */
void handle_send_message(const chat::Request &request, int client_sock, chat::Operation operation)
{
  chat::Response response_to_sender;
  response_to_sender.set_operation(operation);
  chat::Response response_to_recipient;
  response_to_recipient.set_operation(chat::Operation::INCOMING_MESSAGE);
  chat::IncomingMessageResponse message_response = prepare_message_response(request, client_sock);

  if (request.send_message().recipient().empty())
  {
    send_broadcast_message(response_to_sender, response_to_recipient, message_response, client_sock);
  }
  else
  {
    int recipient_sock = find_recipient_socket(request.send_message().recipient());
    if (recipient_sock != -1)
    {
      send_direct_message(response_to_sender, response_to_recipient, message_response, client_sock, recipient_sock);
    }
    else
    {
      response_to_sender.set_message("Recipient not found.");
      response_to_sender.set_status_code(chat::StatusCode::NOT_FOUND);
      send_proto_message(client_sock, response_to_sender);
    }
  }
}

/**
 * UPDATE_STATUS auxiliary function
 */
void update_user_status_and_time(int client_sock, const chat::UpdateStatusRequest &status_request)
{
  std::lock_guard<std::mutex> lock(clients_mutex);
  user_status[client_sessions[client_sock]] = status_request.new_status();
  // last_active[client_sessions[client_sock]] = std::chrono::system_clock::now(); TODO: Move this to any action retrieved on the general handling
}

/**
 * UPDATE_STATUS main function
 */
void update_status(const chat::Request &request, int client_sock)
{
  auto status_request = request.update_status();
  update_user_status_and_time(client_sock, status_request);

  chat::Response response;
  response.set_message("Status updated successfully."); // Consider replacing this with a constant or a configuration value
  response.set_status_code(chat::StatusCode::OK);
  send_proto_message(client_sock, response);
}

/**
 * UNREGISTER_USER main function
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
    response.set_status_code(chat::StatusCode::OK);

    // Send response to the client
    send_proto_message(client_sock, response);
  }
  else
  {
    // User not found or already unregistered, send error response
    chat::Response response;
    response.set_message("User not found or already unregistered.");
    response.set_status_code(chat::StatusCode::NOT_FOUND);

    send_proto_message(client_sock, response);
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
      switch (request.operation())
      {
      case chat::Operation::REGISTER_USER:

        if (!registered)
        {
          handle_registration(request, client_sock, chat::Operation::REGISTER_USER);
          username = request.register_user().username();
          registered = true;
        }
        else
        {

          chat::Response response;
          response.set_message("User already registered.");
          response.set_status_code(chat::StatusCode::BAD_REQUEST);
          send_proto_message(client_sock, response);
        }
        break;
      case chat::Operation::SEND_MESSAGE:
        if (registered)
          handle_send_message(request, client_sock, chat::Operation::SEND_MESSAGE);
        else
        {
          chat::Response response;
          response.set_message("User not registered.");
          response.set_status_code(chat::StatusCode::BAD_REQUEST);
          send_proto_message(client_sock, response);
        }
        break;
      case chat::Operation::UPDATE_STATUS:
        if (registered)
          update_status(request, client_sock);
        else
        {
          chat::Response response;
          response.set_message("User not registered.");
          response.set_status_code(chat::StatusCode::BAD_REQUEST);
          send_proto_message(client_sock, response);
        }
        break;
      case chat::Operation::GET_USERS:
        if (registered)
          handle_get_users(request, client_sock, chat::Operation::GET_USERS);
        else
        {
          chat::Response response;
          response.set_message("User not registered.");
          response.set_status_code(chat::StatusCode::BAD_REQUEST);
          send_proto_message(client_sock, response);
        }
        break;
      case chat::Operation::UNREGISTER_USER:
        if (registered && username == request.unregister_user().username())
        {
          unregister_user(client_sock);
          return; // Exit the loop after unregistering
        }
        else
        {
          chat::Response response;
          response.set_message("User not registered or username mismatch.");
          response.set_status_code(chat::StatusCode::BAD_REQUEST);
          send_proto_message(client_sock, response);
        }
        break;
      default:
        chat::Response response;
        response.set_message("Unknown request type.");
        response.set_status_code(chat::StatusCode::BAD_REQUEST);
        send_proto_message(client_sock, response);
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