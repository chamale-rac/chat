// message_util.cpp
#include "utils.h"
#include <iostream> // For std::cerr

bool SPM(int sock, const google::protobuf::Message &message)
{
  std::string output;
  message.SerializeToString(&output);

  uint32_t size = htonl(output.size());
  std::vector<char> buffer(sizeof(size) + output.size());
  memcpy(buffer.data(), &size, sizeof(size));
  memcpy(buffer.data() + sizeof(size), output.data(), output.size());

  ssize_t sentBytes = send(sock, buffer.data(), buffer.size(), 0);
  if (sentBytes < 0)
  {
    perror("send failed");
    return false;
  }
  else if (sentBytes < buffer.size())
  {
    std::cerr << "Not all data was sent. Sent " << sentBytes << " of " << buffer.size() << " bytes." << std::endl;
    return false;
  }
  return true;
}

bool RPM(int sock, google::protobuf::Message &message)
{
  uint32_t size = 0;
  int headerSize = sizeof(size);

  if (recv(sock, &size, headerSize, MSG_PEEK) != headerSize)
  {
    return false; // Failed to read size
  }

  size = ntohl(size); // Convert size

  std::vector<char> buffer(headerSize + size);
  int totalBytesRead = recv(sock, buffer.data(), buffer.size(), MSG_WAITALL);
  if (totalBytesRead < buffer.size())
  {
    return false; // Failed to read full message
  }

  return message.ParseFromArray(buffer.data() + headerSize, buffer.size() - headerSize);
}
