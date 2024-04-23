# chat-system
Creating a Chat System for the Operating Systems Course

## Requirements:

### Installing protoc
https://grpc.io/docs/protoc-installation/

sudo apt install -y protobuf-compiler

sudo apt-get install libprotobuf-dev protobuf-compiler

## Compilation

protoc --cpp_out=. chat.proto 

g++ -o chat_client ./client/console/client.cpp ./protocols/proto/chat.pb.cc ./protocols/message/message.cpp -lprotobuf

g++ -o chat_server ./server/server.cpp ./protocols/proto/chat.pb.cc ./protocols/message/message.cpp -lpthread -lprotobuf