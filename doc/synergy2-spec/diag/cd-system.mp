input metauml;

beginfig(1);
  
  Class.System("System")(
      "-screen:Screen")();

  Class.Server("Server")()();

  Class.Client("Client")()();
  
  Class.Screen("Screen")(
      "-virtualScreens:vector<VirtualScreen>")();

  Class.VirtualScreen("VirtualScreen")(
      "-width:int32", "-height:int32", "-x:int32", "-y:int32")();

  Group.ServerClient(Server, Client);
  leftToRight(20)(Server, Client);
  
  topToBottom(20)(ServerClient, System, Screen, VirtualScreen);
  
  drawObjects(ServerClient, System, Screen, VirtualScreen);

  clink(inheritance)(Server, System);
  clink(inheritance)(Client, System);
  clink(associationUni)(Screen, VirtualScreen);
  clink(associationUni)(System, Screen);
  
endfig;

end;
