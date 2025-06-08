# Protocol Constraints

This document details mandatory constraints for any client implementation of the Deskflow protocol. Adherence to these rules is required for security, stability, and compatibility.

## Message and Data Size Limits

To prevent resource exhaustion and denial-of-service vectors, the protocol enforces strict size limits on all messages and data structures. These limits are validated by the server and will result in a disconnect if violated.

-   **PROTOCOL\_MAX\_MESSAGE\_LENGTH** (Maximum Total Message Size)
    -   **Value**: 4,194,304 bytes (4 MB)
    -   **Details**: The total size of any single, length-prefixed data packet cannot exceed this value.
    -   **Source**: Defined in [`ProtocolTypes.h`](file:///P:/work/deskflow/doc/build/html/group__protocol__constants.html#gad5b510266c8a03bcd0ee9745ae7046b6). Enforced in [`PacketStreamFilter::readPacketSize`](file:///P:/work/deskflow/doc/build/html/_packet_stream_filter_8h.html).

-   **PROTOCOL\_MAX\_LIST\_LENGTH** (Maximum List/Vector Size)
    -   **Value**: 1,048,576 elements (1 million)
    -   **Details**: When a message contains a list of items (e.g., options in `DSetOptions`), the 32-bit integer specifying the number of items cannot exceed this value.
    -   **Source**: Defined in [`ProtocolTypes.h`](file:///P:/work/deskflow/doc/build/html/group__protocol__constants.html#gab0f7d7d0c8719715c9d6dcb2ef83e8f0). Enforced in `ProtocolUtil::readVectorSize` in `src/lib/deskflow/ProtocolUtil.cpp`.

-   **kMaxHelloLength** (Maximum Handshake (Hello) Length)
    -   **Value**: 1024 bytes
    -   **Details**: The initial `HelloBack` message from the client, which includes the client's name, is restricted to this size.
    -   **Source**: Defined in [`ProtocolTypes.h`](file:///P:/work/deskflow/doc/build/html/group__protocol__constants.html#ga98962d84082470b4473418edba98067c).

## TLS Handshake and Security (Protocol v1.4+)

When encryption is enabled, the Deskflow protocol is tunnelled over a standard TLS session.

-   **Handshake Sequence**:
    1.  A standard TCP connection is established.
    2.  A TLS handshake is performed over the established TCP socket to create a secure channel.
    3.  **Only after the TLS session is successfully established** does the Deskflow protocol handshake (`Hello`/`HelloBack`) begin.
    -   **Source**: The client initiates a standard TCP connection, then [`SecureSocket::handleTCPConnected`](file:///P:/work/deskflow/doc/build/html/class_secure_socket.html#a1bf476ecdf629ab92445e40b5582d463) is called, which begins the TLS handshake.

-   **Certificate Validation**:
    -   Client implementations **must** validate the server's certificate. The reference implementation checks for specific key properties.
    -   **Source**: The function `TlsCertificate::isCertificateValid` in `src/lib/gui/tls/TlsCertificate.cpp` checks that the public key is `EVP_PKEY_RSA` or `EVP_PKEY_DSA` and that the key length is at least 2048 bits.

## Key Code and Modifier Mapping

A critical responsibility of a client is to correctly map the protocol's abstract key identifiers to the native platform's input system.

-   **Key-Up/Key-Down Strategy**:
    -   A key-down event (`DKDN`) provides both a `KeyID` (virtual) and a `KeyButton` (physical). A client **must** use the `KeyButton` to track which key is currently pressed, as the `KeyID` can change based on modifier state.
    -   **Source**: This strategy is described in the documentation for [`kMsgDKeyDown`](file:///P:/work/deskflow/doc/build/html/group__protocol__keyboard.html#ga3347d0669e9605acefc17c4f1ee07a7d).

-   **Modifier Remapping**:
    -   The server can command the client to remap modifier keys.
    -   **Source**: The [`ServerProxy::setOptions`](file:///P:/work/deskflow/doc/build/html/class_server_proxy.html#a5d63c19f41be6e69540c213bc88979a1) function iterates through the options received in a `DSetOptions` message and updates the modifier translation table. 