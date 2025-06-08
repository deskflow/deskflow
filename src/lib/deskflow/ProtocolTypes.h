/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

/**
 * @file ProtocolTypes.h
 * @brief Deskflow Network Protocol Specification and Implementation
 * 
 * @section protocol_overview Protocol Overview
 * 
 * The Deskflow protocol enables keyboard and mouse sharing between multiple computers
 * over a TCP network connection. The protocol operates in a client-server model where:
 * - **Primary (Server)**: The computer whose keyboard and mouse are shared
 * - **Secondary (Client)**: The computer that receives input from the primary
 * 
 * @section protocol_architecture Protocol Architecture
 * 
 * ```
 * [Primary Computer] ←→ [Network] ←→ [Secondary Computer]
 *      (Server)                         (Client)
 * ```
 * 
 * @section protocol_versions Protocol Versions
 * 
 * The protocol has evolved through several versions with backward compatibility:
 * - **1.0**: Initial protocol with basic keyboard/mouse sharing
 * - **1.1**: Added KeyCode to key press, release, and repeat messages
 * - **1.2**: Added relative mouse motion support
 * - **1.3**: Added keep-alive mechanism, deprecated heartbeats, added horizontal scrolling
 * - **1.4**: Added cryptographic support for secure connections
 * - **1.5**: Added file transfer capabilities, removed homebrew crypto
 * - **1.6**: Added clipboard streaming for large clipboard data
 * - **1.7**: Added secure input notifications (macOS)
 * - **1.8**: Added language synchronization functionality
 * 
 * @section protocol_message_format Message Format
 * 
 * All protocol messages follow a consistent format:
 * 
 * **Standard Message Structure:**
 * ```
 * [4-byte Message Code][Parameters...]
 * ```
 * 
 * **Message with Length Prefix:**
 * ```
 * [4-byte Length][4-byte Message Code][Parameters...]
 * ```
 * 
 * @subsection protocol_data_types Data Types
 * 
 * - **Integers**: All multi-byte integers are transmitted in Network Byte Order (big-endian)
 * - **Strings**: UTF-8 encoded, length-prefixed
 * - **Coordinates**: Signed 16-bit integers for screen positions
 * - **Sizes**: Unsigned integers for dimensions
 * 
 * @section protocol_handshake Connection Handshake
 * 
 * The connection establishment follows this sequence:
 * 
 * 1. **Client connects** to server on port 24800 (default)
 * 2. **Server sends Hello**: `kMsgHello` with protocol version and server name
 * 3. **Client responds**: `kMsgHelloBack` with protocol version and client name
 * 4. **Version negotiation**: Both sides use the minimum supported version
 * 5. **Ready for operation**: Normal message exchange begins
 * 
 * @subsection protocol_handshake_example Handshake Example
 * 
 * ```
 * Server → Client: "Deskflow\x01\x08" (Hello: Deskflow, version 1.8)
 * Client → Server: "Deskflow\x01\x08client-name" (HelloBack: version 1.8, name)
 * ```
 * 
 * @section protocol_message_flow Message Flow
 * 
 * @subsection protocol_screen_entry Screen Entry Sequence
 * 
 * When the mouse moves from primary to secondary screen:
 * 
 * 1. **Server sends**: `kMsgCEnter` with entry position and modifier state
 * 2. **Client acknowledges**: Ready to receive input
 * 3. **Input forwarding**: All keyboard/mouse events sent to client
 * 4. **Server sends**: `kMsgCLeave` when mouse exits secondary screen
 * 
 * @subsection protocol_input_events Input Event Messages
 * 
 * **Keyboard Events:**
 * - `kMsgDKeyDown`: Key press with KeyID, modifiers, and physical key code
 * - `kMsgDKeyUp`: Key release
 * - `kMsgDKeyRepeat`: Auto-repeat events
 * 
 * **Mouse Events:**
 * - `kMsgDMouseMove`: Absolute mouse position
 * - `kMsgDMouseRelMove`: Relative mouse movement
 * - `kMsgDMouseDown`/`kMsgDMouseUp`: Button press/release
 * - `kMsgDMouseWheel`: Scroll wheel events
 * 
 * @section protocol_clipboard Clipboard Synchronization
 * 
 * Clipboard data is synchronized between primary and secondary screens:
 * 
 * 1. **Clipboard grab**: `kMsgCClipboard` indicates clipboard ownership change
 * 2. **Data transfer**: `kMsgDClipboard` transfers actual clipboard content
 * 3. **Streaming support**: Large clipboard data split into chunks (v1.6+)
 * 
 * @section protocol_file_transfer File Transfer (v1.5+)
 * 
 * File transfer uses a chunked protocol:
 * 
 * 1. **Drag initiation**: `kMsgDDragInfo` with file list
 * 2. **File transfer**: `kMsgDFileTransfer` with chunked data
 * 3. **Transfer states**: Start (size), Chunk (data), End (completion)
 * 
 * @section protocol_keepalive Keep-Alive Mechanism (v1.3+)
 * 
 * To detect connection failures:
 * 
 * - **Server sends**: `kMsgCKeepAlive` every 3 seconds (configurable)
 * - **Client responds**: Same message back to server
 * - **Timeout detection**: 3 missed keep-alives trigger disconnection
 * 
 * @section protocol_security Security Considerations
 * 
 * - **Encryption**: TLS/SSL support for secure connections (v1.4+)
 * - **Authentication**: Optional client authentication
 * - **Input validation**: All messages validated against maximum lengths
 * - **Secure input**: Notifications for secure input mode (macOS, v1.7+)
 * 
 * @section protocol_implementation Implementation Notes
 * 
 * @subsection protocol_compatibility Backward Compatibility
 * 
 * - Newer clients can connect to older servers
 * - Protocol version negotiated during handshake
 * - Graceful degradation of features not supported in older versions
 * 
 * @subsection protocol_error_handling Error Handling
 * 
 * - `kMsgEIncompatible`: Version mismatch
 * - `kMsgEBusy`: Client name already in use
 * - `kMsgEUnknown`: Client not in server configuration
 * - `kMsgEBad`: Protocol violation
 * 
 * @subsection protocol_limits Protocol Limits
 * 
 * - Maximum message length: 4MB
 * - Maximum string length: 1MB
 * - Maximum list length: 1MB
 * - Clipboard data: 32KB per chunk
 * 
 * @section protocol_reference Reference Implementation
 * 
 * For a minimal reference implementation, see:
 * - [uSynergy micro client](https://github.com/symless/synergy-micro-client)
 * - This demonstrates basic protocol handling in ~600 lines of C code
 * 
 * @author Chris Schoeneman (original protocol design)
 * @author Symless Ltd. (protocol extensions)
 * @since Protocol version 1.0 (2002)
 */

