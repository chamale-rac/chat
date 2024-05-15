// message_util.cpp
#include "message.h"
#include <iostream> // For std::cerr
#include <vector>   // For std::vector
#include <cstring>  // For memcpy
#include <unistd.h> // For ssize_t

bool SPM(int sock, const google::protobuf::Message &message)
{
  std::string output;
  message.SerializeToString(&output);

  // Assuming a large buffer size, e.g., 64KB, adjust as necessary
  std::vector<char> buffer(BUFFER_SIZE);

  if (output.size() > BUFFER_SIZE)
  {
    std::cerr << "Message size exceeds buffer capacity" << std::endl;
    return false;
  }

  memcpy(buffer.data(), output.data(), output.size());

  ssize_t sentBytes = send(sock, buffer.data(), output.size(), 0);
  if (sentBytes < 0)
  {
    perror("send failed");
    return false;
  }
  else if (sentBytes < output.size())
  {
    std::cerr << "Not all data was sent. Sent " << sentBytes << " of " << output.size() << " bytes." << std::endl;
    return false;
  }
  return true;
}

bool RPM(int sock, google::protobuf::Message &message)
{
  std::vector<char> buffer(BUFFER_SIZE);

  ssize_t bytesRead = recv(sock, buffer.data(), buffer.size(), MSG_WAITALL);
  if (bytesRead <= 0)
  {
    return false; // Handle errors or disconnection
  }

  return message.ParseFromArray(buffer.data(), bytesRead);
}
