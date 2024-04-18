#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main()
{
  int sock = 0;
  struct sockaddr_in serv_addr;
  const char *hello = "Hello from client";
  char buffer[1024] = {0};

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cout << "\n Socket creation error \n";
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(8080);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
  {
    std::cout << "\nInvalid address/ Address not supported \n";
    return -1;
  }

  // Connect to the server
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cout << "\nConnection Failed \n";
    return -1;
  }

  // Send a message to the server
  send(sock, hello, strlen(hello), 0);
  std::cout << "Hello message sent\n";

  // Read response from the server
  int valread = read(sock, buffer, 1024);
  std::cout << "Server: " << buffer << std::endl;

  // Close the socket
  close(sock);

  return 0;
}