#pragma once

#include "base/EventTypes.h"

#include <cstdint>

/**
 * @defgroup protocol_constants Protocol Constants
 * @brief Core protocol version and configuration constants
 * @{
 */

/**
 * @brief Protocol major version number
 * 
 * The major version indicates fundamental protocol compatibility.
 * Clients and servers with different major versions cannot communicate.
 * 
 * @since Protocol version 1.0
 */
static const int16_t kProtocolMajorVersion = 1;

/**
 * @brief Protocol minor version number
 * 
 * The minor version indicates feature availability within the same major version.
 * Higher minor versions are backward compatible with lower minor versions.
 * 
 * **Version History:**
 * - **1.0**: Initial protocol with basic keyboard/mouse sharing
 * - **1.1**: Added KeyCode to key press, release, and repeat messages
 * - **1.2**: Added relative mouse motion support
 * - **1.3**: Added keep-alive mechanism, deprecated heartbeats, added horizontal scrolling
 * - **1.4**: Added cryptographic support for secure connections
 * - **1.5**: Added file transfer capabilities, removed homebrew crypto
 * - **1.6**: Added clipboard streaming for large clipboard data
 * - **1.7**: Added secure input notifications (macOS)
 * - **1.8**: Added language synchronization functionality
 * 
 * @note When incrementing the minor version, the Deskflow application version should also increment
 * @since Protocol version 1.0
 */
static const int16_t kProtocolMinorVersion = 8;

/**
 * @brief Default TCP port for Deskflow connections
 * 
 * Clients connect to this port on the server by default.
 * Can be overridden in configuration.
 * 
 * @since Protocol version 1.0
 */
static const uint16_t kDefaultPort = 24800;

/**
 * @brief Maximum length for client greeting message
 * 
 * Limits the size of the hello message sent by clients during handshake.
 * Prevents memory exhaustion attacks during connection establishment.
 * 
 * @since Protocol version 1.0
 */
static const uint32_t kMaxHelloLength = 1024;

/**
 * @brief Keep-alive message interval in seconds
 * 
 * Time between kMsgCKeepAlive messages sent by the server.
 * A non-positive value disables keep-alives.
 * This is the default rate that can be overridden using options.
 * 
 * @see kMsgCKeepAlive
 * @since Protocol version 1.3
 */
static const double kKeepAliveRate = 3.0;

/**
 * @brief Keep-alive timeout threshold
 * 
 * Number of consecutive missed kMsgCKeepAlive messages that indicates
 * a connection problem. After this many missed messages, the connection
 * is considered dead and will be terminated.
 * 
 * @see kMsgCKeepAlive
 * @since Protocol version 1.3
 */
static const double kKeepAlivesUntilDeath = 3.0;

/**
 * @brief Obsolete heartbeat rate (deprecated)
 * 
 * @deprecated Since protocol version 1.3, replaced by keep-alive mechanism
 * @see kKeepAliveRate
 */
static const double kHeartRate = -1.0;

/**
 * @brief Obsolete heartbeat timeout (deprecated)
 * 
 * @deprecated Since protocol version 1.3, replaced by keep-alive mechanism
 * @see kKeepAlivesUntilDeath
 */
static const double kHeartBeatsUntilDeath = 3.0;

/**
 * @brief Maximum allowed message length
 * 
 * Messages exceeding this size indicate a likely protocol error.
 * Such messages are not parsed and cause connection termination.
 * This prevents memory exhaustion attacks.
 * 
 * @note Clipboard messages are separately limited to 32KB chunks
 * @since Protocol version 1.0
 */
static constexpr uint32_t PROTOCOL_MAX_MESSAGE_LENGTH = 4 * 1024 * 1024;

/**
 * @brief Maximum allowed list length in protocol messages
 * 
 * Limits the size of lists (arrays) in protocol messages to prevent
 * memory exhaustion attacks.
 * 
 * @since Protocol version 1.0
 */
static constexpr uint32_t PROTOCOL_MAX_LIST_LENGTH = 1024 * 1024;

/**
 * @brief Maximum allowed string length in protocol messages
 * 
 * Limits the size of strings in protocol messages to prevent
 * memory exhaustion attacks.
 * 
 * @since Protocol version 1.0
 */
static constexpr uint32_t PROTOCOL_MAX_STRING_LENGTH = 1024 * 1024;

/** @} */ // end of protocol_constants group

/**
 * @defgroup protocol_enums Protocol Enumerations
 * @brief Enumeration types used in protocol messages
 * @{
 */

/**
 * @brief Screen edge directions for mouse movement
 * 
 * Used to specify which edge of a screen the mouse cursor crosses
 * when moving between primary and secondary screens.
 * 
 * @since Protocol version 1.0
 */
enum EDirection
{
  kNoDirection,     ///< No specific direction
  kLeft,           ///< Left edge of screen
  kRight,          ///< Right edge of screen  
  kTop,            ///< Top edge of screen
  kBottom,         ///< Bottom edge of screen
  kFirstDirection = kLeft,    ///< First valid direction value
  kLastDirection = kBottom,   ///< Last valid direction value
  kNumDirections = kLastDirection - kFirstDirection + 1  ///< Total number of directions
};

