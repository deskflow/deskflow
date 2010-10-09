input metauml;

beginfig(1);
  
  Begin.b;
  Activity.dev_change("Developer", "makes code change");
  Activity.dev_ut("Developer", "runs unit tests");
  Branch.ut_pass;
  Activity.dev_commit("Developer", "commits to SCM");
  Activity.bm_update("Build machines", "update from SCM");
  Activity.bm_it("Build machines", "run integration tests");
  Branch.it_pass;
  Activity.dev_beer("Developer", "has a beer");
  End.e;
  
  leftToRight(20)(b, dev_change, dev_ut, ut_pass);
  topToBottom(20)(dev_ut, dev_commit, bm_update);
  leftToRight(20)(it_pass, bm_it, bm_update);
  topToBottom(20)(bm_it, dev_beer, e);
  
  drawObjects(
      b, dev_change, dev_ut, ut_pass, dev_commit,
      bm_update, bm_it, it_pass, dev_beer, e);

  clink(transition)(b, dev_change);
  clink(transition)(dev_change, dev_ut);
  clink(transition)(dev_ut, ut_pass);
  link(transition)(pathStepY(ut_pass.n, dev_change.n, 30));
  link(transition)(pathStepX(ut_pass.s, dev_commit.e, 0));
  clink(transition)(dev_commit, bm_update);
  clink(transition)(bm_update, bm_it);
  clink(transition)(bm_it, it_pass);
  link(transition)(pathStepY(it_pass.n, dev_change.s, 30));
  link(transition)(pathStepX(it_pass.s, dev_beer.w, 0));
  clink(transition)(dev_beer, e);

  % label the branch lines (unit test)
  item(iGuard)("pass")(obj.w = ut_pass.e + (-4, -25));
  item(iGuard)("fail")(obj.s = ut_pass.e + (-100, 36));

  % label the branch lines (integration test)
  item(iGuard)("pass")(obj.e = it_pass.e + (20, -43));
  item(iGuard)("fail")(obj.s = it_pass.e + (40, 36));
  
endfig;

end;
