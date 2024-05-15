// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <sys/types.h> // For ssize_t

// Boolean flag for handling UNIQUE IP
constexpr bool HANDLE_UNIQUE_IP = false;

// Integer flag for handling auto idling status in seconds
constexpr int AUTO_IDLING_SECONDS = 300;

// Indicating the static size of the buffer
constexpr size_t BUFFER_SIZE = 64 * 1024;

#endif // CONSTANTS_H