/**
 * @brief Bitmask values for screen edge directions
 * 
 * Used to create bitmasks representing multiple screen edges.
 * Useful for configuration and edge detection.
 * 
 * @since Protocol version 1.0
 */
enum EDirectionMask
{
  kNoDirMask = 0,                ///< No direction mask
  kLeftMask = 1 << kLeft,        ///< Left edge mask
  kRightMask = 1 << kRight,      ///< Right edge mask
  kTopMask = 1 << kTop,          ///< Top edge mask
  kBottomMask = 1 << kBottom     ///< Bottom edge mask
};

/**
 * @brief File transfer data chunk types
 * 
 * Used in kMsgDFileTransfer messages to indicate the type of data
 * being transferred in each chunk.
 * 
 * @since Protocol version 1.5
 */
enum EDataTransfer
{
  kDataStart = 1,   ///< Start of transfer (contains file size)
  kDataChunk = 2,   ///< Data chunk (contains file content)
  kDataEnd = 3      ///< End of transfer (transfer complete)
};

/**
 * @brief Data reception status codes
 * 
 * Used internally to track the status of data reception
 * during file transfers and clipboard operations.
 * 
 * @since Protocol version 1.5
 */
enum EDataReceived
{
  kStart,      ///< Reception started
  kNotFinish,  ///< Reception in progress
  kFinish,     ///< Reception completed successfully
  kError       ///< Reception failed with error
};

/** @} */ // end of protocol_enums group

/**
 * @defgroup protocol_messages Protocol Message Definitions
 * @brief All protocol message types and their format specifications
 * 
 * @section message_format Message Format Specification
 * 
 * All protocol messages follow a consistent format:
 * - **Message codes** are always 4 bytes (ASCII characters)
 * - **Parameters** follow the message code as specified
 * - **Integers** are transmitted in Network Byte Order (big-endian)
 * - **Strings** are UTF-8 encoded with length prefixes
 * - **Positions and sizes** are signed 16-bit integers
 * 
 * In parameter descriptions, `$n` refers to the n'th argument (counting from one).
 * 
 * @section message_categories Message Categories
 * 
 * Messages are categorized by their first letter:
 * - **Greeting**: Connection handshake messages (no prefix)
 * - **C**: Command messages (server → client control)
 * - **D**: Data messages (input events, clipboard, files)
 * - **Q**: Query messages (information requests)
 * - **E**: Error messages (protocol violations, failures)
 * 
 * @{
 */

/**
 * @defgroup protocol_handshake Handshake Messages
 * @brief Connection establishment and version negotiation
 * @{
 */

/**
 * @brief Protocol name for Synergy compatibility
 * 
 * Used in handshake messages to identify the protocol as Synergy-compatible.
 * Must be exactly 7 characters for backward compatibility.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kSynergyProtocolName;

/**
 * @brief Protocol name for Barrier compatibility
 * 
 * Used in handshake messages to identify the protocol as Barrier-compatible.
 * Must be exactly 7 characters for backward compatibility.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kBarrierProtocolName;

/**
 * @brief Server hello message format
 * 
 * **Direction**: Primary → Secondary  
 * **Format**: `"%7s%2i%2i"`  
 * **Parameters**:
 * - `$1`: Protocol name (7 characters, e.g., "Deskflow")
 * - `$2`: Server major version number (2 bytes)
 * - `$3`: Server minor version number (2 bytes)
 * 
 * **Example**:
 * ```
 * "Deskflow\x00\x01\x00\x08"  // Deskflow version 1.8
 * ```
 * 
 * This is the first message sent by the server after a client connects.
 * The client uses this to determine protocol compatibility.
 * 
 * @see kMsgHelloBack
 * @since Protocol version 1.0
 */
extern const char *const kMsgHello;

/**
 * @brief Server hello arguments format (dynamic version)
 * 
 * **Format**: `"%2i%2i"`  
 * **Parameters**:
 * - `$1`: Server major version number (2 bytes)
 * - `$2`: Server minor version number (2 bytes)
 * 
 * Used as part of dynamic hello message construction.
 * This can be superseded by kMsgHello once ProtocolUtil::writef
 * supports fixed length strings (e.g., %7s).
 * 
 * @see kMsgHello
 * @since Protocol version 1.0
 */
extern const char *const kMsgHelloArgs;

/**
 * @brief Client hello response message format
 * 
 * **Direction**: Secondary → Primary  
 * **Format**: `"%7s%2i%2i%s"`  
 * **Parameters**:
 * - `$1`: Protocol name (7 characters, e.g., "Deskflow")
 * - `$2`: Client major version number (2 bytes)
 * - `$3`: Client minor version number (2 bytes)
 * - `$4`: Client name (string)
 * 
 * **Example**:
 * ```
 * "Deskflow\x00\x01\x00\x08\x00\x00\x00\x0Bworkstation"
 * // Deskflow version 1.8, client name "workstation"
 * ```
 * 
 * Sent by the client in response to kMsgHello. After this exchange,
 * both sides negotiate to use the minimum supported protocol version.
 * 
 * @see kMsgHello
 * @since Protocol version 1.0
 */
extern const char *const kMsgHelloBack;

/**
 * @brief Client hello arguments format (dynamic version)
 * 
 * **Format**: `"%2i%2i%s"`  
 * **Parameters**:
 * - `$1`: Client major version number (2 bytes)
 * - `$2`: Client minor version number (2 bytes)
 * - `$3`: Client name (string)
 * 
 * Used as part of dynamic hello message construction.
 * This can be superseded by kMsgHelloBack once ProtocolUtil::writef
 * supports fixed length strings (e.g., %7s).
 * 
 * @see kMsgHelloBack
 * @since Protocol version 1.0
 */
extern const char *const kMsgHelloBackArgs;

/** @} */ // end of protocol_handshake group

