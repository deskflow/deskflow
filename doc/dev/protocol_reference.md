# Protocol Reference

This document provides a comprehensive reference for the Deskflow network protocol. It is the primary source of information for developers implementing Deskflow clients or extending the protocol.

## Protocol Overview

The Deskflow protocol enables keyboard and mouse sharing between multiple computers over a TCP network connection. The protocol uses two distinct sets of terminology to describe the roles of the computers involved:

- **Network Role (Client/Server)**: This describes the connection architecture.
  - **Server**: The machine that listens for incoming TCP connections.
  - **Client**: The machine that initiates the TCP connection to the server.

- **Input Control Role (Primary/Secondary)**: This describes the flow of keyboard and mouse events.
  - **Primary**: The computer whose keyboard and mouse are currently being used to control other computers.
  - **Secondary**: A computer that is being controlled by the Primary's keyboard and mouse.

In a typical setup, the Primary computer (the one whose keyboard and mouse are shared) also acts as the Server. However, the protocol is flexible and allows these roles to be separate. For example, a Primary machine can act as a Client to connect to a Secondary machine that is configured as a Server. This can be useful for navigating restrictive network environments like firewalls.

Throughout the documentation, message direction is often described using the Primary/Secondary roles to clarify the input control flow, while Client/Server roles are used when discussing the underlying network connection.

### Key Implementation Files

- **[ProtocolTypes.h](@ref ProtocolTypes.h)** – Complete protocol specification
- **[ProtocolUtil.h](@ref ProtocolUtil.h)** – Message formatting utilities
- **[ClientInfo](@ref ClientInfo)** – Screen information structure

The protocol is designed to be:
- Lightweight and efficient
- Cross-platform compatible
- Extensible for new features
- Backward compatible with older versions

## Protocol Architecture


```
┌─────────────────┐    TCP/IP Network    ┌─────────────────┐
│   Primary       │◄────────────────────►│   Secondary     │
│   (Server)      │     Port 24800       │   (Client)      │
│                 │                      │                 │
│ • Shares input  │                      │ • Receives input│
│ • Manages layout│                      │ • Reports info  │
│ • Coordinates   │                      │ • Executes cmds │
└─────────────────┘                      └─────────────────┘
```

The protocol operates over a standard TCP connection on port 24800. In protocol versions 1.4 and later, TLS encryption is supported for secure communications.

## Protocol State Machine

The client's connection lifecycle is defined by five primary states:


```
                         ┌──────────────────┐
                         │      START       │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌────────┴─────────┐
                         │   DISCONNECTED   │
                         │   (Initial &     │◄───────────────────┐
                         │   Final State)   │                    │
                         └────────┬─────────┘                    │
                                  │                              │
                                  ▼                              │
                         ┌──────────────────┐                    │
                         │    CONNECTING    │  TCP Failure       │
                         │  (TCP handshake) ├───────────────────►┤
                         └────────┬─────────┘                    │
                                  │                              │
                      TCP Success │                              │
                                  │                              │
                                  ▼                              │
                         ┌──────────────────┐                    │
                         │    HANDSHAKE     │  Version Mismatch  │
                         │  (Hello/HelloBk) ├───────────────────►┤
                         └────────┬─────────┘                    │
                                  │                              │
                               OK │                              │
                                  │                              │
                                  ▼                              │
                         ┌──────────────────┐                    │
                         │    CONNECTED     │  CCLOSE (close)    │
                    ┌───►│  (Authenticated  ├───────────────────►┤
                    │    │   but inactive)  │                    │
                    │    └────────┬─────────┘                    │
                    │             │                              │
             COUT   │        CINN │                              │
           (Leave)  │      (Enter)│                              │
                    │             ▼                              │
                    │    ┌──────────────────┐                    │
                    │    │      ACTIVE      │  CCLOSE (close)    │
                    └────┤  (Receiving all  ├───────────────────►┘
                         │   input events)  ├◄─────┐
                         └────────┬─────────┘      │
                                  │                │
                                  ▼                │
                         ┌──────────────────┐      │
                         │  PROCESS EVENT   ├──────┘
                         └──────────────────┘
                                  
                                  

```

