# Protocol Overview

- For a quick start on building a client, see the **[Flash Tutorial - Custom Client](@ref flash_tutorial.md)**.

## Overview

This documentation provides comprehensive information about the **Deskflow network protocol**, enabling developers to create compatible clients and servers. The protocol facilitates keyboard and mouse sharing between multiple computers over TCP connections.

## Quick Start for Implementers

### Essential Reading Order

1. **@ref protocol_overview** - Understand the basic architecture
2. **@ref protocol_handshake** - Learn connection establishment
3. **@ref protocol_commands** - Master screen control messages
4. **@ref protocol_data** - Implement input event handling
5. **@ref protocol_errors** - Handle error conditions

### Key Implementation Files

- **@ref ProtocolTypes.h** - Complete protocol specification
- **@ref ProtocolUtil.h** - Message formatting utilities
- **@ref ClientInfo** - Screen information structure

## Protocol Architecture

```
┌─────────────────┐    TCP/IP Network    ┌─────────────────┐
│   Primary       │ ←─────────────────→  │   Secondary     │
│   (Server)      │     Port 24800       │   (Client)      │
│                 │                      │                 │
│ • Shares input  │                      │ • Receives input│
│ • Manages layout│                      │ • Reports info  │
│ • Coordinates   │                      │ • Executes cmds │
└─────────────────┘                      └─────────────────┘
```

## Message Categories

The protocol organizes messages into logical categories:

| Category | Prefix | Purpose | Examples |
|----------|--------|---------|----------|
| **Handshake** | None | Connection setup | Hello, HelloBack |
| **Commands** | `C` | Screen control | Enter, Leave, KeepAlive |
| **Data** | `D` | Input events | KeyDown, MouseMove, Clipboard |
| **Queries** | `Q` | Information requests | QueryInfo |
| **Errors** | `E` | Error notifications | Incompatible, Busy |

## Implementation Checklist

### Basic Client Implementation

- [ ] **Connection Management**
  - [ ] TCP connection to server port 24800
  - [ ] Protocol handshake (Hello/HelloBack)
  - [ ] Version negotiation
  - [ ] Keep-alive handling

- [ ] **Message Processing**
  - [ ] Message parsing and validation
  - [ ] Command message handling (Enter/Leave)
  - [ ] Input event processing (keyboard/mouse)
  - [ ] Error handling and recovery

- [ ] **Screen Management**
  - [ ] Screen information reporting (DINF)
  - [ ] Resolution change detection
  - [ ] Mouse cursor positioning

- [ ] **Input Synthesis**
  - [ ] Keyboard event injection
  - [ ] Mouse event injection
  - [ ] Modifier key synchronization

### Advanced Features

- [ ] **Clipboard Synchronization**
  - [ ] Clipboard grab notifications
  - [ ] Data transfer (text, images, HTML)
  - [ ] Streaming for large data (v1.6+)

- [ ] **File Transfer** (v1.5+)
  - [ ] Drag-and-drop initiation
  - [ ] Chunked file transfer
  - [ ] Progress tracking

- [ ] **Security Features**
  - [ ] TLS/SSL encryption (v1.4+)
  - [ ] Secure input notifications (v1.7+)
  - [ ] Input validation and limits

## Reference Implementation

The [uSynergy micro client](https://github.com/symless/synergy-micro-client) provides a minimal reference implementation in ~600 lines of C code, demonstrating:

- Basic protocol handling
- Message parsing and generation
- Connection management
- Input event processing

## Protocol Versions and Compatibility

| Version | Features | Compatibility |
|---------|----------|---------------|
| **1.0** | Basic keyboard/mouse sharing | All versions |
| **1.1** | Physical key codes | 1.1+ |
| **1.2** | Relative mouse movement | 1.2+ |
| **1.3** | Keep-alive, horizontal scroll | 1.3+ |
| **1.4** | Encryption support | 1.4+ |
| **1.5** | File transfer | 1.5+ |
| **1.6** | Clipboard streaming | 1.6+ |
| **1.7** | Secure input notifications | 1.7+ |
| **1.8** | Language synchronization | 1.8+ |

## Common Implementation Patterns

### Connection Lifecycle

```cpp
// 1. Connect to server
std::string server_ip = "192.168.1.100";
connect(server_ip, 24800);

// 2. Receive Hello from server
auto hello = receive_message();
std::string server_version, server_name;
parse_hello(hello, &server_version, &server_name);

// 3. Send HelloBack to server
std::string client_version = "1.8";
std::string client_name = "MyClient";
send_hello_back(client_version, client_name);

// 4. Enter main message loop
bool connected = true;
while (connected) {
    auto message = receive_message();
    handle_message(message);
}
```

### Message Handling

```cpp
void handle_message(const Message& msg) {
    switch (msg.type) {
        case "CINN": // Enter screen
            handle_enter(msg.x, msg.y, msg.sequence, msg.modifiers);
            break;
        case "DKDN": // Key down
            handle_key_down(msg.key_id, msg.modifiers, msg.key_button);
            break;
        case "DMMV": // Mouse move
            handle_mouse_move(msg.x, msg.y);
            break;
        // ... handle other message types
    }
}
```

## Debugging and Troubleshooting

### Common Issues

1. **Version Mismatch**: Check protocol version negotiation
2. **Message Format**: Validate message structure and parameters
3. **Byte Order**: Ensure network byte order for multi-byte integers
4. **Keep-Alive**: Implement proper keep-alive response
5. **Screen Info**: Send accurate screen dimensions and mouse position

### Debug Tools

- **Wireshark**: Capture and analyze network traffic
- **Protocol Logs**: Enable verbose logging in Deskflow
- **Message Validation**: Check message format against specification

## Contributing

When extending the protocol:

1. **Maintain Compatibility**: New features should be backward compatible
2. **Update Documentation**: Document new messages and parameters
3. **Version Increment**: Bump minor version for new features
4. **Test Thoroughly**: Verify with existing clients and servers

## Support and Resources

- [Deskflow GitHub Repository](https://github.com/deskflow/deskflow)
- [Report bugs and request features](https://github.com/deskflow/deskflow/issues)
- [Join discussions and get help](https://github.com/deskflow/deskflow/discussions)
- [Study the uSynergy micro client implementation](https://github.com/symless/synergy-micro-client)

---

*This documentation is generated from the source code and is always up-to-date with the latest protocol implementation.*