/**
 * @defgroup protocol_commands Command Messages
 * @brief Control messages for screen management and connection maintenance
 * @{
 */

/**
 * @brief No operation command
 * 
 * **Message Code**: `"CNOP"`  
 * **Direction**: Secondary → Primary  
 * **Format**: No parameters  
 * 
 * A no-operation message that can be used for testing connectivity
 * or as a placeholder. Has no effect on the receiving end.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgCNoop;

/**
 * @brief Close connection command
 * 
 * **Message Code**: `"CBYE"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Instructs the client to close the connection gracefully.
 * The client should clean up resources and disconnect.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgCClose;

/**
 * @brief Enter screen command
 * 
 * **Message Code**: `"CINN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"CINN%2i%2i%4i%2i"`  
 * **Parameters**:
 * - `$1`: Entry X coordinate (2 bytes, signed)
 * - `$2`: Entry Y coordinate (2 bytes, signed)
 * - `$3`: Sequence number (4 bytes, unsigned)
 * - `$4`: Modifier key mask (2 bytes, unsigned)
 * 
 * **Example**:
 * ```
 * "CINN\x01\x90\x01\x2C\x00\x00\x00\x01\x00\x00"
 * // Enter at (400, 300), sequence 1, no modifiers
 * ```
 * 
 * Sent when the mouse cursor enters the secondary screen from the primary.
 * The coordinates specify the exact entry point. The sequence number is used
 * to order messages and must be returned in subsequent messages from the client.
 * The modifier mask indicates which toggle keys (Caps Lock, Num Lock, etc.)
 * are active and should be synchronized on the secondary screen.
 * 
 * @see kMsgCLeave
 * @since Protocol version 1.0
 */
extern const char *const kMsgCEnter;

/**
 * @brief Leave screen command
 * 
 * **Message Code**: `"COUT"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Sent when the mouse cursor leaves the secondary screen and returns to the primary.
 * Upon receiving this message, the secondary screen should:
 * 1. Send clipboard data for any clipboards it has grabbed
 * 2. Only send clipboards that have changed since the last leave
 * 3. Use the sequence number from the most recent kMsgCEnter
 * 
 * @see kMsgCEnter, kMsgCClipboard
 * @since Protocol version 1.0
 */
extern const char *const kMsgCLeave;

/**
 * @brief Clipboard grab notification
 * 
 * **Message Code**: `"CCLP"`  
 * **Direction**: Primary ↔ Secondary  
 * **Format**: `"CCLP%1i%4i"`  
 * **Parameters**:
 * - `$1`: Clipboard identifier (1 byte)
 * - `$2`: Sequence number (4 bytes)
 * 
 * **Example**:
 * ```
 * "CCLP\x00\x00\x00\x00\x01"
 * // Primary clipboard grabbed, sequence 1
 * ```
 * 
 * Sent when an application grabs a clipboard on either screen.
 * This notifies the other screen that clipboard ownership has changed.
 * Secondary screens must use the sequence number from the most recent
 * kMsgCEnter. The primary always sends sequence number 0.
 * 
 * **Clipboard Identifiers**:
 * - `0`: Primary clipboard (Ctrl+C/Ctrl+V)
 * - `1`: Selection clipboard (middle-click on X11)
 * 
 * @see kMsgDClipboard
 * @since Protocol version 1.0
 */
extern const char *const kMsgCClipboard;

/**
 * @brief Screensaver state change
 * 
 * **Message Code**: `"CSEC"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"CSEC%1i"`  
 * **Parameters**:
 * - `$1`: Screensaver state (1 byte): 1 = started, 0 = stopped
 * 
 * **Example**:
 * ```
 * "CSEC\x01"  // Screensaver started
 * "CSEC\x00"  // Screensaver stopped
 * ```
 * 
 * Notifies the secondary screen when the primary's screensaver
 * starts or stops. The secondary can use this to synchronize
 * its own screensaver state.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgCScreenSaver;

/**
 * @brief Reset options command
 * 
 * **Message Code**: `"CROP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Instructs the client to reset all of its options to their default values.
 * This is typically sent when the server configuration changes.
 * 
 * @see kMsgDSetOptions
 * @since Protocol version 1.0
 */
extern const char *const kMsgCResetOptions;

/**
 * @brief Screen information acknowledgment
 * 
 * **Message Code**: `"CIAK"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Sent by the primary in response to a secondary screen's kMsgDInfo message.
 * This acknowledgment is sent for every kMsgDInfo, whether or not the
 * primary had previously sent a kMsgQInfo query.
 * 
 * @see kMsgDInfo, kMsgQInfo
 * @since Protocol version 1.0
 */
extern const char *const kMsgCInfoAck;

/**
 * @brief Keep-alive message
 * 
 * **Message Code**: `"CALV"`  
 * **Direction**: Primary ↔ Secondary  
 * **Format**: No parameters  
 * 
 * Sent periodically by the server to verify that connections are still
 * active. Clients must reply with the same message upon receipt.
 * 
 * **Timing**:
 * - Default interval: 3 seconds (configurable)
 * - Timeout: 3 missed messages trigger disconnection
 * 
 * **Behavior**:
 * - Server sends keep-alive to client
 * - Client immediately responds with keep-alive back to server
 * - If server doesn't receive response within timeout, it disconnects client
 * - If client doesn't receive keep-alives, it should disconnect from server
 * 
 * @see kKeepAliveRate, kKeepAlivesUntilDeath
 * @since Protocol version 1.3
 */
extern const char *const kMsgCKeepAlive;

/** @} */ // end of protocol_commands group

/**
 * @defgroup protocol_data Data Messages
 * @brief Input events, clipboard data, and file transfer messages
 * @{
 */

/**
 * @defgroup protocol_keyboard Keyboard Messages
 * @brief Keyboard input event messages
 * @{
 */

