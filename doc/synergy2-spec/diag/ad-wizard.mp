input metauml;

beginfig(1);

  Begin.b;

  Activity.install_a("User installs Synergy", "on first node");
  Activity.set_master("User chooses the", "'master' option");
  Activity.master_wait("Master waits for", "nodes to announce");
  
  Activity.install_b("User installs Synergy", "on another node");
  Activity.set_slave("User chooses the", "'slave' option");
  Activity.announce("Slave announces", "continuously until connect");
  
  Activity.connect("Master connects", "a new node");
  Activity.screen_edge("Tell user to move", "mouse to edge of screen");
  Activity.auto_place("New node is placed", "depending on edge hit");
  Activity.custom_place("Allow user to", "reposition new node");
  Activity.more_nodes("Does the user have", "more nodes to add?");
  Activity.save_config("Config is saved", "to all nodes");

  End.e;

  Group.master(install_a, set_master, master_wait);
  topToBottom(20)(
      install_a, set_master, master_wait);

  Group.slave(install_b, set_slave, announce);
  topToBottom(20)(
      install_b, set_slave, announce);

  Group.master_slave(master, slave);
  leftToRight(20)(master, slave);

  topToBottom(20)(b, master_slave);
  
  topToBottom(20)(
      master_slave, connect, screen_edge, auto_place,
      custom_place, more_nodes, save_config, e);

  drawObjects(
      b, master_slave, connect, screen_edge, auto_place, custom_place,
      more_nodes, save_config, e);

  % master
  clink(transition)(b, install_a);
  clink(transition)(install_a, set_master);
  clink(transition)(set_master, master_wait);
  clink(transition)(master_wait, connect);

  % slave
  clink(transition)(b, install_b);
  clink(transition)(install_b, set_slave);
  clink(transition)(set_slave, announce);
  clink(transition)(announce, connect);

  % designer
  clink(transition)(connect, screen_edge);
  clink(transition)(screen_edge, auto_place);
  clink(transition)(auto_place, custom_place);
  clink(transition)(custom_place, more_nodes);
  clink(transition)(more_nodes, save_config);
  link(transition)(pathStepX(more_nodes.e, install_b.e, 90));
  clink(transition)(save_config, e);
  
  %labels
  item(iGuard)("no")(obj.nw = more_nodes.s + (+3, -3));
  item(iGuard)("yes")(obj.sw = more_nodes.e + (+3, +3));
  
endfig;

end;