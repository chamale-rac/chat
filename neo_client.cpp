#include "chat.pb.h" // Include the generated protobuf header
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

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

  // Example: Sending a registration request
  chat::Request request;
  request.set_type(chat::Request::REGISTER_USER);
  chat::NewUserRequest *new_user = new chat::NewUserRequest();
  new_user->set_username(username);
  new_user->set_ip_address(server_ip); // Assuming the client IP is the same as the server IP for simplicity
  request.set_allocated_register_user(new_user);

  // Serialize and send the request with size prepended
  std::string output;
  request.SerializeToString(&output);

  uint32_t size = htonl(output.size());        // Convert size to network byte order
  send(sock, &size, sizeof(size), 0);          // Send the size of the ProtoBuf message first
  send(sock, output.data(), output.size(), 0); // Send the actual ProtoBuf message

  // Read and process the response
  size = 0;
  int length = recv(sock, &size, sizeof(size), MSG_WAITALL); // Read the size first
  if (length == sizeof(size))
  {
    size = ntohl(size);                    // Convert from network byte order to host byte order
    char *responseBuffer = new char[size]; // Allocate buffer according to the size

    int bytesRead = recv(sock, responseBuffer, size, MSG_WAITALL); // Read the message
    if (bytesRead > 0)
    {
      chat::Response response;
      response.ParseFromArray(responseBuffer, bytesRead);
      std::cout << "Server response: " << response.message() << std::endl;
    }
    delete[] responseBuffer;
  }
  else
  {
    std::cerr << "Failed to receive the full response size." << std::endl;
  }

  // Close the socket
  close(sock);

  return 0;
}
