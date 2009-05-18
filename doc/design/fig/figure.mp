input rboxes;
input graph;
input esk;

%prologues:=1;

vardef dcuta(suffix a,b) expr p =
  drawarrow p cutbefore bpath.a cutafter bpath.b;
enddef; 

vardef cuta(suffix a,b) expr p =
  drawarrow p cutbefore bpath.a cutafter bpath.b;
  point .5*length p of p
enddef; 



%%
%% Mapping DB memory allocator
%%
beginfig(1);
  u := 3mm;

  circleit.b1(btex $b$ etex);
  circleit.b2(btex $b$ etex);
  circleit.b3(btex $b$ etex);
  circleit.b4(btex $b$ etex);
  circleit.b5(btex $b$ etex);
  circleit.b6(btex $b$ etex);
  circleit.b7(btex $b$ etex);
  boxit.pfl1(btex \vbox{\hbox{freelist ptr.}
    \hbox{num free = 7}}etex);
  boxit.fr1(btex  etex);

  circleit.c1(btex $b$ etex);
  circleit.c2(btex $b$ etex);
  circleit.c3(btex $b$ etex);
  boxit.pfl2(btex \vbox{\hbox{freelist ptr.}
    \hbox{num free = 3}} etex);
  boxit.fr2(btex  etex);
  eqsize(fr1,fr2);

  boxit.ptr(btex list-list pointer etex);
  
  eqsize(b1,b2,b3,b4,b5,b6,b7,c1,c2,c3);
  halign(pfl1,b1,b2,b3,b4,b5,b6,b7);
  halign(pfl2,c1,c2,c3);
  xspace(pfl1,b1,b2,b3,b4,b5,b6,b7) = xspace(pfl2,c1,c2,c3) = u;

  xpart(pfl1.w-fr1.w) = ypart(fr1.n-pfl1.n) = ypart(pfl1.s-fr1.s) =
  xpart(pfl2.w-fr2.w) = ypart(fr2.n-pfl2.n) = u;
  xpart(fr1.e-b7.e) = 2u;
  valign.e(fr1,fr2);

  yspace(fr2,fr1) = 2u;
  ptr.c = pfl1.n + (-3u,5u);

  fixsize(pfl1,pfl2,ptr,b1,b2,b3,b4,b5,b6,b7,c1,c2,c3,fr1,fr2);
  dropshadow(fr1,fr2);
  drawunboxed(pfl1,pfl2,ptr); 
  drawboxed(b1,b2,b3,b4,b5,b6,b7,c1,c2,c3,fr1,fr2);

  dcuta(pfl1,b1) pfl1.c{dir80}..{dir300}b1.c;
  dcuta(b1,b2)   b1.c{up}..{down}b2.c;
  dcuta(b2,b3)   b2.c{up}..{down}b3.c;
  dcuta(b3,b4)   b3.c{up}..{down}b4.c;
  dcuta(b4,b5)   b4.c{up}..{down}b5.c;
  dcuta(b5,b6)   b5.c{up}..{down}b6.c;
  dcuta(b6,b7)   b6.c{up}..{down}b7.c;
  drawterm (b7.c{up}..b7.c+(2u,.7u)) cutbefore bpath.b7;

  dcuta(pfl2,c1) pfl2.c{dir80}..{dir300}c1.c;
  dcuta(c1,c2)   c1.c{up}..{down}c2.c;
  dcuta(c2,c3)   c2.c{up}..{down}c3.c;
  drawterm (c3.c{up}..c3.c+(2u,.7u)) cutbefore bpath.c3;

  dcuta(ptr,fr1) ptr.c{right}..{down}pfl1.c;
  dcuta(fr1,fr2) pfl1.c..pfl2.c;
  drawterm (pfl2.c{down}..{dir320}pfl2.c+(1.3u,ypart(fr2.n-pfl1.c)))
  cutbefore bpath.fr2;

  numeric k;
  k = xpart(.5[pfl1.e,b1.w]);
  draw (k,ypart(fr1.n))--(k,ypart(fr1.s)) dashed evenly;
  draw (k,ypart(fr2.n))--(k,ypart(fr2.s)) dashed evenly;

  label.ulft(btex \it 4KB frame etex scaled .6, fr1.se);
  label.ulft(btex \it 4KB frame etex scaled .6, fr2.se);
