**Deskflow** is a free and open source keyboard and mouse sharing app.
Use the keyboard, mouse, or trackpad of one computer to control nearby computers,
and work seamlessly between them.

Deskflow acts as a software KVM (without video) that allows you to:
- Share keyboard and mouse input across multiple computers
- Synchronize clipboard content between machines
- Work seamlessly across different operating systems (Windows, macOS, Linux, BSD)

Deskflow software consists of a **server** (primary computer) that shares its input devices and **clients** (secondary computers) that receive and execute the input commands over a TCP network connection.

### Architecture Overview

Deskflow is built with a modular, cross-platform architecture:

```
┌─────────────────┐    Network Protocol    ┌─────────────────┐
│   Server App    │◄──────────────────────►│  Client App     │
│                 │     (Port 24800)       │   (Windows)     │
│ ┌─────────────┐ │                        │ ┌─────────────┐ │
│ │   Screen    │ │                        │ │   Screen    │ │
│ │  Platform   │ │                        │ │  Platform   │ │
│ │   Layer     │ │                        │ │   Layer     │ │
│ └─────────────┘ │                        │ └─────────────┘ │
└─────────────────┘                        └─────────────────┘
┌───────┐ ┌───────┐
│ Keyb. │ │ Mouse │
└───────┘ └───────┘

                                           ┌─────────────────┐
                                           │  Client App     │
                                           │    (macOS)      │
                                           │ ┌─────────────┐ │
                                           │ │   Screen    │ │
                                           │ │  Platform   │ │
                                           │ │   Layer     │ │
                                           │ └─────────────┘ │
                                           └─────────────────┘

                                           ┌─────────────────┐
                                           │  Client App     │
                                           │   (Custom)      │
                                           │ ┌─────────────┐ │
                                           │ │   Screen    │ │
                                           │ │  Platform   │ │
                                           │ │   Layer     │ │
                                           │ └─────────────┘ │
                                           └─────────────────┘
```

### More info

For more info, see our [Wiki](https://github.com/deskflow/deskflow/wiki).

Check out our [Building guide](build.md) or our general @ref contributing_guide "Contributing section". We also have a detailed [Protocol Reference](protocol_reference.md).
