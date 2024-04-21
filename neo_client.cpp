#include "chat.pb.h" // Include the generated protobuf header
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

std::atomic<bool> running{true};
std::atomic<bool> in_input_mode{false};
std::atomic<bool> waiting_response{false};
std::atomic<bool> streaming_mode{false};
std::mutex cout_mutex;
std::deque<std::string> message_buffer; // Buffer for messages received during input mode

// TODO: add identifier uuid to each request and response to match them

void flush_message_buffer()
{
  std::lock_guard<std::mutex> lock(cout_mutex);
  while (!message_buffer.empty())
  {
    std::cout << message_buffer.front() << std::endl;
    message_buffer.pop_front();
  }
}

// Helper function to send a protobuf message with size prepended, is practically a copy of the code in the server
void send_proto_message(int sock, const google::protobuf::Message &message)
{
  std::string output;
  message.SerializeToString(&output);

  uint32_t size = htonl(output.size()); // Convert size to network byte order

  send(sock, &size, sizeof(size), 0);          // Send the size of the ProtoBuf message first
  send(sock, output.data(), output.size(), 0); // Send the actual ProtoBuf message
}

bool read_proto_message(int sock, google::protobuf::Message &message)
{
  uint32_t size = 0;
  int length = recv(sock, &size, sizeof(size), MSG_WAITALL); // Read the size first
  if (length == sizeof(size))
  {
    size = ntohl(size); // Convert from network byte order to host byte order
    std::vector<char> buffer(size);
    int bytesRead = recv(sock, buffer.data(), size, MSG_WAITALL); // Read the message
    if (bytesRead > 0)
    {
      return message.ParseFromArray(buffer.data(), bytesRead);
    }
  }
  return false;
}

void messageListener(int sock)
{
  while (running)
  {
    chat::Response response;
    if (read_proto_message(sock, response))
    {
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::string message;
      if (response.status_code() != chat::StatusCode::OK)
      {
        message = RED "Server error: " + response.message() + RESET;
      }
      else
      {
        switch (response.operation())
        {
        case chat::Operation::INCOMING_MESSAGE:
          if (response.has_incoming_message())
          {
            const auto &msg = response.incoming_message();
            std::string type = (msg.type() == chat::MessageType::BROADCAST) ? "Broadcast" : "Direct";
            message = BLUE + type + " message from " + msg.sender() + ": " + msg.content() + RESET;
          }
          break;
        case chat::Operation::GET_USERS:
          if (response.has_user_list())
          {
            const auto &user_list = response.user_list();
            if (user_list.type() == chat::UserListType::SINGLE)
            {
              message = std::string(MAGENTA) + "User info: ";
            }
            else
            {
              message = std::string(MAGENTA) + "Users online: ";
            }
            for (const auto &user : user_list.users())
            {
              message += user.username() + " ";
            }
            message += RESET;
          }
          break;
        default:
          message = "Server response: " + response.message();
          break;
        }
      }

      if (response.operation() == chat::Operation::INCOMING_MESSAGE)
      {
        if (streaming_mode)
        {
          std::cout << message << std::endl;
        }
        else
        {
          message_buffer.push_back(message);
        }
      }
      else
      {
        std::cout << message << std::endl;

        if (waiting_response)
        {
          waiting_response = false;
        }
      }
    }
    else
    {
      if (!waiting_response)
        std::cerr << RED "Failed to receive a message or connection closed by server." RESET << std::endl;
      break;
    }
  }
}

void printMenu()
{
  std::lock_guard<std::mutex> lock(cout_mutex);
  std::cout << GREEN << "1. Broadcast Message\n";
  std::cout << "2. Send Direct Message\n";
  std::cout << "3. Change Status\n";
  std::cout << "4. List Users\n";
  std::cout << "5. Get User Info\n";
  std::cout << "6. Help\n";
  std::cout << "7. Stream Messages\n";
  std::cout << "8. Exit\n"
            << RESET;
  std::cout << "Enter choice: ";
}
void handleBroadcastMessage(int sock)
{
  std::string message;
  std::cout << "Enter message to broadcast: ";
  std::cin.ignore(); // Clear the newline character from the input buffer
  std::getline(std::cin, message);

  chat::Request request;
  request.set_operation(chat::Operation::SEND_MESSAGE);
  auto *msg = request.mutable_send_message();
  msg->set_content(message);

  send_proto_message(sock, request);
}

void handleDirectMessage(int sock)
{
  std::string recipient;
  std::cout << "Enter recipient username: ";
  std::cin >> recipient;

  std::string message;
  std::cout << "Enter message to send: ";
  std::cin.ignore(); // Clear the newline character from the input buffer
  std::getline(std::cin, message);

  chat::Request request;
  request.set_operation(chat::Operation::SEND_MESSAGE);
  auto *msg = request.mutable_send_message();
  msg->set_content(message);
  msg->set_recipient(recipient);

  send_proto_message(sock, request);
}

