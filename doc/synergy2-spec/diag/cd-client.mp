input metauml;

beginfig(1);

  Class.System("System")()();
  
  Class.Client("Client")(
      "-relays:vector<DeviceRelay>")(
      "+connect()");

  Class.DeviceRelay("DeviceRelay")()(
      "+relayMessage()");

  Class.MouseRelay("MouseRelay")()(
      "+setDelta()",
      "+buttonDown()",
      "+buttonUp()");

  Class.KeyboardRelay("KeyboardRelay")()(
      "+keyDown()",
      "+keyUp()");

  Group.MouseKeyboard(MouseRelay, KeyboardRelay);
  leftToRight.top(20)(MouseRelay, KeyboardRelay);
  
  topToBottom(20)(System, Client, DeviceRelay, MouseKeyboard);
  
  drawObjects(System, Client, DeviceRelay, MouseKeyboard);

  clink(inheritance)(Client, System);
  clink(inheritance)(MouseRelay, DeviceRelay);
  clink(inheritance)(KeyboardRelay, DeviceRelay);

  clink(associationUni)(Client, DeviceRelay);
  
endfig;

end;
