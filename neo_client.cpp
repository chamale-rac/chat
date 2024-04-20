#include "chat.pb.h" // Include the generated protobuf header
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

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

void printMenu()
{
  std::cout << "1. Broadcast Message\n";
  std::cout << "2. Send Direct Message\n";
  std::cout << "3. Change Status\n";
  std::cout << "4. List Users\n";
  std::cout << "5. Get User Info\n";
  std::cout << "6. Help\n";
  std::cout << "7. Exit\n";
  std::cout << "Enter choice: ";
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
  new_user->set_ip_address(server_ip);

  send_proto_message(sock, request);

  chat::Response response;
  if (read_proto_message(sock, response))
  {
    std::cout << "Server response: " << response.message() << std::endl;
  }
  else
  {
    std::cerr << "Failed to receive registration confirmation." << std::endl;
    close(sock);
    return -1;
  }

  int choice = 0;
  do
  {
    printMenu();
    std::cin >> choice;
    switch (choice)
    {
    case 1:
      // Handle broadcasting a message
      break;
    case 2:
      // Handle sending a direct message
      break;
    case 3:
      // Handle changing user status
      break;
    case 4:
      // Handle listing users
      break;
    case 5:
      // Handle fetching user info
      break;
    case 6:
      // Display help
      break;
    case 7:
      // Exit the application
      break;
    default:
      std::cout << "Invalid choice, please try again.\n";
      break;
    }
  } while (choice != 7);

  // Close the socket
  close(sock);

  return 0;
}
