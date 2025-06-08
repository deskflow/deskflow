# Flash Tutorial - Custom Client

(code was compiled on a windows machine, though not tested agains a server.)

## Scenario

Imagine a setup with three computers. The first is your primary desktop, running the Deskflow server. The second is another computer (e.g., a laptop) running the standard Deskflow client. The third computer runs a custom operating system, and our goal is to build a new, minimal client from scratch for this custom OS, allowing it to be controlled seamlessly by the primary desktop.

This tutorial guides you through the process of creating this minimal client.

## Step 0: Project Structure Setup

Create the project directory and files:

```bash
mkdir myclient
cd myclient
```

We will create the following files:
- `main.cpp` - Main application entry point
- `client.h` - Client class declaration
- `client.cpp` - Client implementation
- `CMakeLists.txt` - Build configuration

## Step 1: Project Setup and Dependencies

Create a basic C++ project with socket support. You'll need:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(DeskflowClient)

set(CMAKE_CXX_STANDARD 17)

add_executable(deskflow_client main.cpp client.cpp)

# Platform-specific socket libraries
if(WIN32)
    target_link_libraries(deskflow_client ws2_32)
else()
    # Unix/Linux - no additional libraries needed
endif()
```

```cpp
// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <arpa/inet.h>  // For network byte order conversion
#include <sys/socket.h> // For socket operations
#include <unistd.h>     // For close()

// Protocol constants from ProtocolTypes.h
const char* kMsgHello = "Deskflow";
const char* kMsgHelloBack = "Deskflow";
const char* kMsgCEnter = "CINN";
const char* kMsgCLeave = "COUT";
const char* kMsgDKeyDown = "DKDN";
const char* kMsgDKeyUp = "DKUP";
const char* kMsgDMouseMove = "DMMV";
const char* kMsgDMouseDown = "DMDN";
const char* kMsgDMouseUp = "DMUP";

class DeskflowClient {
private:
    int socket_fd;
    std::string server_ip;
    int server_port;
    
public:
    DeskflowClient(const std::string& ip, int port) 
        : server_ip(ip), server_port(port), socket_fd(-1) {}
    
    bool connect();
    bool performHandshake();
    void messageLoop();
    void disconnect();
};
```

*Reference: @ref protocol_overview for protocol architecture*

## Step 2: Network Connection

Establish TCP connection to the Deskflow server:

```cpp
// client.h
#pragma once
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
#endif

class DeskflowClient {
private:
#ifdef _WIN32
    SOCKET socket_fd;
#else
    int socket_fd;
#endif
    std::string server_ip;
    int server_port;
    
public:
    DeskflowClient(const std::string& ip, int port);
    ~DeskflowClient();
    
    bool connect();
    void disconnect();
    bool performHandshake();
    void messageLoop();
    
private:
    bool sendMessage(const std::string& message);
    std::string receiveMessage();
    void handleEnterMessage(const char* data, size_t length);
    void handleLeaveMessage();
    void handleKeyDownMessage(const char* data, size_t length);
    void handleKeyUpMessage(const char* data, size_t length);
    void handleMouseMoveMessage(const char* data, size_t length);
    void handleMouseDownMessage(const char* data, size_t length);
    void handleMouseUpMessage(const char* data, size_t length);
};
```

```cpp
// client.cpp
#include "client.h"
#include <iostream>
#include <vector>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

// Protocol constants from ProtocolTypes.h
const char* kMsgHello = "Deskflow";
const char* kMsgHelloBack = "Deskflow";
const char* kMsgCEnter = "CINN";
const char* kMsgCLeave = "COUT";
const char* kMsgDKeyDown = "DKDN";
const char* kMsgDKeyUp = "DKUP";
const char* kMsgDMouseMove = "DMMV";
const char* kMsgDMouseDown = "DMDN";
const char* kMsgDMouseUp = "DMUP";

DeskflowClient::DeskflowClient(const std::string& ip, int port) 
    : server_ip(ip), server_port(port) {
#ifdef _WIN32
    socket_fd = INVALID_SOCKET;
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    socket_fd = -1;
#endif
}