endfig;


vardef largeroots@# =
  boxit.@#0(btex $R^{0}_{4k}$ etex scaled .8);
  boxit.@#1(btex $R^{1}_{4k}$ etex scaled .8);
  boxit.@#2(btex $R^{2}_{4k}$ etex scaled .8);
  boxit.@#14(btex $R^{14}_{4k}$ etex scaled .8);
  boxit.@#15(btex $R^{15}_{4k}$ etex scaled .8);
  eqsize(@#0,@#1,@#2,@#14,@#15);
  halign(@#0,@#1,@#2,@#14,@#15);
  xspace(@#0,@#1,@#2) = xspace(@#14,@#15) = 0; xspace(@#2,@#14) = 4u;
  ypart(@#15.n-@#15.s) = xpart(@#15.e-@#15.w);
enddef;

vardef smallroots@# =
  boxit.@#0(btex $R^{0}_{1k}$ etex scaled .8);
  boxit.@#1(btex $R^{1}_{1k}$ etex scaled .8);
  boxit.@#2(btex $R^{2}_{1k}$ etex scaled .8);
  boxit.@#3(btex $R^{3}_{1k}$ etex scaled .8);
  eqsize(@#0,@#1,@#2,@#3);
  halign(@#0,@#1,@#2,@#3);
  xspace(@#0,@#1,@#2,@#3) = 0;
enddef;


%%
%% Complex mapoping tree
%%
beginfig(2);
  u := 3mm;
  dl := 3u;

  circleit.nA(btex A etex);
  circleit.nB(btex B etex);
  circleit.nC(btex C etex);
  circleit.nD(btex D etex);
  circleit.nE(btex E etex);
  circleit.nF(btex F etex);
  circleit.nG(btex G etex);
  circleit.nH(btex H etex);
  circleit.nI(btex I etex);
  eqsize(nA,nB,nC,nD,nE,nF,nG,nH,nI);

  %% Top array
  boxit.r1(btex $R^1_{64k}$ etex);
  boxit.r2(btex $R^2_{64k}$ etex);
  eqxsize(r1,r2) = eqysize(r1,r2);
  halign(r1,r2);
  xspace(r1,r2) = 0;

  %% Root arrays
  largeroots.ra;
  largeroots.re;
  smallroots.rf;

  % Locate A, C, D
  valign(r2,nA,nC,nD);
  yspace(nF,nD,nC,nA,r2) = dl;

  % Locate A's array, C, E
  halign(ra0,nC,nE);
  xspace(ra15,nC,nE) = 3u;

  % Locate B
  valign(nB,ra0);
  halign(nB,nD);

  % Locate E's array, nF, nI
  valign(re0,nE,nF);
  halign(nD,re0);
  halign(nF,nI);
  valign(nI,re15);

  % Locate F's array
  valign(rf0,nF);
  yspace(rf0,nF) = dl;

  % Locate G, H
  halign(nG,nH);
  valign(nG,rf0); valign(nH,rf1);
  yspace(nG,rf0) = dl;

  fixsize(nA,r1,ra0);
  drawboxed(r1,r2,ra0,ra1,ra2,ra14,ra15);
  drawboxed(nA,nB,nC,nD,nE);
  drawboxed(re0,re1,re2,re14,re15);
  drawboxed(nF,nI,rf0,rf1,rf2,rf3,nG,nH);
  
  dcuta(r2,nA) r2.c -- nA.c;
  dcuta(nA,ra15) nA.c{left} .. ra15.c;
  dcuta(ra0,nB) ra0.c -- nB.c;
  dcuta(nA,nC) nA.c -- nC.c;
  dcuta(nC,nD) nC.c -- nD.c;
  dcuta(nA,nE) nA.c{right} .. nE.c;
  dcuta(nE,re0) nE.c -- re0.c;
  dcuta(re0,nF) re0.c -- nF.c;
  dcuta(re15,nI) re15.c -- nI.c;
  dcuta(nF,rf0) nF.c -- rf0.c;
  dcuta(rf0,nG) rf0.c -- nG.c;
  dcuta(rf1,nH) rf1.c -- nH.c;
  
  draw r1.nw -- r1.nw - (2u,0) dashed evenly;
  draw r1.sw -- r1.sw - (3.5u,0) dashed evenly;
  draw r2.ne -- r2.ne + (1.7u,0) dashed evenly;
  draw r2.se -- r2.se + (2.7u,0) dashed evenly;
  draw ra2.ne -- ra14.nw dashed evenly;
  draw ra2.se -- ra14.sw dashed evenly;
  draw re2.ne -- re14.nw dashed evenly;
  draw re2.se -- re14.sw dashed evenly;
endfig;

color partlyInvalid;
color completelyInvalid;

partlyInvalid = .85white;
completelyInvalid = .65white;


%%
%% Complex mapping tree, 4KB flushed
%%
beginfig(3);
  u := 3mm;
  dl := 3u;

  circleit.nA(btex A etex);
  circleit.nC(btex C etex);
  circleit.nD(btex D etex);
  circleit.nE(btex E etex);
  circleit.nI(btex I etex);
  eqsize(nA,nC,nD,nE,nI);

  %% Top array
  boxit.r1(btex $R^1_{64k}$ etex);
  boxit.r2(btex $R^2_{64k}$ etex);
  eqxsize(r1,r2) = eqysize(r1,r2);
  halign(r1,r2);
  xspace(r1,r2) = 0;

  %% Root arrays
  largeroots.ra;
  largeroots.rc;
  largeroots.rd;
  largeroots.re;

  % Locate A, C, D
  valign(r2,nA,nC,nD);
  yspace(nE,nA,r2) = dl;
  halign(nC,re0);
  yspace(nD,nC) = dl;

  % Locate A's array, C, E
  halign(ra0,nE);
  xspace(ra15,nC,nE) = 3u;

  % Locate C's array, D's array, E's array, nI
  valign(re0,nE);
  valign(nI,re15);
  yspace(nI,re0,nE) = dl;
  valign(rc0,ra0);
  halign(nD,rc0);
  yspace(rd0,nD) = dl;
  valign(rd0,nD);
  
  fixsize(nA,r1,ra0,rc0,rd0,re0);
  fillall(nA,nC,nD,nE)(withcolor partlyInvalid);
  fillall(ra0,rc0,rd0,re0)(withcolor completelyInvalid);
  drawboxed(r1,r2,ra0,ra1,ra2,ra14,ra15);
  drawboxed(nA,nC,nD,nE);
  drawboxed(re0,re1,re2,re14,re15,nI);
  drawboxed(rc0,rc1,rc2,rc14,rc15);
  drawboxed(rd0,rd1,rd2,rd14,rd15);
  
  dcuta(r2,nA) r2.c -- nA.c;
  dcuta(nA,ra15) nA.c{left} .. ra15.c;
  dcuta(nA,nC) nA.c -- nC.c;
  dcuta(nC,rc15) nC.c{left} .. rc15.c;
  dcuta(nC,nD) nC.c -- nD.c;
  dcuta(nD,rd0) nD.c -- rd0.c;
  dcuta(nA,nE) nA.c{right} .. nE.c;
  dcuta(nE,re0) nE.c -- re0.c;
  dcuta(re15,nI) re15.c -- nI.c;
  
  draw r1.nw -- r1.nw - (2u,0) dashed evenly;
  draw r1.sw -- r1.sw - (3.5u,0) dashed evenly;
  draw r2.ne -- r2.ne + (1.7u,0) dashed evenly;
  draw r2.se -- r2.se + (2.7u,0) dashed evenly;
  draw ra2.ne -- ra14.nw dashed evenly; draw ra2.se -- ra14.sw dashed evenly;
  draw rc2.ne -- rc14.nw dashed evenly; draw rc2.se -- rc14.sw dashed evenly;
  draw rd2.ne -- rd14.nw dashed evenly; draw rd2.se -- rd14.sw dashed evenly;
  draw re2.ne -- re14.nw dashed evenly; draw re2.se -- re14.sw dashed evenly;

  draw ra0.nw--ra0.se; draw ra0.sw--ra0.ne;
  draw rc0.nw--rc0.se; draw rc0.sw--rc0.ne;
  draw rd0.nw--rd0.se; draw rd0.sw--rd0.ne;
  draw re0.nw--re0.se; draw re0.sw--re0.ne;
endfig;



%%
%% Complex mapping tree, 1KB flushed
%%
beginfig(4);
  u := 3mm;
  dl := 3u;

  circleit.nA(btex A etex);
  circleit.nB(btex B etex);
  circleit.nC(btex C etex);
  circleit.nD(btex D etex);
  circleit.nE(btex E etex);
  circleit.nF(btex F etex);
  circleit.nH(btex H etex);
  circleit.nI(btex I etex);
  eqsize(nA,nC,nD,nE,nI);

  %% Top array
  boxit.r1(btex $R^1_{64k}$ etex);
  boxit.r2(btex $R^2_{64k}$ etex);
  eqxsize(r1,r2) = eqysize(r1,r2);
  halign(r1,r2);
  xspace(r1,r2) = 0;

  %% Root arrays
  largeroots.ra;
  smallroots.rrb;
  largeroots.rc;
  smallroots.rrc;
  largeroots.rd;
  smallroots.rrd;
  largeroots.re;
  smallroots.rrf;

  % Locate A, A's array, B, E
  valign(r2,nA);
  yspace(nB,nE,nA,r2) = dl;
  xspace(ra15,nA,nE) = 3u;
  valign(ra0,nB);
  halign(ra0,nE);

  % Locate E's array, nF, nI
  valign(re0,nE);
  halign(re0,nB);
  valign(nI,re15); valign(nF,re0);
  yspace(nF,nB) = dl; halign(nF,nI);

  % Locate B's array, C
  halign(rrb0,nF,nC);
  valign(rrb0,nB);
  valign(nC,nA);

  % Locate F's array, H
  valign(rrf0,nF);
  valign(rrf1,nH);
  yspace(nH,rrf0,nF) = dl;

  % Locate C'a arrays
  halign(rc0,rrf0);
  halign(rrc0,nH);
  valign(rrc0,rc0,rrb0);
  
  % Locate D, D's arrays
  halign(nD,nH);
  valign(rrd0,rd0,nD,nC);
  yspace(rrd0,rd0,nD) = dl;

  
  fixsize(nA,r1,ra0,rc0,rd0,re0);
  fillall(nA,nB,nC,nE,nF,re0,ra0)(withcolor partlyInvalid);
  drawboxed(r1,r2,ra0,ra1,ra2,ra14,ra15)
  drawboxed(nA,nB,nC,nE,nF,nI);
  drawboxed(re0,re1,re2,re14,re15);
  fillall(rrb0,rrf0)(withcolor completelyInvalid);
  drawboxed(rrb0,rrb1,rrb2,rrb3);
  drawboxed(rrf0,rrf1,rrf2,rrf3,nH);
  fillall(rrc0,rrd0)(withcolor completelyInvalid);
  fillall(nD,rc0,rd0)(withcolor partlyInvalid);
  drawboxed(rc0,rc1,rc2,rc14,rc15,rrc0,rrc1,rrc2,rrc3);
  drawboxed(nD,rd0,rd1,rd2,rd14,rd15,rrd0,rrd1,rrd2,rrd3);
  
  dcuta(r2,nA)    r2.c -- nA.c;
  dcuta(nA,ra15)  nA.c{left} .. ra15.c;
  dcuta(ra0,nB)   ra0.c -- nB.c;
  dcuta(nB,rrb0)  nB.c -- rrb0.c;
  dcuta(nA,nC)    nA.c -- nC.c;
  dcuta(nC,rc15)  nC.c{left} .. rc15.c;
  dcuta(rc0,rrc0) rc0.c -- rrc0.c;
  dcuta(nC,nD)    nC.c -- nD.c;
  dcuta(nD,rd0)   nD.c -- rd0.c;
  dcuta(rd0,rrd0) rd0.c -- rrd0.c;
  dcuta(nA,nE)    nA.c{right} .. nE.c;
  dcuta(nE,re0)   nE.c -- re0.c;
  dcuta(re15,nI)  re15.c -- nI.c;
  dcuta(re0,nF)   re0.c -- nF.c;
  dcuta(nF,rrf0)  nF.c -- rrf0.c;
  dcuta(rrf1,nH)  rrf1.c -- nH.c;
  
  draw r1.nw -- r1.nw - (2u,0) dashed evenly;
  draw r1.sw -- r1.sw - (3.5u,0) dashed evenly;
  draw r2.ne -- r2.ne + (1.7u,0) dashed evenly;
  draw r2.se -- r2.se + (2.7u,0) dashed evenly;
  draw ra2.ne -- ra14.nw dashed evenly; draw ra2.se -- ra14.sw dashed evenly;
  draw rc2.ne -- rc14.nw dashed evenly; draw rc2.se -- rc14.sw dashed evenly;
  draw rd2.ne -- rd14.nw dashed evenly; draw rd2.se -- rd14.sw dashed evenly;
  draw re2.ne -- re14.nw dashed evenly; draw re2.se -- re14.sw dashed evenly;

  draw rrb0.nw--rrb0.se; draw rrb0.sw--rrb0.ne;
  draw rrc0.nw--rrc0.se; draw rrc0.sw--rrc0.ne;
  draw rrd0.nw--rrd0.se; draw rrd0.sw--rrd0.ne;
  draw rrf0.nw--rrf0.se; draw rrf0.sw--rrf0.ne;
endfig;


%%
%% Maptree, logical
%%
beginfig(5)
  u := 3mm;

  circleit.A(btex A etex);
  circleit.B(btex B etex);
  circleit.C(btex C etex);
  circleit.D(btex D etex);
  circleit.E(btex E etex);
  eqsize(A,B,C,D,E);

  halign(B,E);
  halign(C,D);
  yspace(C,B,A) = 2.3u;
  xspace(C,D) = xspace(B,E) = 2u;
  xpart(.5[B.c,E.c]) = xpart(A.c);
  xpart(.5[C.c,D.c]) = xpart(B.c);

  drawboxed(A,B,C,D,E);
  undraw C.w-(u,0) -- C.w-(4u,0); % Extend picture leftwards
  undraw E.e+(u,0) -- E.e+(4u,0); % Extend picture rightwards

  dcuta(A,B) A.c -- B.c;
  dcuta(B,C) B.c -- C.c;
  dcuta(B,D) B.c -- D.c;
  dcuta(A,E) A.c -- E.c;

  label.lft(btex 0 etex scaled .7, A.w);
  label.lft(btex 1 etex scaled .7, B.w);
  label.lft(btex 2 etex scaled .7, C.w);
  label.lft(btex 2 etex scaled .7, D.w);
  label.lft(btex 1 etex scaled .7, E.w);
endfig;



%%
%% Maptree, implementation
%%
beginfig(6)
  u := 3mm;

  circleit.A(btex A etex);
  circleit.B(btex B etex);
  circleit.C(btex C etex);
  circleit.D(btex D etex);
  circleit.E(btex E etex);
  eqsize(A,B,C,D,E);

  halign(A,B,C,D,E);
  xspace(A,B,C,D,E) = 2u;

  drawboxed(A,B,C,D,E);

  dcuta(A,B) A.c{dir80} .. {dir280}B.c;
  dcuta(B,C) B.c{dir80} .. {dir280}C.c;
  dcuta(C,D) C.c{dir80} .. {dir280}D.c;
  dcuta(D,E) D.c{dir80} .. {dir280}E.c;
  drawterm (E.c{dir80}  .. E.c+(3.2u,u)) cutbefore bpath.E;

  undraw C.s-(0,u) -- C.s-(0,3u); % Extend picture downwards
  
  label.bot(btex 0 etex scaled .7, A.s);
  label.bot(btex 1 etex scaled .7, B.s);
  label.bot(btex 2 etex scaled .7, C.s);
  label.bot(btex 2 etex scaled .7, D.s);
  label.bot(btex 1 etex scaled .7, E.s);
endfig;


%%
%% Thread states
%%
beginfig(7)
  u := 5mm;

  circleit.run(btex \tt RUNNING etex);
  circleit.poll(btex \tt POLLING etex);
  circleit.wait(btex \tt WAITING etex);
  circleit.lrun(btex \tt\vbox{\hbox{LOCKED-}\hbox{RUNNING}} etex);
  circleit.lwait(btex \tt\vbox{\hbox{LOCKED-}\hbox{WAITING}} etex);

  eqsize(run,poll,wait,lrun,lwait);
  lwait.dx = lwait.dy = u;

  halign(poll,wait);
  halign(lrun,lwait);

  yspace(lwait,wait) = 5u;
  yspace(wait,run) = 3u;
  xspace(wait,poll) = 10u;
  xspace(lwait,lrun) = 8u;
  xpart(.5[wait.c,poll.c]) = xpart(.5[lwait.c,lrun.c]) = xpart(run.c);

  fixsize(lwait);
  drawboxed(run,poll,wait,lrun,lwait);

  label.urt(btex send etex,
    cuta(run,poll) run.c{dir10} .. {dir260}poll.c);
  label.llft(btex timeout etex,
    cuta(poll,run) poll.c{dir100} .. {dir170}run.c);

  label.ulft(btex recv etex,
    cuta(run,wait) run.c{dir170} .. {dir280}wait.c);
  label.lrt(btex timeout etex,
    cuta(wait,run) wait.c{dir80} .. {dir10}run.c);
  
  label.urt(btex \vbox{\hbox{receiver}\hbox{ready}} etex,
    cuta(poll,lrun) poll.c{dir290} .. {dir250}lrun.c);
  label.urt(btex \vbox{\hbox{sender}\hbox{ready}} etex,
    cuta(wait,lwait) wait.c{dir250} .. {dir290}lwait.c);
  
  label.top(btex pf resolved etex,
    cuta(lrun,lwait) lrun.c{dir130} .. {dir230}lwait.c);
  label.top(btex resolve pf etex,
    cuta(lwait,lrun) lwait.c{dir25} .. {dir335}lrun.c);

  label.bot(btex resolve pf etex,
    cuta(lrun,lwait) lrun.c{dir205} .. {dir155}lwait.c);
  label.bot(btex pf resolved etex,
    cuta(lwait,lrun) lwait.c{dir310} .. {dir50}lrun.c);

  label.urt(btex IPC done etex,
    cuta(lrun,run) lrun.c{dir30} .. {dir220}run.c);
  label.ulft(btex IPC done etex,
    cuta(lwait,run) lwait.c{dir150} .. {dir320}run.c);

  label.urt(btex recv etex,
    cuta(lrun,wait) lrun.c{up} .. {left}wait.c);
endfig;


%%
%% Shadow page table
%%
beginfig(8)
  U := 3mm;

  boxit.x1(btex $v_r$ etex); x1.dx = .1u; x1.dy = .3u;
  boxit.x2(btex $v_s$ etex);
  boxit.p1();
  boxit.p2();

  eqysize(x1,x2,p1,p2);
  eqxsize(x1,x2);
  eqxsize(p1,p2) = 13u;
  xspace(p1,p2) = 0;
  halign(x1,x2,p1,p2);
  xpart(p1.c-x1.c) = xpart(p2.c-x2.c) = 3u;

  dropshadow(p1,p2);
  fillall(x1,x2)(withcolor .85white);
  drawboxed(p1,p2,x1,x2);

  label.bot(btex Page Table etex, p1.s - (0,.5u));
  label.bot(btex Shadow Page Table etex, p2.s - (0,.5u));
  label.bot(btex $x$ etex scaled .8, p1.sw - (0,.2u));
  label.bot(btex $x+4096$ etex scaled .8, p2.sw - (0,.2u));
  label.bot(btex $x+8192$ etex scaled .8, p2.se - (0,.2u));

  label.top(btex $\overbrace{\hskip65mm}^{addr(v_s)=addr(v_r)+4096}$ etex,
    .5[x1.n,x2.n]);
endfig;

end;
