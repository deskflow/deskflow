input metauml;

beginfig(1);
  
  Begin.b;
  Activity.dev_change("Developer", "makes code change");
%  Activity.dev_ut("Developer", "runs unit tests");
%  Branch.ut_pass;
%  Activity.dev_commit("Developer", "commits to SCM");
%  Activity.bm_update("Build machines", "update from SCM");
%  Activity.bm_it("Build machines", "run integration tests");
%  Branch.it_pass;
%  Activity.dev_beer("Developer", "has a beer");
  End.e;

  leftToRight.top(10)(b, dev_change, e);
  drawObjects(b, dev_change, e);
  

%  leftToRight.top(10)(bm_update, bm_it);
%  Group.bm(bm_update, bm_it);

%  leftToRight(30)(b, dev_change, dev_ut);
%  topToBottom(20)(b, dev_change, dev_commit, ut_pass, bm, it_pass, dev_beer);

%  drawObjects(
%      b, dev_change, dev_ut, ut_pass, dev_commit, bm_update, bm_it,
%      it_pass, dev_beer);

  clink(transition)(b, dev_change);
  clink(transition)(dev_change, e);
  
%  clink(b, dev_change);
%  clink(transition)(dev_change, ut_pass);
%  clink(transition)(ut_pass, dev_commit);
%  clink(transition)(dev_commit, bm_update);
%  clink(transition)(bm_update, bm_it);
%  clink(transition)(bm_it, it_pass);
%  clink(transition)(it_pass, dev_beer);
%  clink(transition)(dev_beer, e);

%  item(iGaurd)("fail")(dev_change.s = ut_pass.w);
%  item(iGaurd)("fail")(dev_change.s = it_pass.n);
  
endfig;