void handleChangeStatus(int sock)
{
  int status;
  std::cout << "Select status - 1: Online, 2: Busy, 3: Offline: ";
  std::cin >> status;

  chat::Request request;
  request.set_operation(chat::Operation::UPDATE_STATUS);
  auto *status_request = request.mutable_update_status();
  switch (status)
  {
  case 1:
    status_request->set_new_status(chat::UserStatus::ONLINE);
    break;
  case 2:
    status_request->set_new_status(chat::UserStatus::BUSY);
    break;
  case 3:
    status_request->set_new_status(chat::UserStatus::OFFLINE);
    break;
  default:
    std::cout << "Invalid status.\n";
    return;
  }

  send_proto_message(sock, request);
}

void handleListUsers(int sock)
{
  chat::Request request;
  request.set_operation(chat::Operation::GET_USERS);
  auto *user_list = request.mutable_get_users();

  send_proto_message(sock, request);
}

void handleGetUserInfo(int sock)
{
  std::string username;
  std::cout << "Enter username to get info: ";
  std::cin >> username;

  chat::Request request;
  request.set_operation(chat::Operation::GET_USERS);
  auto *user_list = request.mutable_get_users();
  user_list->set_username(username);

  send_proto_message(sock, request);
}

void displayHelp()
{
  std::cout << "Help - List of commands\n";
  std::cout << "1. Broadcast Message: Send a message to all users\n";
  std::cout << "2. Send Direct Message: Send a message to a specific user\n";
  std::cout << "3. Change Status: Change your status\n";
  std::cout << "4. List Users: List all users online\n";
  std::cout << "5. Get User Info: Get information about a specific user\n";
  std::cout << "6. Help: Display this help message\n";
  std::cout << "7. Stream Messages: Stream messages from the server\n";
  std::cout << "8. Exit: Exit the program\n";
}

void handleUnregisterUser(int sock, const std::string &username)
{
  chat::Request request;
  request.set_operation(chat::Operation::UNREGISTER_USER);
  auto *unregister_user = request.mutable_unregister_user();
  unregister_user->set_username(username);

  send_proto_message(sock, request);
}

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <server IP> <server port> <username>\n";
    return 1;
  }

  std::string server_ip = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];

  int sock = 0;
  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cerr << "Socket creation error \n";
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(server_port);

  if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0)
  {
    std::cerr << "Invalid address/ Address not supported \n";
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cerr << "Connection Failed \n";
    return -1;
  }

  // Register the user first
  chat::Request request;
  request.set_operation(chat::Operation::REGISTER_USER);
  auto *new_user = request.mutable_register_user();
  new_user->set_username(username);

  send_proto_message(sock, request);

  chat::Response response;
  if (read_proto_message(sock, response))
  {
    std::cout << "Server response: " << response.message() << std::endl;
  }
  else
  {
    std::cerr << "Connection closed." << std::endl;
    close(sock);
    return -1;
  }

  std::thread listener(messageListener, sock);
  listener.detach();

  int choice = 0;
  do
  {
    printMenu();
    in_input_mode = true; // Set input mode to true to suppress messageListener output
    waiting_response = true;
    std::cin >> choice;
    switch (choice)
    {
    case 1:
      handleBroadcastMessage(sock);
      break;
    case 2:
      handleDirectMessage(sock);
      break;
    case 3:
      handleChangeStatus(sock);
      break;
    case 4:
      handleListUsers(sock);
      break;
    case 5:
      handleGetUserInfo(sock);
      break;
    case 6:
      displayHelp();
      break;
    case 7:
      if (!streaming_mode)
      {
        flush_message_buffer();
      }
      streaming_mode = !streaming_mode;
      std::cout << "Streaming mode: " << (streaming_mode ? "ON" : "OFF") << std::endl;
      flush_message_buffer();
      waiting_response = false;
      break;
    case 8:
      // This is the exit option and special handling is required
      // 1. Stop running the listener thread
      running = false;
      if (listener.joinable())
      {
        listener.join(); // Wait for the listener thread to finish
      }
      waiting_response = false;
      // 2. Send the unregister request
      handleUnregisterUser(sock, username);
      // 3. Wait for the server to respond
      if (read_proto_message(sock, response))
      {
        std::cout << "Server response: " << response.message() << std::endl;
      }
      else
      {
        std::cerr << "Connection closed." << std::endl;
      }
      std::cout << "Exiting..." << std::endl;
      break;
    default:
      std::cout << "Invalid choice, please try again.\n";
      waiting_response = false;
      break;
    }
    in_input_mode = false; // Reset input mode after action is handled
    while (waiting_response)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    flush_message_buffer();
  } while (choice != 8);

  // If by some reason the listener thread is still running, stop it
  running = false;
  if (listener.joinable())
  {
    listener.join(); // Wait for the listener thread to finish
  }
  // Close the socket
  close(sock);

  return 0;
}
