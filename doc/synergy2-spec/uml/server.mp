input metauml;

beginfig(1);
  Class.Server("Server")()();
  Class.Keyboard("Keyboard")()();
  Class.Mouse("Mouse")()();
  Class.Screen("Screen")()();

  Screen.n = Server.s + (-50, -20);
  Keyboard.n = Server.s + (0, -20);
  Mouse.n = Server.s + (+50, -20);
  
  drawObjects(Server, Keyboard, Mouse, Screen);
  
  link(inheritance)(Screen.n -- Server.s);
  link(inheritance)(Mouse.n -- Server.s);
  link(inheritance)(Keyboard.n -- Server.s);
endfig;