### State Descriptions

1. **Disconnected**: Initial and final state. No connection to @ref Server.
2. **Connecting**: @ref TCPSocket connection attempt in progress.
   - Initiating @ref TCPSocket connection.
   - On successful TCP connection, moves to the `Handshake` state.
   - If TCP connection fails (timeout, RST packet), returns to `Disconnected`.
3. **Handshake**: Protocol version negotiation and authentication.
   - @ref Server sends @ref kMsgHello with protocol version information.
   - @ref Client responds with @ref kMsgHelloBack including version and screen name.
   - @ref Server validates the client's message.
   - Success transitions to `Connected`, failure sends @ref kMsgEIncompatible error.
4. **Connected**: Authenticated but not receiving input events.
   - @ref Client must respond to @ref kMsgCKeepAlive messages from the @ref Server.
   - Receiving @ref kMsgCEnter message transitions to `Active`.
5. **Active**: Receiving and processing input events from @ref Server.
   - Receiving @ref kMsgCLeave message transitions back to `Connected`.
   - Receiving @ref kMsgCClose message transitions to `Disconnected`.


## Message Categories

The protocol organizes messages into logical categories:

| Category | Prefix | Purpose | Examples |
|----------|---------|----------|-----------|
| **[Handshake](@ref protocol_handshake)** | None | Connection setup | @ref kMsgHello, @ref kMsgHelloBack |
| **[Commands](@ref protocol_commands)** | `C` | Screen control | @ref kMsgCEnter, @ref kMsgCLeave, @ref kMsgCKeepAlive |
| **[Data](@ref protocol_data)** | `D` | Input events | @ref kMsgDKeyDown, @ref kMsgDMouseMove, @ref kMsgCClipboard, @ref kMsgDClipboard |
| **[Queries](@ref protocol_queries)** | `Q` | Information requests | @ref kMsgQInfo |
| **[Errors](@ref protocol_errors)** | `E` | Error notifications | @ref kMsgEIncompatible, @ref kMsgEBusy |

## Message Reference Table

