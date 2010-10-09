input metauml;

beginfig(1);
  
  Class.Server("Server")(
      "-screen:Screen", "-devices:vector<Device>")();
  
  Class.Screen("Screen")(
      "-virtualScreens:vector<VirtualScreen>")();

  Class.Device("Device")()();
  
  Class.Mouse("Mouse")(
      "-axes::map<char, int32>", "-buttons:map<char, int32>")();

  Class.Keyboard("Keyboard")(
      "-layout:KeyboardLayout")();

  Class.VirtualScreen("VirtualScreen")(
      "-width:int32", "-height:int32", "-x:int32", "-y:int32")();

  Group.ServerAssoc(Screen, Device);
  leftToRight(40)(Screen, Device);

  Group.Devices(Mouse, Keyboard);
  leftToRight(20)(Mouse, Keyboard);

  topToBottom(20)(Server, ServerAssoc);
  topToBottom(40)(Device, Devices);
  topToBottom.left(20)(Screen, VirtualScreen);
  
  drawObjects(Server, ServerAssoc, Devices, VirtualScreen);
  
  clink(associationUni)(Server, Screen);
  clink(associationUni)(Server, Device);
  clink(associationUni)(Screen, VirtualScreen);

  clink(inheritance)(Keyboard, Device);
  clink(inheritance)(Mouse, Device);
  
endfig;

end;
