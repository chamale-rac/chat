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
  // std::string message;
  // std::cout << "Enter message to broadcast: ";
  // std::cin.ignore(); // Clear the newline character from the input buffer
  // std::getline(std::cin, message);

  chat::Request request;
  request.set_operation(chat::Operation::SEND_MESSAGE);
  auto *msg = request.mutable_send_message();
  msg->set_content(message);

  SPM(sock, request);
}

void verifier(const std::string &s) {
  std::vector<std::string> tokens = split(s, ' ');
  std::string[] validTokens = {"send"};
  for (int i = 0; i < tokens.size(); i++) {
    if (tokens[i] != validTokens[i]) {
      // std::cout << "Invalid command" << std::endl;
      std::string result = "Invalid command";
      return result;
    }
  }

  if (tokens.size() != 3) {
    // std::cout << "Invalid number of arguments" << std::endl;
    std::string result = "Invalid number of arguments";
    return result;
  }

  if (tokens[0] == "send") {
    // std::cout << "Valid command" << std::endl;
    std::string result = "Valid command";
    return result;
  }

}
