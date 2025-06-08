# Protocol State Machine

This document specifies the client-side state machine for the Deskflow protocol. A client must implement this logic to correctly manage its connection to the server.

## States and Transitions

The client's connection lifecycle is defined by five primary states. Transitions are triggered by network events and messages from the server.

```
                        ┌─────────────────┐
                        │      START      │
                        └─────────┬───────┘
                                  │
                                  v
                        ┌─────────────────┐
                        │  DISCONNECTED   │◄──────────────────┐
                        │  (Initial &     │                   │
                        │   Final State)  │                   │
                        └─────────┬───────┘                   │
                                  │                           │
                         TCP Connect                          │
                                  │                           │
                                  v                           │
                        ┌─────────────────┐                   │
                        │   CONNECTING    │                   │
                        │ (TCP handshake) │                   │
                        └─────┬─────┬─────┘                   │
                              │     │                         │
                   TCP Success│     │TCP Failure              │
                              │     └─────────────────────────┤
                              v                               │
                        ┌─────────────────┐                   │
                        │   HANDSHAKE     │                   │
                        │ (Hello/HelloBk) │                   │
                        └─────┬─────┬─────┘                   │
                              │     │                         │
             Hello/HelloBack  │     │Version ismatch          │
                       OK     │     └─────────────────────────┤
                              v                               │
                        ┌─────────────────┐                   │
                   ┌────┤   CONNECTED     │                   │
                   │    │ (Authenticated  │                   │
                   │    │  but inactive)  │                   │
                   │    └─────┬─────┬─────┘                   │
                   │          │     │                         │
            COUT   │   CINN   │     │CCLOSE                   │
          (Leave)  │ (Enter)  │     └─────────────────────────┤
                   │          v                               │
                   │    ┌─────────────────┐                   │
                   │    │     ACTIVE      │                   │
                   │    │ (Receiving all  │                   │
                   │    │ input events)   │                   │
                   │    └─────────┬───────┘                   │
                   │              │                           │
                   └──────────────┤                           │
                                  │                           │
                           CCLOSE │                           │
                           (Close)│                           │
                                  └───────────────────────────┘

Legend:
  ┌─────────┐  States (rectangles)
  │  STATE  │
  └─────────┘

  ──────────→  Transitions (arrows with labels)
```

### State: Disconnected

This is the initial and final state. The client is not connected to the server.

### State: Connecting

The client enters this state upon attempting a TCP connection to the server's port (default `24800`).

-   **Entry**: Initiating TCP socket connection.
-   **Success Transition**: On successful TCP connection, the client moves to the `Handshake` state.
-   **Failure Transition**: If the TCP connection fails (e.g., timeout, RST packet), the client returns to the `Disconnected` state.

### State: Handshake

Upon establishing a TCP connection, the client and server perform a handshake to verify protocol compatibility.

1.  **Server Sends `Hello`**: The server sends a `Deskflow` message with its protocol version.
    -   **Source**: This message is sent immediately upon connection by the server in the [`ClientProxyUnknown`](file:///P:/work/deskflow/doc/build/html/class_client_proxy_unknown.html) constructor.
2.  **Client Responds `HelloBack`**: The client receives `Hello` and responds with its own `Deskflow` message, including its version and screen name.
3.  **Validation**: The server validates the client's `HelloBack` message.
    -   **Success Transition**: Upon receiving a valid `HelloBack`, the server transitions the client to a versioned protocol handler. See [`ClientProxy1_0::parseHandshakeMessage`](file:///P:/work/deskflow/doc/build/html/class_client_proxy1__0.html).
    -   **Failure Transition**: An `EIncompatible` error is sent if versions do not match, and the connection is closed.

### State: Connected

The client is connected and authenticated, but the server's input is not being forwarded to it. In this state, the client primarily listens for keep-alive messages and screen entry commands.

-   **Keep-Alive**: The client must respond to `CALV` messages from the server. Failure to do so will result in a timeout and transition to `Disconnected`.
    -   **Source**: See [`ServerProxy::parseMessage`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#ad9f78802b421e2ac23d300303616fcff) for the echo logic and [`ServerProxy::handleKeepAliveAlarm`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#ae42b407bc1b6100fdee3eb0cf9a05d59) for the timeout.
-   **Enter Screen**: Receiving a `CINN` message transitions the state to `Active`.

### State: Active

The client is the active screen. It receives and processes all input events from the server (mouse, keyboard, clipboard).

-   **Leave Screen**: Receiving a `COUT` message transitions the state back to `Connected`.
-   **Close Connection**: Receiving a `CCLOSE` message transitions the state to `Disconnected`. 