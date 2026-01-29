void handleDeactivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, const GVariant *options);
  void handleZonesChanged(XdpInputCaptureSession *session, const GVariant *options);

  /// g_signal_connect callback wrapper
  static void sessionClosed(XdpSession *session, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSessionClosed(session);
  }
  static void disabled(const XdpInputCaptureSession *session, const GVariant *options, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleDisabled(session, options);
  }
  static void activated(
      const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options, const gpointer data
  )
  {
    static_cast<PortalInputCapture *>(data)->handleActivated(session, activationId, options);
  }
  static void deactivated(
      const XdpInputCaptureSession *session, const std::uint32_t activationId, const GVariant *options,
      const gpointer data
  )
  {
    static_cast<PortalInputCapture *>(data)->handleDeactivated(session, activationId, options);
  }
  static void zonesChanged(XdpInputCaptureSession *session, const GVariant *options, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleZonesChanged(session, options);
  }

  // Persistent dialog handling
  void persistDialog();
  void restoreDialog();

private:
  enum class Signal : uint8_t
  {
    SessionClosed,
    Disabled,
    Activated,
    Deactivated,
    ZonesChanged
  };

  // Dialog persistence state
  bool m_dialogPersistent = false;
  char *m_dialogToken = nullptr;