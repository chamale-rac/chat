# chat-system
Creating a Chat System for the Operating Systems Course

## Requirements:

### Installing protoc
https://grpc.io/docs/protoc-installation/

sudo apt-get install libprotobuf-dev protobuf-compiler

## Compilation

g++ -o chat_server neo_server.cpp chat.pb.cc -lpthread -lprotobuf

g++ -o chat_client neo_client.cpp chat.pb.cc -lprotobuf

protoc --cpp_out=. chat.proto 