input metauml;

beginfig(1);

  Begin.b;
  Activity.eat("Eat something good", "from the kitchen");
  Branch.enough;
  Fork.fork("h", 50);
  Activity.read("Read a book");
  Activity.listen("Listen to music", "(and ignore it)");
  Fork.join("h", 50);
  End.e;

  leftToRight.top(10)(read, listen);
  Group.readListen(read, listen);

  leftToRight(30)(b, eat);
  topToBottom(20)(eat, enough, fork, readListen, join, e);

  drawObjects(b, eat, enough, fork, readListen, join, e);

  clink(transition)(b, eat);
  clink(transition)(eat, enough);
  link(transition)(pathStepX(enough.e, eat.e, 80));
  clink(transition)(enough, fork);
  clink(transition)(fork, read);
  clink(transition)(fork, listen);
  clink(transition)(read, join);
  clink(transition)(listen, join);
  clink(transition)(join, e);

  item(iGuard)("still hungry")(obj.sw = enough.e + (20, 0));
  item(iGuard)("had enough")(obj.nw = enough.s + (0, -4));

endfig;

end;
