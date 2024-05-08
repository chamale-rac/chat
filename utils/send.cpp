#include <iostream>
#include <string>

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

void handleBroadcastMessage(int sock, std::string message)
{
  chat::Request request;
  request.set_operation(chat::Operation::SEND_MESSAGE);
  auto *msg = request.mutable_send_message();
  msg->set_content(message);

  SPM(sock, request);
}

void handleDirectMessage(int sock, std::string message, std::string recipient)
{
  chat::Request request;
  request.set_operation(chat::Operation::SEND_MESSAGE);
  auto *msg = request.mutable_send_message();
  msg->set_content(message);
  msg->set_recipient(recipient);

  SPM(sock, request);
}

void handleChangeStatus(int sock, int status)
{
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

  SPM(sock, request);
}

void handleListUsers(int sock)
{
  chat::Request request;
  request.set_operation(chat::Operation::GET_USERS);
  auto *user_list = request.mutable_get_users();

  SPM(sock, request);
}

void handleGetUserInfo(int sock, std::string username)
{
  // std::string username;
  // std::cout << "Enter username to get info: ";
  // std::cin >> username;

  chat::Request request;
  request.set_operation(chat::Operation::GET_USERS);
  auto *user_list = request.mutable_get_users();
  user_list->set_username(username);

  SPM(sock, request);
}

void verifier(std::string ) {
  std::vector<std::string> tokens = split(s, ' ');
  vector<std::string> validTokens = [ "send" ];

  if (tokens[0] != validTokens[i]) {
    std::string result = "Invalid command";
    return result;
  }

  if (tokens.size() != 3) {
    std::string result = "Invalid number of arguments";
    return result;
  }

  // action, recipient, message
  // status, status
  // info, username
  std::string action = tokens[0];
  std::string recipient = tokens[1];
  std::string message = tokens[2];

  std::string status = tokens[1];
  std::string username = tokens[1];

  if (action == "send") { // send broadcast message
    handleBroadcastMessage(sock, message);
  } else if (action == "sendto") {
    handleDirectMessage(sock, message, recipient);
  } else if (action == "status") {
    handleChangeStatus(sock, std::stoi(status));
  } else if (action == "list") {
    handleListUsers(sock);
  } else if (action == "info") {
    handleGetUserInfo(sock, username);
  } else {
    std::string result = "Invalid command";
    return result;
  }

}