This table lists all protocol messages in alphabetical order. For a typical sequence of messages, see the [Typical Control Flow](#typical-control-flow) section.

| Message | Constant | Category | Direction | Purpose | Constraints | Protocol Version |
|---|---|---|---|---|---|---|
| [**CALV**](@ref kMsgCKeepAlive) | @ref kMsgCKeepAlive | Command | Both | Keep-alive | [MsgSize](#constraint-protocol-max-message-length), [KeepAlive](#constraint-keep-alive) | 1.3+ |
| [**CBYE**](@ref kMsgCClose) | @ref kMsgCClose | Command | Server→Client | Close connection | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**CCLP**](@ref kMsgCClipboard) | @ref kMsgCClipboard | Command | Both | Clipboard ownership notification | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**CIAK**](@ref kMsgCInfoAck) | @ref kMsgCInfoAck | Command | Server→Client | Acknowledge info message | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**CINN**](@ref kMsgCEnter) | @ref kMsgCEnter | Command | Server→Client | Enter screen | [MsgSize](#constraint-protocol-max-message-length), [ScreenEntrySync](#constraint-screen-entry-sync) | 1.0+ |
| [**CNOP**](@ref kMsgCNoop) | @ref kMsgCNoop | Command | Both | No operation | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**COUT**](@ref kMsgCLeave) | @ref kMsgCLeave | Command | Server→Client | Leave screen | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**CROP**](@ref kMsgCResetOptions) | @ref kMsgCResetOptions | Command | Server→Client | Reset options to defaults | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**CSEC**](@ref kMsgCScreenSaver) | @ref kMsgCScreenSaver | Command | Server→Client | Screen saver control | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DCLP**](@ref kMsgDClipboard) | @ref kMsgDClipboard | Data | Both | Clipboard data | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DDRG**](@ref kMsgDDragInfo) | @ref kMsgDDragInfo | Data | Server→Client | Drag file info | [MsgSize](#constraint-protocol-max-message-length), [ListSize](#constraint-max-list) | 1.5+ |
| [**DFTR**](@ref kMsgDFileTransfer) | @ref kMsgDFileTransfer | Data | Both | File transfer data | [MsgSize](#constraint-protocol-max-message-length) | 1.5+ |
| [**DINF**](@ref kMsgDInfo) | @ref kMsgDInfo | Data | Client→Server | Screen information | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DKDL**](@ref kMsgDKeyDownLang) | @ref kMsgDKeyDownLang | Data | Server→Client | Key down with language | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.8+ |
| [**DKDN**](@ref kMsgDKeyDown) | @ref kMsgDKeyDown | Data | Server→Client | Key down | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.1+ |
| [**DKDN**](@ref kMsgDKeyDown1_0) | @ref kMsgDKeyDown1_0 | Data | Server→Client | Key down (legacy) | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.0 |
| [**DKRP**](@ref kMsgDKeyRepeat) | @ref kMsgDKeyRepeat | Data | Server→Client | Key repeat | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.1+ |
| [**DKRP**](@ref kMsgDKeyRepeat1_0) | @ref kMsgDKeyRepeat1_0 | Data | Server→Client | Key repeat (legacy) | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.0 |
| [**DKUP**](@ref kMsgDKeyUp) | @ref kMsgDKeyUp | Data | Server→Client | Key up | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.1+ |
| [**DKUP**](@ref kMsgDKeyUp1_0) | @ref kMsgDKeyUp1_0 | Data | Server→Client | Key up (legacy) | [MsgSize](#constraint-protocol-max-message-length), [KeyMap](#constraint-keymap) | 1.0 |
| [**DMDN**](@ref kMsgDMouseDown) | @ref kMsgDMouseDown | Data | Server→Client | Mouse down | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DMMV**](@ref kMsgDMouseMove) | @ref kMsgDMouseMove | Data | Server→Client | Mouse move (absolute) | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DMRM**](@ref kMsgDMouseRelMove) | @ref kMsgDMouseRelMove | Data | Server→Client | Mouse move (relative) | [MsgSize](#constraint-protocol-max-message-length) | 1.2+ |
| [**DMUP**](@ref kMsgDMouseUp) | @ref kMsgDMouseUp | Data | Server→Client | Mouse up | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**DMWM**](@ref kMsgDMouseWheel) | @ref kMsgDMouseWheel | Data | Server→Client | Mouse wheel | [MsgSize](#constraint-protocol-max-message-length) | 1.3+ |
| [**DMWM**](@ref kMsgDMouseWheel1_0) | @ref kMsgDMouseWheel1_0 | Data | Server→Client | Mouse wheel (legacy) | [MsgSize](#constraint-protocol-max-message-length) | 1.0-1.2 |
| [**DSOP**](@ref kMsgDSetOptions) | @ref kMsgDSetOptions | Data | Server→Client | Set options | [MsgSize](#constraint-protocol-max-message-length), [ListSize](#constraint-max-list) | 1.0+ |
| [**EBAD**](@ref kMsgEBad) | @ref kMsgEBad | Error | Server→Client | Protocol violation | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**EBSY**](@ref kMsgEBusy) | @ref kMsgEBusy | Error | Server→Client | Server busy | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**EICV**](@ref kMsgEIncompatible) | @ref kMsgEIncompatible | Error | Server→Client | Incompatible version | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**EUNK**](@ref kMsgEUnknown) | @ref kMsgEUnknown | Error | Server→Client | Unknown client | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**Hello**](@ref kMsgHello) | @ref kMsgHello | Handshake | Server→Client | Protocol identification | [HelloSize](#constraint-max-hello), [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**HelloArgs**](@ref kMsgHelloArgs) | @ref kMsgHelloArgs | Handshake | Internal | Hello message construction | [HelloSize](#constraint-max-hello), [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**HelloBack**](@ref kMsgHelloBack) | @ref kMsgHelloBack | Handshake | Client→Server | Client identification | [HelloSize](#constraint-max-hello), [MsgSize](#constraint-protocol-max-message-length), [HandshakeTimeout](#constraint-handshake-timeout) | 1.0+ |
| [**HelloBackArgs**](@ref kMsgHelloBackArgs) | @ref kMsgHelloBackArgs | Handshake | Internal | HelloBack message construction | [HelloSize](#constraint-max-hello), [MsgSize](#constraint-protocol-max-message-length), [HandshakeTimeout](#constraint-handshake-timeout) | 1.0+ |
| [**LSYN**](@ref kMsgDLanguageSynchronisation) | @ref kMsgDLanguageSynchronisation | Data | Server→Client | Language synchronization | [MsgSize](#constraint-protocol-max-message-length) | 1.8+ |
| [**QINF**](@ref kMsgQInfo) | @ref kMsgQInfo | Query | Server→Client | Request screen info | [MsgSize](#constraint-protocol-max-message-length) | 1.0+ |
| [**SECN**](@ref kMsgDSecureInputNotification) | @ref kMsgDSecureInputNotification | Data | Server→Client | Secure input notification | [MsgSize](#constraint-protocol-max-message-length) | 1.7+ |

## Typical Control Flow
<a id="typical-control-flow"></a>

A typical control flow is as follows:
1.  **Handshake**: The server and client exchange `Hello` and `HelloBack` messages to agree on a protocol version.
2.  **Information Exchange**: The server requests client information with `QINF`, and the client responds with `DINF`.
3.  **Options**: The server sends `DSOP` to configure client options.
4.  **Keep-Alive**: The server and client periodically exchange `CALV` messages to maintain the connection.
5.  **Screen Entry**: The server sends `CINN` to grant control to the client.
6.  **Input Events**: The server sends a stream of input event messages (e.g., `DMMV`, `DMDN`, `DKDN`).
7.  **Screen Leave**: The server sends `COUT` to revoke control from the client.
8.  **Connection Close**: The server sends `CCLOSE` to terminate the connection.

## Protocol Constraints

To ensure security, stability, and compatibility, the protocol enforces strict constraints:

<a id="constraint-max-msg"></a>

### Message and Data Size Limits

**Maximum Message Size:**  
<a id="constraint-protocol-max-message-length"></a>**4,194,304 bytes (4 MB)** — @ref PROTOCOL_MAX_MESSAGE_LENGTH  
Maximum total size of any single, length-prefixed data packet  
Defined in Protocol Limits

**Maximum List Size:**  
<a id="constraint-max-list"></a>**1,048,576 elements** — @ref PROTOCOL_MAX_LIST_LENGTH  
Maximum number of items in a list/vector within a message  
Defined in Protocol Limits

**Maximum Hello Size:**  
<a id="constraint-max-hello"></a>**1,024 bytes** — @ref kMaxHelloLength  
Maximum size of the initial Connection Handshake message  
Defined in Protocol Limits

<a id="constraint-tls"></a>

### TLS Handshake and Security (Protocol v1.4+)

When encryption is enabled, the protocol follows this sequence:

1. Standard TCP connection established
2. TLS handshake performed over TCP socket
3. Protocol handshake begins only after TLS session is established

- **Implementation Details**:
    - The client initiates a standard TCP connection, then the @ref SecureSocket::handleTCPConnected method is called, which begins the TLS handshake

- **Certificate Validation**:
    - Client implementations **must** validate the server's certificate
    - The reference implementation checks that the public key is RSA or DSA and that the key length is at least 2048 bits

### Key Code and Modifier Mapping

A modifier (modifier mask) represents the state of modifier keys (like Shift, Control, Alt, and Command) on a keyboard. It is a binary code (like 0000 0110) where each bit corresponds to a specific modifier key.

**Key-Up/Key-Down Strategy:**
- <a id="constraint-keymap"></a>Client must use the @ref KeyButton (physical key) to track pressed keys, as the @ref KeyID (virtual key) can change based on modifier state
- This strategy is described in the documentation for @ref kMsgDKeyDown

**Modifier Remapping:**
- The server can command clients to remap modifier keys via the @ref kMsgDSetOptions message
- The client processes the @ref kMsgDSetOptions message and updates the modifier translation table accordingly

## Timing and Synchronization

<a id="constraint-keep-alive"></a>

### Keep-Alive Mechanism (Protocol v1.3+)

**Server-Side Behavior:**
- The server sends kMsgCKeepAlive messages every 3.0 seconds (defined by @ref kKeepAliveRate)
- This timer is implemented in @ref ClientProxy1_3::addHeartbeatTimer in the @ref ClientProxy1_3 class

**Client-Side Behavior:**
- Upon receiving a kMsgCKeepAlive message, the client must immediately send a kMsgCKeepAlive message back
- The client maintains a timeout that is reset each time any message is received
- If no message is received for 9.0 seconds (3 × @ref kKeepAliveRate), client must disconnect
- This is handled by the @ref ServerProxy::handleKeepAliveAlarm method

<a id="constraint-screen-entry-sync"></a>
### Synchronization on Screen Entry

- The @ref kMsgCEnter (Enter Screen) message includes the current modifier state
- Client must synchronize their local modifier state with this mask

<a id="constraint-handshake-timeout"></a>
### Handshake Timeout

- Server allows **30 seconds** for handshake completion
- If client fails to send valid @ref kMsgHelloBack within this time, connection is closed
- When a new client connects, the server creates a temporary @ref ClientProxyUnknown to handle the version handshake
- A one-shot timer is started for 30 seconds
- If the client fails to respond in time, the @ref protocol_errors function is triggered, the connection is logged as unresponsive, and the socket is closed

## Version Compatibility

| Version | Release Date | Project | Features | Compatibility |
|---------|----------|---------------|----------|---------------|
| **1.0** | May 2001 | Synergy | Basic keyboard/mouse sharing (@ref kMsgDKeyDown, @ref kMsgDMouseMove) | All versions |
| **1.1** | Apr 2002 | Synergy | Physical key codes (@ref KeyButton) | 1.1+ |
| **1.2** | Jan 2006 | Synergy | Relative mouse movement | 1.2+ |
| **1.3** | May 2006 | Synergy | Keep-alive (@ref kMsgCKeepAlive), horizontal scroll (@ref kMsgDMouseWheel) | 1.3+ |
| **1.4** | Nov 2012 | Synergy | Encryption support (@ref SecureSocket) | 1.4+ |
| **1.5** | Sep 2013 | Synergy | File transfer | 1.5+ |
| **1.6** | Jan 2014 | Synergy | Clipboard streaming | 1.6+ |
| **1.7** | Nov 2021 | Barrier | Secure input notifications | 1.7+ |
| **1.8** | Jun 2025 | Deskflow | Language synchronization | 1.8+ |

### Version Migration Guide

When implementing a client that supports multiple protocol versions:

1. **Version Negotiation**: During handshake, client should advertise highest supported version
2. **Feature Detection**: Check server's version in `Hello` message before using version-specific features
3. **Fallback Mechanism**: Be prepared to operate with only features available in the negotiated version
4. **Graceful Degradation**: If server supports a lower version than client's minimum, handle `EIncompatible` error gracefully

## Implementation Examples

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

### Complete Message Exchange Sequence

Below is a typical message exchange sequence for a client connecting to a server:



```
Client                                 Server
  |                                      |
  |                                      | Server starts listening on port 24800
  |                                      |
  | TCP SYN                              |
  | ───────────────────────────────────► |
  |                                      |
  | ◄─────────────────────────────────── |
  | TCP SYN+ACK                          |
  |                                      |
  | TCP ACK                              |
  | ───────────────────────────────────► |
  |                                      | TCP connection established
  |                                      |
  | ◄─────────────────────────────────── |
  | "Deskflow" + version (1.8)           | Hello message
  |                                      |
  | "Deskflow" + version + name          |
  | ───────────────────────────────────► | HelloBack message
  |                                      |
  | ◄─────────────────────────────────── |
  | "QINF"                               | Query screen info
  |                                      |
  | "DINF" + screen dimensions           |
  | ───────────────────────────────────► | Report screen info
  |                                      |
  | ◄─────────────────────────────────── |
  | "DSOP" + options                     | Set options
  |                                      |
  | ◄─────────────────────────────────── |
  | "CALV"                               | Keep-alive
  |                                      |
  | "CALV"                               |
  | ───────────────────────────────────► | Keep-alive response
  |                                      |
  | ◄─────────────────────────────────── |
  | "CINN" + x + y + seq + mask          | Enter screen
  |                                      |
  | ◄─────────────────────────────────── |
  | "DMMV" + x + y                       | Mouse move
  |                                      |
  | ◄─────────────────────────────────── |
  | "DMDN" + button                      | Mouse down
  |                                      |
  | ◄─────────────────────────────────── |
  | "DMUP" + button                      | Mouse up
  |                                      |
  | ◄─────────────────────────────────── |
  | "DKDN" + key + mask + button         | Key down
  |                                      |
  | ◄─────────────────────────────────── |
  | "DKUP" + key + mask + button         | Key up
  |                                      |
  | ◄─────────────────────────────────── |
  | "COUT"                               | Leave screen
  |                                      |
  | ◄─────────────────────────────────── |
  | "CCLOSE"                             | Close connection
  |                                      |
  | TCP FIN                              |
  | ◄─────────────────────────────────── |
  |                                      |
  | TCP FIN+ACK                          |
  | ───────────────────────────────────► |
  |                                      | Connection closed
```

**Legend:**

- Hello message: @ref kMsgHello
- HelloBack message: @ref kMsgHelloBack
- Query screen info: @ref kMsgQInfo
- Report screen info: @ref kMsgDInfo
- Set options: @ref kMsgDSetOptions
- Keep-alive: @ref kMsgCKeepAlive
- Enter screen: @ref kMsgCEnter
- Mouse move: @ref kMsgDMouseMove
- Mouse down: @ref kMsgDMouseDown
- Mouse up: @ref kMsgDMouseUp
- Key down: @ref kMsgDKeyDown
- Key up: @ref kMsgDKeyUp
- Leave screen: @ref kMsgCLeave
- Close connection: @ref kMsgCClose

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

## Platform-Specific Implementations

For platform-specific implementation details, refer to:
- @ref ProtocolTypes.h – Complete protocol specification
- @ref ProtocolUtil.h – Message formatting utilities

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
  - [ ] Data transfer (@ref kMsgDClipboard - text, images, HTML)
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

The @ref ServerProxy class provides a reference implementation demonstrating:

- Basic protocol handling
- Message parsing and generation
- Connection management
- Input event processing


## Contributing

When extending the protocol:

1. **Maintain Compatibility**: New features should be backward compatible
2. **Update Documentation**: Document new messages and parameters
3. **Version Increment**: Bump minor version for new features
4. **Test Thoroughly**: Verify with existing clients and servers

## Support and Resources


- @ref ProtocolTypes.h – Complete protocol specification
- @ref ProtocolUtil.h – Message formatting utilities

---

*This documentation is generated from the source code and is always up-to-date with the latest protocol implementation.*