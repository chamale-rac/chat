# Use an official Ubuntu runtime as a parent image
FROM ubuntu:20.04

# Set the working directory in the container
WORKDIR /app

# Install necessary packages
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    g++ \
    libprotobuf-dev \
    protobuf-compiler \
    make \
    && rm -rf /var/lib/apt/lists/*

# Copy the current directory contents into the container at /app
# Assuming your .proto and .cpp files are in the same directory as your Dockerfile
COPY . /app

# Generate protobuf source files
RUN protoc --cpp_out=. chat.proto

# Compile the chat server
RUN g++ -o chat_server neo_server.cpp chat.pb.cc -lpthread -lprotobuf

# Expose the port the server listens on
EXPOSE 8000

# Define the command to run your executable
CMD ["./chat_server", "8000", "ChatServer"]
