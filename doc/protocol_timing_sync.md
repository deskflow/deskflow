# Protocol Timing/Synch

This document provides validated specifications for the Deskflow protocol's timing and synchronization mechanisms. Implementations must follow these rules to ensure interoperability and a stable user experience.

## Keep-Alive Mechanism (Protocol v1.3+)

The keep-alive mechanism is used to detect unresponsive clients or network interruptions.

-   **Server-Side Behavior**:
    -   The server sends a `CALV` (Keep-Alive) message to the client periodically.
    -   The default interval is defined by [`kKeepAliveRate`](file:///P:/work/deskflow/doc/build/html/group__protocol__constants.html#ga8460e7dc3aae99da87dcff40b2fad424) as **3.0 seconds**.
    -   **Source**: The timer is implemented in `addHeartbeatTimer` in the [`ClientProxy1_3` documentation](file:///P:/work/deskflow/doc/build/html/_client_proxy1__3_8h.html).

-   **Client-Side Behavior**:
    -   Upon receiving a `CALV` message, the client must immediately send a `CALV` message back to the server. This is handled in [`ServerProxy::parseMessage`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#ad9f78802b421e2ac23d300303616fcff).
    -   The client maintains a timeout that is reset each time any message is received from the server. If this timer expires, the client must disconnect. The timeout is `kKeepAliveRate` * [`kKeepAlivesUntilDeath`](file:///P:/work/deskflow/doc/build/html/group__protocol__constants.html#ga5c730a96bc41c32cb014dff8d28f3a28) (3.0 * 3.0 = 9 seconds).
    -   **Source**: The logic is in `ServerProxy`: the rate is set in [`setKeepAliveRate`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#a0f700481b5a5136288a40277173cfe7f) and the timeout is handled by [`handleKeepAliveAlarm`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#ae42b407bc1b6100fdee3eb0cf9a05d59).

## Synchronization on Screen Entry

To ensure the client's modifier key state matches the server's, the server sends the current modifier state as part of the `CINN` (Enter Screen) message.

-   **Modifier Key Sync**:
    -   The [`CINN` message](file:///P:/work/deskflow/doc/build/html/group__protocol__commands.html#gae826571a53b798c573418568f4397190) includes the `KeyModifierMask`, an 8-bit mask representing the state of all modifier keys at the moment the cursor entered the client's screen.
    -   The client **must** use this mask to synchronize its local modifier state before processing any subsequent input events. This is handled in [`ServerProxy::enter`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#aab6e8d24471d96f47aaf63f7dbb78f88).

## Handshake Timeout

To prevent resources from being held by unresponsive new connections, the server enforces a timeout for the handshake process.

-   **Timeout Value**: **30 seconds**
-   **Details**: When a new client connects, the server creates a temporary `ClientProxyUnknown` to handle the version handshake. A one-shot timer is started for 30 seconds. If the client fails to send a valid `HelloBack` message within this time, the `handleTimeout` function is triggered, the connection is logged as unresponsive, and the socket is closed.
-   **Source**: The timeout value is hardcoded during the instantiation of `ClientProxyUnknown` in the [`ClientListener` documentation](file:///P:/work/deskflow/doc/build/html/_client_listener_8h.html). The timer logic is in the constructor and `handleTimeout` function documented in the [`ClientProxyUnknown` documentation](file:///P:/work/deskflow/doc/build/html/_client_proxy_unknown_8h.html).
