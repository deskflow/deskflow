input metauml;

beginfig(1);

  Class.System("System")()();
  
  Class.Server("Server")(
      "-devices:vector<Device>")(
      "+listen()");
  
  Class.Device("Device")()();
  
  Class.Mouse("Mouse")(
      "-axes::map<char, int32>", "-buttons:map<char, int32>")();

  Class.Keyboard("Keyboard")(
      "-layout:KeyboardLayout")();
  
  Group.Devices(Mouse, Keyboard);
  leftToRight.top(20)(Mouse, Keyboard);

  topToBottom(20)(System, Server, Device);
  topToBottom(20)(Device, Devices);
  
  drawObjects(System, Server, Device, Devices);

  clink(inheritance)(Server, System);
  clink(inheritance)(Keyboard, Device);
  clink(inheritance)(Mouse, Device);

  clink(associationUni)(Server, Device);
  
endfig;

end;