/**
 * @brief Key press with language code (v1.8+)
 * 
 * **Message Code**: `"DKDL"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKDL%2i%2i%2i%s"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * - `$3`: KeyButton (2 bytes) - Physical key code
 * - `$4`: Language code (string) - Keyboard language identifier
 * 
 * **Example**:
 * ```
 * "DKDL\x00\x61\x00\x00\x00\x1E\x00\x00\x00\x02en"
 * // 'a' key (0x61), no modifiers, physical key 0x1E, English
 * ```
 * 
 * Enhanced version of kMsgDKeyDown that includes language information
 * to help clients handle unknown language characters correctly.
 * 
 * @see kMsgDKeyDown
 * @since Protocol version 1.8
 */
extern const char *const kMsgDKeyDownLang;

/**
 * @brief Key press event
 * 
 * **Message Code**: `"DKDN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKDN%2i%2i%2i"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * - `$3`: KeyButton (2 bytes) - Physical key code
 * 
 * **Example**:
 * ```
 * "DKDN\x00\x61\x00\x00\x00\x1E"
 * // 'a' key (0x61), no modifiers, physical key 0x1E
 * ```
 * 
 * **Key Mapping Strategy**:
 * The KeyButton parameter is crucial for proper key release handling.
 * The secondary screen should:
 * 1. Map the KeyID to its local virtual key
 * 2. Remember the association between KeyButton and the local physical key
 * 3. Use KeyButton (not KeyID) to identify which key to release
 * 
 * This is necessary because:
 * - Dead keys can change the KeyID between press and release
 * - Different keyboard layouts may produce different KeyIDs
 * - Modifier keys released before the main key can alter KeyID
 * 
 * @see kMsgDKeyUp, kMsgDKeyDownLang
 * @since Protocol version 1.1
 */
extern const char *const kMsgDKeyDown;

/**
 * @brief Key press event (legacy v1.0)
 * 
 * **Message Code**: `"DKDN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKDN%2i%2i"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * 
 * Legacy version without KeyButton parameter. Used only when
 * communicating with protocol version 1.0 clients.
 * 
 * @deprecated Use kMsgDKeyDown for protocol version 1.1+
 * @see kMsgDKeyDown
 * @since Protocol version 1.0
 */
extern const char *const kMsgDKeyDown1_0;

/**
 * @brief Key auto-repeat event
 * 
 * **Message Code**: `"DKRP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKRP%2i%2i%2i%2i%s"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * - `$3`: Repeat count (2 bytes) - Number of repeats
 * - `$4`: KeyButton (2 bytes) - Physical key code
 * - `$5`: Language code (string) - Keyboard language identifier
 * 
 * **Example**:
 * ```
 * "DKRP\x00\x61\x00\x00\x00\x03\x00\x1E\x00\x00\x00\x02en"
 * // 'a' key repeating 3 times, English layout
 * ```
 * 
 * Sent when a key is held down and auto-repeating. The repeat count
 * indicates how many repeat events occurred since the last message.
 * 
 * @see kMsgDKeyDown
 * @since Protocol version 1.1
 */
extern const char *const kMsgDKeyRepeat;

/**
 * @brief Key auto-repeat event (legacy v1.0)
 * 
 * **Message Code**: `"DKRP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKRP%2i%2i%2i"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * - `$3`: Repeat count (2 bytes) - Number of repeats
 * 
 * Legacy version without KeyButton and language parameters.
 * 
 * @deprecated Use kMsgDKeyRepeat for protocol version 1.1+
 * @see kMsgDKeyRepeat
 * @since Protocol version 1.0
 */
extern const char *const kMsgDKeyRepeat1_0;

/**
 * @brief Key release event
 * 
 * **Message Code**: `"DKUP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKUP%2i%2i%2i"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * - `$3`: KeyButton (2 bytes) - Physical key code
 * 
 * **Example**:
 * ```
 * "DKUP\x00\x61\x00\x00\x00\x1E"
 * // Release 'a' key, physical key 0x1E
 * ```
 * 
 * **Important**: The secondary screen should use KeyButton (not KeyID)
 * to determine which physical key to release. This ensures correct
 * behavior with dead keys and layout differences.
 * 
 * @see kMsgDKeyDown
 * @since Protocol version 1.1
 */
extern const char *const kMsgDKeyUp;

/**
 * @brief Key release event (legacy v1.0)
 * 
 * **Message Code**: `"DKUP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DKUP%2i%2i"`  
 * **Parameters**:
 * - `$1`: KeyID (2 bytes) - Virtual key identifier
 * - `$2`: KeyModifierMask (2 bytes) - Active modifier keys
 * 
 * Legacy version without KeyButton parameter.
 * 
 * @deprecated Use kMsgDKeyUp for protocol version 1.1+
 * @see kMsgDKeyUp
 * @since Protocol version 1.0
 */
extern const char *const kMsgDKeyUp1_0;

/** @} */ // end of protocol_keyboard group

/**
 * @defgroup protocol_mouse Mouse Messages
 * @brief Mouse input event messages
 * @{
 */

/**
 * @brief Mouse button press event
 * 
 * **Message Code**: `"DMDN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMDN%1i"`  
 * **Parameters**:
 * - `$1`: ButtonID (1 byte) - Mouse button identifier
 * 
 * **Example**:
 * ```
 * "DMDN\x01"  // Left mouse button pressed
 * "DMDN\x02"  // Right mouse button pressed
 * "DMDN\x03"  // Middle mouse button pressed
 * ```
 * 
 * **Button IDs**:
 * - `1`: Left button
 * - `2`: Right button  
 * - `3`: Middle button
 * - `4+`: Additional buttons (side buttons, etc.)
 * 
 * @see kMsgDMouseUp
 * @since Protocol version 1.0
 */
extern const char *const kMsgDMouseDown;

