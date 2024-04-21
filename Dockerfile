# Build stage
FROM debian:bullseye-slim as builder
RUN apt-get update && apt-get install -y \
    g++ \
    libprotobuf-dev \
    protobuf-compiler
COPY . /app
WORKDIR /app
RUN protoc --cpp_out=. chat.proto
RUN g++ -o chat_server neo_server.cpp chat.pb.cc -lpthread -lprotobuf

# Final stage
FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y libprotobuf23  # Make sure the version matches what was used in builder
COPY --from=builder /app/chat_server /app/chat_server
EXPOSE 8000
CMD ["/app/chat_server", "8000", "ChatServer"]