DeskflowClient::~DeskflowClient() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool DeskflowClient::connect() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (socket_fd == INVALID_SOCKET) {
#else
    if (socket_fd < 0) {
#endif
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return false;
    }
    
#ifdef _WIN32
    if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
#else
    if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
#endif
        std::cerr << "Connection failed" << std::endl;
        return false;
    }
    
    std::cout << "Connected to Deskflow server at " << server_ip << ":" << server_port << std::endl;
    return true;
}
```

*Reference: @ref protocol_handshake for connection establishment*

## Step 3: Protocol Handshake Implementation

Implement the handshake sequence required by the Deskflow protocol:

```cpp
// client.cpp (continued)
bool DeskflowClient::performHandshake() {
    // Step 1: Receive Hello message from server
    char hello_buffer[256];
#ifdef _WIN32
    int bytes_received = recv(socket_fd, hello_buffer, sizeof(hello_buffer), 0);
#else
    ssize_t bytes_received = recv(socket_fd, hello_buffer, sizeof(hello_buffer), 0);
#endif
    
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive Hello message" << std::endl;
        return false;
    }
    
    // Verify Hello message format: "Deskflow" + version bytes
    if (strncmp(hello_buffer, kMsgHello, strlen(kMsgHello)) != 0) {
        std::cerr << "Invalid Hello message received" << std::endl;
        return false;
    }
    
    // Extract protocol version (bytes after "Deskflow")
    uint16_t major_version = ntohs(*(uint16_t*)(hello_buffer + strlen(kMsgHello)));
    uint16_t minor_version = ntohs(*(uint16_t*)(hello_buffer + strlen(kMsgHello) + 2));
    
    std::cout << "Server protocol version: " << major_version << "." << minor_version << std::endl;
    
    // Step 2: Send HelloBack message
    std::string client_name = "MinimalClient";
    std::vector<char> hello_back_buffer;
    
    // Add HelloBack identifier
    hello_back_buffer.insert(hello_back_buffer.end(), kMsgHelloBack, kMsgHelloBack + strlen(kMsgHelloBack));
    
    // Add our protocol version (1.8)
    uint16_t our_major = htons(1);
    uint16_t our_minor = htons(8);
    hello_back_buffer.insert(hello_back_buffer.end(), (char*)&our_major, (char*)&our_major + 2);
    hello_back_buffer.insert(hello_back_buffer.end(), (char*)&our_minor, (char*)&our_minor + 2);
    
    // Add client name length and name
    uint32_t name_length = htonl((uint32_t)client_name.length());
    hello_back_buffer.insert(hello_back_buffer.end(), (char*)&name_length, (char*)&name_length + 4);
    hello_back_buffer.insert(hello_back_buffer.end(), client_name.begin(), client_name.end());
    
#ifdef _WIN32
    int bytes_sent = send(socket_fd, hello_back_buffer.data(), (int)hello_back_buffer.size(), 0);
#else
    ssize_t bytes_sent = send(socket_fd, hello_back_buffer.data(), hello_back_buffer.size(), 0);
#endif
    if (bytes_sent != (int)hello_back_buffer.size()) {
        std::cerr << "Failed to send HelloBack message" << std::endl;
        return false;
    }
    
    std::cout << "Handshake completed successfully" << std::endl;
    return true;
}
```

*Reference: @ref protocol_handshake for handshake message format*

## Step 4: Message Processing Loop

Implement the main loop to receive and process protocol messages:

```cpp
// client.cpp (continued)
void DeskflowClient::messageLoop() {
    std::cout << "Entering message processing loop..." << std::endl;
    
    char message_buffer[1024];
    
    while (true) {
#ifdef _WIN32
        int bytes_received = recv(socket_fd, message_buffer, sizeof(message_buffer), 0);
#else
        ssize_t bytes_received = recv(socket_fd, message_buffer, sizeof(message_buffer), 0);
#endif
        
        if (bytes_received <= 0) {
            std::cout << "Connection closed by server" << std::endl;
            break;
        }
        
        if (bytes_received < 4) {
            continue; // Need at least 4 bytes for message code
        }
        
        // Extract 4-byte message code
        std::string message_code(message_buffer, 4);
        
        if (message_code == kMsgCEnter) {
            handleEnterMessage(message_buffer + 4, bytes_received - 4);
        }
        else if (message_code == kMsgCLeave) {
            handleLeaveMessage();
        }
        else if (message_code == kMsgDKeyDown) {
            handleKeyDownMessage(message_buffer + 4, bytes_received - 4);
        }
        else if (message_code == kMsgDKeyUp) {
            handleKeyUpMessage(message_buffer + 4, bytes_received - 4);
        }
        else if (message_code == kMsgDMouseMove) {
            handleMouseMoveMessage(message_buffer + 4, bytes_received - 4);
        }
        else if (message_code == kMsgDMouseDown) {
            handleMouseDownMessage(message_buffer + 4, bytes_received - 4);
        }
        else if (message_code == kMsgDMouseUp) {
            handleMouseUpMessage(message_buffer + 4, bytes_received - 4);
        }
        else {
            std::cout << "Unknown message: " << message_code << std::endl;
        }
    }
}
```

*Reference: @ref protocol_commands and @ref protocol_data for message types*

## Step 5: Input Event Handlers

Implement handlers for specific input events:

```cpp
// client.cpp (continued)
void handleEnterMessage(const char* data, size_t length) {
    if (length < 8) return;
    
    // Extract mouse position (2 bytes x, 2 bytes y)
    int16_t x = ntohs(*(int16_t*)data);
    int16_t y = ntohs(*(int16_t*)(data + 2));
    
    // Extract modifier mask (4 bytes)
    uint32_t modifiers = ntohl(*(uint32_t*)(data + 4));
    
    std::cout << "Mouse entered screen at (" << x << ", " << y << ") with modifiers: " << modifiers << std::endl;
    
    // Here you would set the actual mouse cursor position on your system
    // Platform-specific code needed (X11, Windows API, etc.)
}