/**
 * @brief Mouse button release event
 * 
 * **Message Code**: `"DMUP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMUP%1i"`  
 * **Parameters**:
 * - `$1`: ButtonID (1 byte) - Mouse button identifier
 * 
 * **Example**:
 * ```
 * "DMUP\x01"  // Left mouse button released
 * ```
 * 
 * Button IDs are the same as for kMsgDMouseDown.
 * 
 * @see kMsgDMouseDown
 * @since Protocol version 1.0
 */
extern const char *const kMsgDMouseUp;

/**
 * @brief Absolute mouse movement
 * 
 * **Message Code**: `"DMMV"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMMV%2i%2i"`  
 * **Parameters**:
 * - `$1`: X coordinate (2 bytes, signed) - Absolute screen position
 * - `$2`: Y coordinate (2 bytes, signed) - Absolute screen position
 * 
 * **Example**:
 * ```
 * "DMMV\x01\x90\x01\x2C"  // Move to (400, 300)
 * ```
 * 
 * Coordinates are absolute positions on the secondary screen.
 * The origin (0,0) is typically the top-left corner.
 * 
 * @see kMsgDMouseRelMove
 * @since Protocol version 1.0
 */
extern const char *const kMsgDMouseMove;

/**
 * @brief Relative mouse movement
 * 
 * **Message Code**: `"DMRM"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMRM%2i%2i"`  
 * **Parameters**:
 * - `$1`: X delta (2 bytes, signed) - Horizontal movement
 * - `$2`: Y delta (2 bytes, signed) - Vertical movement
 * 
 * **Example**:
 * ```
 * "DMRM\x00\x0A\xFF\xF6"  // Move right 10, up 10 pixels
 * ```
 * 
 * Relative movement is useful for:
 * - High-precision input devices
 * - Gaming applications
 * - When absolute positioning is not desired
 * 
 * @see kMsgDMouseMove
 * @since Protocol version 1.2
 */
extern const char *const kMsgDMouseRelMove;

/**
 * @brief Mouse wheel scroll event
 * 
 * **Message Code**: `"DMWM"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMWM%2i%2i"`  
 * **Parameters**:
 * - `$1`: X delta (2 bytes, signed) - Horizontal scroll
 * - `$2`: Y delta (2 bytes, signed) - Vertical scroll
 * 
 * **Example**:
 * ```
 * "DMWM\x00\x00\x00\x78"  // Scroll up one tick (+120)
 * "DMWM\x00\x00\xFF\x88"  // Scroll down one tick (-120)
 * "DMWM\x00\x78\x00\x00"  // Scroll right one tick (+120)
 * ```
 * 
 * **Scroll Values**:
 * - `+120`: One tick forward (away from user) or right
 * - `-120`: One tick backward (toward user) or left
 * - Values are typically multiples of 120
 * 
 * **Directions**:
 * - **Vertical**: Positive = up/away, Negative = down/toward
 * - **Horizontal**: Positive = right, Negative = left
 * 
 * @see kMsgDMouseWheel1_0
 * @since Protocol version 1.3
 */
extern const char *const kMsgDMouseWheel;

/**
 * @brief Mouse wheel scroll event (legacy v1.0-1.2)
 * 
 * **Message Code**: `"DMWM"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DMWM%2i"`  
 * **Parameters**:
 * - `$1`: Y delta (2 bytes, signed) - Vertical scroll only
 * 
 * Legacy version that only supports vertical scrolling.
 * Used when communicating with protocol versions 1.0-1.2.
 * 
 * @deprecated Use kMsgDMouseWheel for protocol version 1.3+
 * @see kMsgDMouseWheel
 * @since Protocol version 1.0
 */
extern const char *const kMsgDMouseWheel1_0;

/** @} */ // end of protocol_mouse group

/**
 * @defgroup protocol_clipboard Clipboard Messages
 * @brief Clipboard data synchronization messages
 * @{
 */

/**
 * @brief Clipboard data transfer
 * 
 * **Message Code**: `"DCLP"`  
 * **Direction**: Primary ↔ Secondary  
 * **Format**: `"DCLP%1i%4i%1i%s"`  
 * **Parameters**:
 * - `$1`: Clipboard identifier (1 byte)
 * - `$2`: Sequence number (4 bytes)
 * - `$3`: Mark/flags (1 byte) - For streaming support (v1.6+)
 * - `$4`: Clipboard data (string)
 * 
 * **Example**:
 * ```
 * "DCLP\x00\x00\x00\x00\x01\x00\x00\x00\x00\x0BHello World"
 * // Primary clipboard, sequence 1, no flags, text "Hello World"
 * ```
 * 
 * **Clipboard Identifiers**:
 * - `0`: Primary clipboard (Ctrl+C/Ctrl+V)
 * - `1`: Selection clipboard (middle-click on X11)
 * 
 * **Sequence Numbers**:
 * - Primary always sends sequence number 0
 * - Secondary uses sequence number from most recent kMsgCEnter
 * 
 * **Streaming (v1.6+)**:
 * For large clipboard data, the mark byte enables chunked transfer:
 * - `0`: Single chunk (complete data)
 * - `1`: First chunk of multi-chunk transfer
 * - `2`: Middle chunk
 * - `3`: Final chunk
 * 
 * @see kMsgCClipboard
 * @since Protocol version 1.0
 */
extern const char *const kMsgDClipboard;

/** @} */ // end of protocol_clipboard group

/**
 * @defgroup protocol_info Information Messages
 * @brief Screen information and configuration messages
 * @{
 */

