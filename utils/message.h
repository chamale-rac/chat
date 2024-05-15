// message.h
#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <string>
#include <cstdint>                   // For uint32_t
#include <sys/types.h>               // For ssize_t
#include <sys/socket.h>              // For send, recv, and MSG_WAITALL
#include <netinet/in.h>              // For htonl, ntohl
#include <google/protobuf/message.h> // For Google Protobuf

constexpr size_t BUFFER_SIZE = 64 * 1024; // 64KB

bool SPM(int sock, const google::protobuf::Message &message); // SPM: Send Protobuf Message
bool RPM(int sock, google::protobuf::Message &message);       // RPM: Receive Protobuf Message

#endif // MESSAGE_H
