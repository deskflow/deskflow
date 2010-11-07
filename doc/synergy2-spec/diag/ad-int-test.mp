input metauml;

beginfig(1);
  
  Begin.b;
  
  Activity.s_start_th("Test harness", "start");
  Activity.s_start("Synergy", "start server");
  Activity.s_listen("Synergy", "listen()");
  Activity.s_mouse_move("Test harness", "moves mouse", "off screen");
  Activity.s_mouse_event("Synergy", "onMouseMove()");
  
  Activity.c_start_th("Test harness", "start");
  Activity.c_start("Synergy", "start client");
  Activity.c_connect("Synergy", "connect()");
  Activity.c_wait("Test harness", "wait for mouse", "move (10ms)");
  Activity.c_move_ok("Cursor has moved?");
  Activity.c_wait_loop("Loop (10)");
  Activity.c_fail("Fail");
  Activity.c_coords_ok("Is cursor at", "expected coods?");
  Activity.c_pass("Pass");

  Fork.sync_a("v", 30);

  End.e;

  Group.c_loop_move(c_wait_loop, c_move_ok);
  leftToRight(40)(c_wait_loop, c_move_ok);

  Group.c_fail_coords(c_fail, c_coords_ok);
  leftToRight(40)(c_fail, c_coords_ok);
  
  topToBottom(30)(b, sync_a);
  leftToRight(50)(s_start_th, sync_a, c_start_th);
  topToBottom(30)(
      s_start_th, s_start, s_listen, s_mouse_move, s_mouse_event);
  topToBottom(30)(
      c_start_th, c_start, c_connect, c_wait,
      c_loop_move, c_fail_coords, c_pass);
  leftToRight(30)(e, c_pass);

  drawObjects(
      b, sync_a, s_start_th, s_start, s_listen, s_mouse_move,
      s_mouse_event, c_start_th, c_start, c_connect, c_wait,
      c_loop_move, c_fail_coords, c_pass, e);

  % server links
  clink(transition)(b, s_start_th);
  clink(transition)(s_start_th, sync_a);
  clink(transition)(sync_a, s_start);
  clink(transition)(s_start, s_listen);
  clink(transition)(s_listen, s_mouse_move);
  clink(transition)(s_mouse_move, s_mouse_event);
  clink(transition)(s_mouse_event, e);

  % client links
  clink(transition)(b, c_start_th);
  clink(transition)(c_start_th, sync_a);
  clink(transition)(sync_a, c_start);
  clink(transition)(c_start, c_connect);
  clink(transition)(c_connect, c_wait);
  clink(transition)(c_wait, c_move_ok);
  clink(transition)(c_move_ok, c_wait_loop);
  clink(transition)(c_wait_loop, c_wait);
  clink(transition)(c_move_ok, c_coords_ok);
  clink(transition)(c_coords_ok, c_pass);
  clink(transition)(c_coords_ok, c_fail);
  clink(transition)(c_pass, e);
  clink(transition)(c_wait_loop, c_fail);
  clink(transition)(c_fail, e);

  % labels
  item(iGuard)("sync")(obj.n = sync_a.s + (0, -5));
  item(iGuard)("yes")(obj.s = c_coords_ok.n + (-3, 15));
  item(iGuard)("no")(obj.e = c_move_ok.w + (-7, -7));
  item(iGuard)("yes")(obj.s = c_pass.n + (5, 10));
  item(iGuard)("no")(obj.e = c_coords_ok.w + (-7, -7));
  item(iGuard)("timeout")(obj.w = c_wait_loop.s + (5, -10));
  
endfig;

end;