/**
 * @brief Client screen information
 * 
 * **Message Code**: `"DINF"`  
 * **Direction**: Secondary → Primary  
 * **Format**: `"DINF%2i%2i%2i%2i%2i%2i%2i"`  
 * **Parameters**:
 * - `$1`: Left edge coordinate (2 bytes, signed)
 * - `$2`: Top edge coordinate (2 bytes, signed)
 * - `$3`: Screen width in pixels (2 bytes, unsigned)
 * - `$4`: Screen height in pixels (2 bytes, unsigned)
 * - `$5`: Warp zone size (2 bytes, obsolete)
 * - `$6`: Mouse X position (2 bytes, signed)
 * - `$7`: Mouse Y position (2 bytes, signed)
 * 
 * **Example**:
 * ```
 * "DINF\x00\x00\x00\x00\x07\x80\x04\x38\x00\x00\x01\x90\x01\x2C"
 * // Screen at (0,0), 1920x1080, mouse at (400,300)
 * ```
 * 
 * **When to Send**:
 * 1. In response to kMsgQInfo query
 * 2. When screen resolution changes
 * 3. During initial connection setup
 * 
 * **Resolution Change Protocol**:
 * When sending due to resolution change, the secondary should:
 * 1. Send kMsgDInfo with new dimensions
 * 2. Ignore kMsgDMouseMove until receiving kMsgCInfoAck
 * 3. This prevents mouse movement outside the new screen area
 * 
 * @see kMsgQInfo, kMsgCInfoAck
 * @since Protocol version 1.0
 */
extern const char *const kMsgDInfo;

/**
 * @brief Set client options
 * 
 * **Message Code**: `"DSOP"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"DSOP%4I"`  
 * **Parameters**:
 * - `$1`: Option/value pairs (4-byte integer list)
 * 
 * **Example**:
 * ```
 * "DSOP\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x00"
 * // 2 pairs: option 1 = value 1, option 2 = value 0
 * ```
 * 
 * Sets configuration options on the client. Options are sent as
 * alternating option ID and value pairs. The client should apply
 * these settings to modify its behavior.
 * 
 * @see kMsgCResetOptions
 * @since Protocol version 1.0
 */
extern const char *const kMsgDSetOptions;

/** @} */ // end of protocol_info group

/**
 * @defgroup protocol_files File Transfer Messages
 * @brief File transfer and drag-and-drop messages (v1.5+)
 * @{
 */

/**
 * @brief File transfer data
 * 
 * **Message Code**: `"DFTR"`  
 * **Direction**: Primary ↔ Secondary  
 * **Format**: `"DFTR%1i%s"`  
 * **Parameters**:
 * - `$1`: Transfer mark (1 byte) - Transfer state
 * - `$2`: Data (string) - Content depends on mark
 * 
 * **Transfer Marks**:
 * - `1` (kDataStart): Data contains file size (8 bytes)
 * - `2` (kDataChunk): Data contains file content chunk
 * - `3` (kDataEnd): Transfer complete (data may be empty)
 * 
 * **Example Transfer Sequence**:
 * ```
 * "DFTR\x01\x00\x00\x00\x00\x00\x00\x10\x00"  // Start: 4096 bytes
 * "DFTR\x02[1024 bytes of file data]"          // Chunk: data
 * "DFTR\x02[1024 bytes of file data]"          // Chunk: data
 * "DFTR\x02[1024 bytes of file data]"          // Chunk: data
 * "DFTR\x02[1024 bytes of file data]"          // Chunk: data
 * "DFTR\x03"                                   // End: complete
 * ```
 * 
 * **Protocol Flow**:
 * 1. Sender initiates with kDataStart containing total file size
 * 2. Sender sends multiple kDataChunk messages with file content
 * 3. Sender concludes with kDataEnd to signal completion
 * 4. Receiver can abort by closing connection
 * 
 * @see kMsgDDragInfo, EDataTransfer
 * @since Protocol version 1.5
 */
extern const char *const kMsgDFileTransfer;

/**
 * @brief Drag and drop information
 * 
 * **Message Code**: `"DDRG"`  
 * **Direction**: Primary ↔ Secondary  
 * **Format**: `"DDRG%2i%s"`  
 * **Parameters**:
 * - `$1`: Number of files (2 bytes)
 * - `$2`: File paths (string) - Null-separated file paths
 * 
 * **Example**:
 * ```
 * "DDRG\x00\x02/path/to/file1.txt\x00/path/to/file2.txt\x00"
 * // Dragging 2 files
 * ```
 * 
 * Sent when a drag-and-drop operation begins. Contains the list
 * of files being dragged. The actual file transfer follows using
 * kMsgDFileTransfer messages.
 * 
 * **File Path Format**:
 * - Paths are null-terminated strings
 * - Multiple paths are concatenated with null separators
 * - Paths should use forward slashes for compatibility
 * 
 * @see kMsgDFileTransfer
 * @since Protocol version 1.5
 */
extern const char *const kMsgDDragInfo;

/** @} */ // end of protocol_files group

/**
 * @defgroup protocol_system System Messages
 * @brief System-level notifications and synchronization
 * @{
 */

/**
 * @brief Secure input notification (macOS)
 * 
 * **Message Code**: `"SECN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"SECN%s"`  
 * **Parameters**:
 * - `$1`: Application name (string) - App requesting secure input
 * 
 * **Example**:
 * ```
 * "SECN\x00\x00\x00\x08Terminal"
 * // Terminal app is requesting secure input
 * ```
 * 
 * Notifies the secondary screen when an application on the primary
 * requests secure input mode. This is primarily a macOS feature
 * where certain applications (like password fields) can request
 * exclusive keyboard access.
 * 
 * The secondary can use this information to:
 * - Display security warnings to the user
 * - Temporarily disable input forwarding
 * - Show which application is requesting secure input
 * 
 * @since Protocol version 1.7
 */
extern const char *const kMsgDSecureInputNotification;

/**
 * @brief Language synchronization
 * 
 * **Message Code**: `"LSYN"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"LSYN%s"`  
 * **Parameters**:
 * - `$1`: Language list (string) - Available server languages
 * 
 * **Example**:
 * ```
 * "LSYN\x00\x00\x00\x0Ben,fr,de,es"
 * // Server supports English, French, German, Spanish
 * ```
 * 
 * Synchronizes keyboard language/layout information between
 * primary and secondary screens. Helps ensure proper character
 * mapping when different keyboard layouts are used.
 * 
 * **Language Format**:
 * - Comma-separated list of language codes
 * - Uses standard ISO 639-1 language codes
 * - Primary language listed first
 * 
 * @since Protocol version 1.8
 */