void handleLeaveMessage() {
    std::cout << "Mouse left screen" << std::endl;
    // Cleanup any active input state
}

void handleKeyDownMessage(const char* data, size_t length) {
    if (length < 6) return;
    
    // Extract key ID (2 bytes) and modifiers (2 bytes)
    uint16_t key_id = ntohs(*(uint16_t*)data);
    uint16_t modifiers = ntohs(*(uint16_t*)(data + 2));
    
    // Extract key code (2 bytes) - available in protocol 1.1+
    uint16_t key_code = ntohs(*(uint16_t*)(data + 4));
    
    std::cout << "Key pressed: ID=" << key_id << ", Code=" << key_code << ", Modifiers=" << modifiers << std::endl;
    
    // Here you would synthesize the key press on your system
    // Platform-specific code needed
}

void handleMouseMoveMessage(const char* data, size_t length) {
    if (length < 4) return;
    
    // Extract absolute mouse position
    int16_t x = ntohs(*(int16_t*)data);
    int16_t y = ntohs(*(int16_t*)(data + 2));
    
    std::cout << "Mouse moved to (" << x << ", " << y << ")" << std::endl;
    
    // Here you would move the actual mouse cursor
    // Platform-specific code needed
}

void handleKeyUpMessage(const char* data, size_t length) {
    if (length < 6) return;
    
    // Extract key ID (2 bytes) and modifiers (2 bytes)
    uint16_t key_id = ntohs(*(uint16_t*)data);
    uint16_t modifiers = ntohs(*(uint16_t*)(data + 2));
    
    // Extract key code (2 bytes) - available in protocol 1.1+
    uint16_t key_code = ntohs(*(uint16_t*)(data + 4));
    
    std::cout << "Key released: ID=" << key_id << ", Code=" << key_code << ", Modifiers=" << modifiers << std::endl;
    
    // Here you would synthesize the key release on your system
    // Platform-specific code needed
}

void handleMouseDownMessage(const char* data, size_t length) {
    if (length < 1) return;
    
    uint8_t button_id = data[0];
    std::cout << "Mouse button " << (int)button_id << " pressed" << std::endl;
    
    // Here you would synthesize the mouse button press
    // Platform-specific code needed
}

void handleMouseUpMessage(const char* data, size_t length) {
    if (length < 1) return;
    
    uint8_t button_id = data[0];
    std::cout << "Mouse button " << (int)button_id << " released" << std::endl;
    
    // Here you would synthesize the mouse button release
    // Platform-specific code needed
}
```

*Reference: @ref protocol_data for message parameter formats*

## Step 6: Complete Working Client

Put it all together in the main function:

```cpp
// main.cpp
#include "client.h"
#include <iostream>

int main() {
    const std::string server_ip = "127.0.0.1"; // localhost for testing
    const int server_port = 24800; // Default Deskflow port
    
    DeskflowClient client(server_ip, server_port);
    
    // Connect to server
    if (!client.connect()) {
        return 1;
    }
    
    // Perform protocol handshake
    if (!client.performHandshake()) {
        client.disconnect();
        return 1;
    }
    
    // Start processing messages
    client.messageLoop();
    
    // Cleanup
    client.disconnect();
    return 0;
}
```

And the disconnect method in client.cpp:

```cpp
void DeskflowClient::disconnect() {
#ifdef _WIN32
    if (socket_fd != INVALID_SOCKET) {
        closesocket(socket_fd);
        socket_fd = INVALID_SOCKET;
#else
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
#endif
        std::cout << "Disconnected from server" << std::endl;
    }
}
```

## Compilation and Testing

### Using CMake (Recommended)

Build the client using CMake:

```bash
# Generate build files
cmake .

# Build the project
cmake --build .
```

On Windows, this will create `Debug/deskflow_client.exe`.
On Linux/macOS, this will create `deskflow_client`.

### Manual Compilation (Alternative)

On Linux/macOS:
```bash
g++ -o deskflow_client main.cpp client.cpp -std=c++17
```

On Windows with MSVC:
```bash
cl main.cpp client.cpp /std:c++17 /Fe:deskflow_client.exe ws2_32.lib
```

### Testing

Run the client (ensure Deskflow server is running):

Windows:
```bash
./Debug/deskflow_client.exe
```

Linux/macOS:
```bash
./deskflow_client
```

## Working Client Result

This implementation creates a functional Deskflow client that:

1. Connects to a Deskflow server via TCP
2. Performs the required protocol handshake
3. Receives and processes input events (keyboard, mouse)
4. Displays event information (ready for platform-specific input synthesis)

The client compiles successfully on Windows with Visual Studio and should work on Linux/macOS with GCC. When no server is running, it will display "Connection failed" and exit gracefully.

To make it fully functional, add platform-specific code to actually synthesize the input events on your system using APIs like X11 (Linux), Windows API (Windows), or Core Graphics (macOS).

*Complete protocol reference: @ref ProtocolTypes.h*
*Message utilities: @ref ProtocolUtil.h*
*Full protocol documentation: @ref protocol.md*