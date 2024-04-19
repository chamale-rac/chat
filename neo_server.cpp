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

std::mutex clients_mutex;
std::map<int, std::string> client_sessions;      // Maps client socket to username
std::map<std::string, std::string> user_details; // Maps username to IP address

// Helper function to send a protobuf message to a socket
void sendProtoMessage(int client_sock, const google::protobuf::Message &message)
{
  std::string buffer;
  message.SerializeToString(&buffer);

  uint32_t size = htonl(buffer.size()); // Ensure network byte order for the size
  send(client_sock, &size, sizeof(size), 0);
  send(client_sock, buffer.data(), buffer.size(), 0);
}

void handle_client(int client_sock)
{
  // Read the size of the incoming message
  uint32_t size;
  int length = recv(client_sock, &size, sizeof(size), 0);
  if (length == sizeof(size))
  {
    size = ntohl(size);            // Convert from network byte order to host byte order
    char *buffer = new char[size]; // Allocate buffer based on expected size

    // Read the actual message based on the previously read size
    int bytesRead = recv(client_sock, buffer, size, MSG_WAITALL);
    if (bytesRead > 0)
    {
      chat::Request request;
      google::protobuf::io::ArrayInputStream ais(buffer, bytesRead);
      google::protobuf::io::CodedInputStream coded_input(&ais);
      if (!request.ParseFromCodedStream(&coded_input))
      {
        std::cerr << "Failed to parse request." << std::endl;
        close(client_sock);
        delete[] buffer;
        return;
      }

      // Process request...
      chat::Response response;

      switch (request.type())
      {
      case chat::Request::REGISTER_USER:
      {
        auto user_request = request.register_user();
        std::lock_guard<std::mutex> lock(clients_mutex);
        if (user_details.find(user_request.username()) == user_details.end())
        {
          user_details[user_request.username()] = user_request.ip_address();
          response.set_message("User registered successfully.");
          std::cout << "User registered: " << user_request.username() << std::endl;
        }
        else
        {
          response.set_message("Username is already taken.");
          std::cout << "Username already taken: " << user_request.username() << std::endl;
        }
        response.set_status_code(200); // OK
        sendProtoMessage(client_sock, response);

        std::cout << "Received registration www request for user: " << request.register_user().username() << std::endl;

        break;
      }
      case chat::Request::GET_USERS:
      {
        chat::UserListResponse user_list;
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &user : user_details)
        {
          chat::User *user_proto = user_list.add_users();
          user_proto->set_username(user.first);
          user_proto->set_ip_address(user.second);
          user_proto->set_status(chat::UserStatus::ONLINE); // Example status
        }
        sendProtoMessage(client_sock, user_list);
        break;
      }
        // Add more cases for other request types
      }
    }
    delete[] buffer;
  }
  else
  {
    std::cerr << "Failed to receive the expected size of the message." << std::endl;
  }

  // Cleanup
  close(client_sock);
  std::cout << "Client disconnected." << std::endl;
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