extern const char *const kMsgDLanguageSynchronisation;

/** @} */ // end of protocol_system group

/** @} */ // end of protocol_data group

/**
 * @defgroup protocol_queries Query Messages
 * @brief Information request messages
 * @{
 */

/**
 * @brief Query screen information
 * 
 * **Message Code**: `"QINF"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Requests the secondary screen to send its current screen information.
 * The client should respond with a kMsgDInfo message containing:
 * - Screen dimensions and position
 * - Current mouse position
 * - Other screen-related data
 * 
 * This is typically sent:
 * - During initial connection setup
 * - When the server needs updated screen information
 * - After configuration changes
 * 
 * @see kMsgDInfo, kMsgCInfoAck
 * @since Protocol version 1.0
 */
extern const char *const kMsgQInfo;

/** @} */ // end of protocol_queries group

/**
 * @defgroup protocol_errors Error Messages
 * @brief Protocol error and failure notifications
 * @{
 */

/**
 * @brief Incompatible protocol versions
 * 
 * **Message Code**: `"EICV"`  
 * **Direction**: Primary → Secondary  
 * **Format**: `"EICV%2i%2i"`  
 * **Parameters**:
 * - `$1`: Primary major version (2 bytes)
 * - `$2`: Primary minor version (2 bytes)
 * 
 * **Example**:
 * ```
 * "EICV\x00\x01\x00\x08"  // Server is version 1.8
 * ```
 * 
 * Sent when the client and server have incompatible protocol versions.
 * This typically occurs when:
 * - Major versions differ (fundamental incompatibility)
 * - Server requires newer features than client supports
 * 
 * After sending this message, the server will disconnect the client.
 * The client should display an appropriate error message to the user.
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgEIncompatible;

/**
 * @brief Client name already in use
 * 
 * **Message Code**: `"EBSY"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Sent when the client name provided during connection is already
 * in use by another connected client. Client names must be unique
 * within a Deskflow network.
 * 
 * After receiving this message, the client should:
 * 1. Disconnect from the server
 * 2. Inform the user of the name conflict
 * 3. Allow the user to choose a different name
 * 4. Retry connection with the new name
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgEBusy;

/**
 * @brief Unknown client name
 * 
 * **Message Code**: `"EUNK"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Sent when the client name provided during connection is not
 * found in the server's screen configuration map. This means
 * the server doesn't know about this client.
 * 
 * This can happen when:
 * - Client name is misspelled
 * - Server configuration doesn't include this client
 * - Client is connecting to wrong server
 * 
 * The client should inform the user and check:
 * - Client name spelling
 * - Server configuration
 * - Network connectivity to correct server
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgEUnknown;

/**
 * @brief Protocol violation
 * 
 * **Message Code**: `"EBAD"`  
 * **Direction**: Primary → Secondary  
 * **Format**: No parameters  
 * 
 * Sent when the client violates the protocol in some way.
 * This can include:
 * - Sending malformed messages
 * - Sending messages in wrong order
 * - Sending unexpected message types
 * - Exceeding protocol limits
 * 
 * After sending this message, the server will immediately
 * disconnect the client. This indicates a serious protocol
 * error that cannot be recovered from.
 * 
 * **Common Causes**:
 * - Implementation bugs in client
 * - Corrupted network data
 * - Version mismatch not caught earlier
 * - Malicious or broken client
 * 
 * @since Protocol version 1.0
 */
extern const char *const kMsgEBad;

/** @} */ // end of protocol_errors group

/** @} */ // end of protocol_messages group

/**
 * @defgroup protocol_structures Protocol Data Structures
 * @brief Data structures used in protocol messages
 * @{
 */

/**
 * @brief Client screen information structure
 * 
 * Contains comprehensive information about a secondary screen,
 * including dimensions, position, and current mouse location.
 * This data is sent via kMsgDInfo messages.
 * 
 * **Usage**:
 * - Sent by client in response to kMsgQInfo
 * - Sent when screen resolution changes
 * - Used by server for screen layout calculations
 * 
 * **Coordinate System**:
 * - Origin (0,0) is typically top-left corner
 * - Coordinates can be negative for multi-monitor setups
 * - All values are in pixels
 * 
 * @see kMsgDInfo, kMsgQInfo
 * @since Protocol version 1.0
 */
class ClientInfo
{
public:
  /**
   * @brief Screen position coordinates
   * 
   * The position of the upper-left corner of the screen in the
   * virtual desktop coordinate system. This is typically (0,0)
   * for single-monitor setups, but can be different in multi-monitor
   * configurations.
   * 
   * @since Protocol version 1.0
   */
  int32_t m_x;  ///< Left edge X coordinate
  int32_t m_y;  ///< Top edge Y coordinate

  /**
   * @brief Screen dimensions
   * 
   * The size of the screen in pixels. These values represent
   * the usable screen area for mouse movement and window placement.
   * 
   * @since Protocol version 1.0
   */
  int32_t m_w;  ///< Screen width in pixels
  int32_t m_h;  ///< Screen height in pixels

  /**
   * @brief Obsolete jump zone size
   * 
   * @deprecated This field is no longer used and should be set to 0
   * @since Protocol version 1.0
   */
  int32_t obsolete1;

  /**
   * @brief Current mouse position
   * 
   * The current location of the mouse cursor on this screen.
   * Coordinates are relative to the screen's coordinate system
   * (m_x, m_y represent the origin).
   * 
   * **Usage**:
   * - Updated when mouse moves on this screen
   * - Used for cursor synchronization between screens
   * - Helps with smooth transitions during screen switching
   * 
   * @since Protocol version 1.0
   */
  int32_t m_mx;  ///< Mouse X position
  int32_t m_my;  ///< Mouse Y position
};

/** @} */ // end of protocol_structures group
