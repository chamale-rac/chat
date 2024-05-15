# 🗨️ Chat System

Creating a Chat System for the Operating Systems Course

## Requirements

### Installing `protoc`

Follow the instructions at [gRPC protoc installation](https://grpc.io/docs/protoc-installation/).

Alternatively, you can use the following commands:

```bash
sudo apt update && sudo apt install -y protobuf-compiler libprotobuf-dev
```

## Compilation

First, generate the necessary protobuf files:

```bash
protoc --cpp_out=./utils ./utils/chat.proto
```

Then, compile the client and server:

```bash
g++ -o ./executables/client client.cpp ./utils/chat.pb.cc ./utils/message.cpp -lprotobuf
g++ -o ./executables/server server.cpp ./utils/chat.pb.cc ./utils/message.cpp -lpthread -lprotobuf
```