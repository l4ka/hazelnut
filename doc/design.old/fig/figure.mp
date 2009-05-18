input rboxes;
input graph;
input esk;

%prologues:=1;

vardef dcuta(suffix a,b) expr p =
  drawarrow p cutbefore bpath.a cutafter bpath.b;
enddef; 


beginfig(1);
  u := 3mm;

  circleit.f2(btex $f_5$ etex);
  circleit.f3(btex $f_{17}$ etex);
  circleit.f5(btex $f_2$ etex);
  circleit.f8(btex $f_8$ etex);
  circleit.f9(btex $f_9$ etex);
  circleit.f10(btex $f_{10}$ etex);
  circleit.f11(btex $f_{11}$ etex);

  eqsize(f2,f3,f4,f5,f8,f9,f10,f11);
  halign(f2,f3,f5,f8,f9,f10,f11);
  xspace(f2,f3,f5,f8,f9,f10,f11) = u;

  circleit.b1(btex  etex);
  xpart(f8.w-b1.w) = xpart(b1.e-f11.e) = .5u;
  ypart(f8.s-b1.s) = ypart(b1.n-f8.n) = 2u;

  boxit.pfreelist(btex frame list pointer etex);
  boxit.pcluster(btex cluster pointer etex);
  boxit.descr(btex \vbox{\hbox{Cluster of four consecutive}
    \hbox{aligned frames.}} etex);

  halign(pfreelist,pcluster);
  xpart(f2.c-pfreelist.c) =  xpart(f8.c-pcluster.c) = 2u;
  yspace(f2,pfreelist) = 5u;

  descr.n = b1.s - (2u,3u);
  
  drawboxed(f11,f2,f3,f5,f8,f9,f10);
  fixsize(b1); draw bpath.b1 dashed evenly;
  drawunboxed(pfreelist,pcluster,descr);

  dcuta(f2,f3)   f2.c{up}..{down}f3.c;
  dcuta(f3,f5)   f3.c{up}..{down}f5.c;
  dcuta(f5,f8)   f5.c{up}..{down}f8.c;
  dcuta(f8,f9)   f8.c{up}..{down}f9.c;
  dcuta(f9,f10)  f9.c{up}..{down}f10.c;
  dcuta(f10,f11) f10.c{up}..{down}f11.c;
  drawterm (f11.c{up}..f11.c+(3u,u)) cutbefore bpath.f11;

  dcuta(pfreelist,f2) pfreelist.c..{down}f2.c;
  dcuta(pcluster,f8) pcluster.c..{down}f8.c;

  dcuta(descr,b1) descr.c..{up}b1.c;

endfig;


beginfig(2);
  u := 3mm;

  boxit.la(btex $A_{64k}$ etex); 
  boxit.lb(btex $B_{64k}$ etex);
  boxit.lc(btex $C_{64k}$ etex);
  boxit.ld(btex $D_{64k}$ etex);
  ypart(la.n-la.s) = xpart(la.e-la.w);

  eqsize(la,lb,lc,ld);

  valign(la,lb);
  halign(lc,ld);
  xpart(.5[lc.c,ld.c]) = xpart(lb.c);

  yspace(lc,lb,la) = 3u;
  xspace(lc,ld) = 1.5u;

  drawboxed(la,lb,lc,ld);

  drawarrow la.s--lb.n;
  dcuta(lb,lc) lb.c--lc.c;
  dcuta(lb,ld) lb.c--ld.c;
endfig;

beginfig(3);
  u := 3mm;
  
  boxit.sa1(btex $A_{4k}^1$ etex);
  boxit.sa2(btex $A_{4k}^2$ etex);
  boxit.sa3(btex $$ etex);
  boxit.sa4(btex $A_{4k}^4$ etex);
  boxit.sacut(); xpart(sacut.e-sacut.w) = 5u;
  boxit.sa14(btex $A_{4k}^{14}$ etex);
  boxit.sa15(btex $A_{4k}^{15}$ etex);
  boxit.sa16(btex $A_{4k}^{16}$ etex);

  boxit.sb1(btex $B_{4k}^1$ etex);
  boxit.sc1(btex $C_{4k}^1$ etex);
  boxit.sd1(btex $D_{4k}^1$ etex);
  boxit.desc(btex Same tree repeated 15 times etex);

  ypart(sa1.n-sa1.s) = xpart(sa1.e-sa1.w);
  eqsize(sa1,sa2,sa3,sa4,sa14,sa15,sa16,sb1,sc1,sd1);
  eqysize(sa1,sacut);

  halign(sa1,sa2,sa3,sa4,sacut,sa14,sa15,sa16);
  xspace(sa1,sa2,sa3,sa4,sacut,sa14,sa15,sa16) = 0;
  valign(sa1,sb1);
  xpart(.5[sc1.c,sd1.c]) = xpart(sb1.c);
  halign(sb1,desc);
  halign(sc1,sd1);
  xspace(sc1,sd1) = u;
  xpart(.5[sa2.c,sa16.c]) = xpart(desc.c);
  yspace(sd1,sb1,sa1) = 3u;

  
  drawboxed(sa1,sa2,sa3,sa4,sa14,sa15,sa16);
  fillall(sa3)(withcolor .8white);
  drawboxed(sa3,sb1,sc1,sd1);
  fixsize(sacut); draw bpath.sacut dashed evenly;
  drawunboxed(desc);

  drawarrow sa1.s--sb1.n;
  dcuta(sb1,sc1) sb1.c--sc1.c;
  dcuta(sb1,sd1) sb1.c--sd1.c;

  drawarrow sa2.s--sa2.s-(0,ypart(sa1.s-sb1.n));
  drawarrow sa4.s--sa4.s-(0,ypart(sa1.s-sb1.n));
  drawarrow sa14.s--sa14.s-(0,ypart(sa1.s-sb1.n));
  drawarrow sa15.s--sa15.s-(0,ypart(sa1.s-sb1.n));
  drawarrow sa16.s--sa16.s-(0,ypart(sa1.s-sb1.n));
endfig;


beginfig(4)
  u := 3mm;

  boxit.r1(btex $R^1_{64k}$ etex);
  boxit.r2(btex $R^2_{64k}$ etex);
  eqxsize(r1,r2) = eqysize(r1,r2);

  boxit.a(btex $A$ etex);
  boxit.b(btex $B$ etex);
  boxit.c(btex $C$ etex);
  eqxsize(a,b,c) = eqysize(a,b,c);

  halign(r1,r2);
  xspace(r1,r2) = 0;

  halign(b,c);
  xspace(b,c) = u;
  valign(a,r2);

  yspace(b,a,r2) = 2u;
  xpart(.5[b.c,c.c]) = xpart(a.c);

  drawboxed(r1,r2,a,b,c);
  drawarrow r2.s -- a.n;
  dcuta(a,b) a.c -- b.c;
  dcuta(a,c) a.c -- c.c;
  draw r1.nw -- r1.nw - (2u,0) dashed evenly;
  draw r1.sw -- r1.sw - (3.5u,0) dashed evenly;
  draw r2.ne -- r2.ne + (1.7u,0) dashed evenly;
  draw r2.se -- r2.se + (2.7u,0) dashed evenly;
endfig;


beginfig(5)
  u := 3mm;

  boxit.r1(btex $R^1_{64k}$ etex);
  boxit.r2(btex $R^2_{64k}$ etex);
  eqxsize(r1,r2) = eqysize(r1,r2);

  boxit.s0(btex $R^{2.0}_{4k}$ etex);
  boxit.s1(btex $R^{2.1}_{4k}$ etex);
  boxit.s2(btex $R^{2.2}_{4k}$ etex);
  boxit.s14(btex $R^{2.14}_{4k}$ etex);
  boxit.s15(btex $R^{2.15}_{4k}$ etex);
  eqsize(s0,s1,s2,s14,s15);
  ypart(s15.n-s15.s) = xpart(s15.e-s15.w);
  
  boxit.a(btex $A$ etex);
  boxit.b(btex $B$ etex);
  boxit.c(btex $C$ etex);
  boxit.a2(btex $A$ etex);
  eqxsize(a,b,c,a2) = eqysize(a,b,c,a2);

  halign(r1,r2);
  xspace(r1,r2) = 0;
  halign(s0,s1,s2,s14,s15);
  xspace(s0,s1,s2) = xspace(s14,s15) = 0;
  xspace(s2,s14) = 8u;

  halign(a,a2);
  halign(b,c);
  xspace(b,c) = u;
  valign(a,s0,r2);
  valign(a2,s15);

  yspace(b,a,s0,r2) = 2u;
  xpart(.5[b.c,c.c]) = xpart(a.c);

  drawboxed(r1,r2,s0,s1,s2,s14,s15,a,b,c,a2);

  drawarrow r2.s -- s0.n;
  drawarrow s0.s -- a.n;
  dcuta(a,b) a.c -- b.c;
  dcuta(a,c) a.c -- c.c;
  drawarrow s15.s -- a2.n;
  dcuta(s1,a) s1.c{down} .. a.c;
  dcuta(s2,a) s2.c{down} .. {left}a.c;
  dcuta(s14,a) s14.c{down} .. {dir150}a.c;

  draw r1.nw -- r1.nw - (2u,0) dashed evenly;
  draw r1.sw -- r1.sw - (3.5u,0) dashed evenly;
  draw r2.ne -- r2.ne + (1.7u,0) dashed evenly;
  draw r2.se -- r2.se + (2.7u,0) dashed evenly;
  draw s2.ne -- s14.nw dashed evenly;
  draw s2.se -- s14.sw dashed evenly;
endfig;


beginfig(6);
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


beginfig(7);
  u := 3.2mm;

  boxit.addrA(); boxit.addrB(); boxit.addrC(); boxit.addrD();
  boxit.pA(btex $a_1$ etex);
  boxit.pB(btex $b_1$ etex);
  boxit.pC(btex $c_1$ etex);
  boxit.pD1(btex $d_1$ etex);
  boxit.pD2(btex $d_2$ etex);
  
  eqysize(addrA,addrB,addrC,addrD) =  eqysize(pA,pB,pC,pD1,pD2);
  eqxsize(pA,pB,pC,pD1,pD2);
  eqxsize(addrA,addrB,addrC,addrD) = 8u;
  
  halign(addrA,pA); xpart(pA.w-addrA.w) = 2u;
  halign(addrB,pB); xpart(pB.w-addrB.w) = 3.5u;
  halign(addrC,pC); xpart(pC.w-addrC.w) = 1.2u;
  halign(addrD,pD1,pD2); xpart(pD1.w-addrD.w) = .5u;
  xpart(pD2.w-addrD.w) = 3.1u;

  yspace(addrD,addrB,addrA) = 3u;
  halign(addrB,addrC);
  xspace(addrB,addrC) = 2.7u;
  xpart(.5[addrB.e,addrC.w]) - u = xpart(addrA.c);
  xpart(addrD.c) = xpart(addrA.c) + 2u;
  
  fixsize(pA,pB,pC,pD1,pD2);
  drawboxed(addrA,addrB,addrC,addrD);
  fillall(pA,pB,pC,pD1,pD2)(withcolor .8white);
  drawboxed(pA,pB,pC,pD1,pD2);

  label.lft(btex $A$: etex, addrA.w);
  label.lft(btex $B$: etex, addrB.w);
  label.lft(btex $C$: etex, addrC.w);
  label.lft(btex $D$: etex, addrD.w);

  dcuta(pA,pB) pA.c{down}..pB.c{down};
  dcuta(pA,pC) pA.c{down}..pC.c{down};
  dcuta(pB,pD1) pB.c{down}..pD1.c{down};
  dcuta(pC,pD2) pC.c{down}..pD2.c{down};
endfig;

beginfig(8);
  u := 3mm;

  boxit.pA(btex $a_1$ etex);
  boxit.pB(btex $b_1$ etex);
  boxit.pC(btex $c_1$ etex);
  boxit.pD1(btex $d_1$ etex);
  boxit.pD2(btex $d_2$ etex);

  eqsize(pA,pB,pC,pD1,pD2);

  valign(pD1,pB); valign(pD2,pC);
  halign(pD1,pD2);
  halign(pB,pC);
  yspace(pD1,pB,pA) = 3u;
  xspace(pB,pC) = 2u;
  xpart(.5[pB.c,pC.c]) = xpart(pA.c);
  
  drawboxed(pA,pB,pC,pD1,pD2);
  
  dcuta(pA,pB)  pA.c--pB.c;
  dcuta(pA,pC)  pA.c--pC.c;
  dcuta(pB,pD1) pB.c--pD1.c;
  dcuta(pC,pD2) pC.c--pD2.c;
endfig;


beginfig(9);
  u := 3.2mm;

  boxit.addrA(); boxit.addrB(); boxit.addrC(); boxit.addrD();
  boxit.pA(btex $a_1$ etex);
  boxit.pB(btex $b_1$ etex);
  boxit.pC(btex $c_1$ etex);
  boxit.pD1(btex $d_1$ etex);
  boxit.pD2(btex $d_2$ etex scaled .7);
  
  eqysize(addrA,addrB,addrC,addrD) =  eqysize(pA,pB,pC,pD1,pD2);
  eqxsize(pA,pB,pC,pD1) = 3.5u;
  xpart(pD2.e-pD2.w) = .8u;
  eqxsize(addrA,addrB,addrC,addrD) = 8u;
  
  halign(addrA,pA); xpart(pA.w-addrA.w) = 2u;
  halign(addrB,pB); xpart(pB.w-addrB.w) = 3.5u;
  halign(addrC,pC); xpart(pC.w-addrC.w) = 1.2u;
  halign(addrD,pD1,pD2); xpart(pD1.w-addrD.w) = .5u;
  xpart(pD2.w-addrD.w) = 5.1u;

  yspace(addrD,addrB,addrA) = 3u;
  halign(addrB,addrC);
  xspace(addrB,addrC) = 2.7u;
  xpart(.5[addrB.e,addrC.w]) - u = xpart(addrA.c);
  xpart(addrD.c) = xpart(addrA.c) + 2u;
  
  fixsize(pA,pB,pC,pD1,pD2);
  drawboxed(addrA,addrB,addrC,addrD);
  fillall(pA,pB,pC,pD1,pD2)(withcolor .8white);
  drawboxed(pA,pB,pC,pD1,pD2);

  label.lft(btex $A$: etex, addrA.w);
  label.lft(btex $B$: etex, addrB.w);
  label.lft(btex $C$: etex, addrC.w);
  label.lft(btex $D$: etex, addrD.w);

  dcuta(pA,pB) pA.c{down}..pB.c{down};
  dcuta(pA,pC) pA.c{down}..pC.c{down};
  dcuta(pB,pD1) pB.c{down}..pD1.c{down};
  dcuta(pC,pD2) (pC.e-(.4u,0)){down}..pD2.c{down};
  draw (pC.se-(.8u,0))--(pC.ne-(.8u,0)) dashed evenly scaled .5;

  pair k;
  k = (xpart(.5[addrB.w,addrC.e]),ypart(addrA.n)+u);
  label.top(btex \vbox{
    \hbox{$a_1 = b_1 = c_1 = d_1 = 64~\rm KB$}
    \hbox{$d_2 = 4~\rm KB$}} etex, k);
endfig;


beginfig(10);
  u := 3mm;

  boxit.pA(btex $a^{64k}_1$ etex);
  boxit.pB(btex $b^{64k}_1$ etex);
  boxit.pD1(btex $d^{64k}_1$ etex);
  boxit.pD2(btex $d^{4k}_2$ etex);

  boxit.s1(btex $c^{4k}_1$ etex);
  boxit.s2(btex $c^{4k}_2$ etex);
  boxit.s3(btex $c^{4k}_3$ etex);
  boxit.s4(btex $c^{4k}_4$ etex);
  boxit.scut(); xpart(scut.e-scut.w) = 5u;
  boxit.s14(btex $c^{4k}_{14}$ etex);
  boxit.s15(btex $c^{4k}_{15}$ etex);
  boxit.s16(btex $c^{4k}_{16}$ etex);

  eqsize(s1,s2,s3,s4,s14,s15,s16,pD2);
  eqysize(s1,scut);
  halign(s1,s2,s3,s4,scut,s14,s15,s16);
  xspace(s1,s2,s3,s4,scut,s14,s15,s16) = 0;
  
  eqsize(pA,pB,pD1);

  valign(pD1,pB);
  halign(pD1,pD2);
  halign(pB,s1);
  yspace(pD1,pB,pA) = 3u;
  xspace(pB,s1) = 2u;
  xpart(.5[pB.c,s1.c]) = xpart(pA.c);
  valign(pD2,s16);
  
  drawboxed(pA,pB,s1,s2,s3,s4,s14,s15,s16,pD1,pD2);
  draw bpath.scut dashed evenly;
  
  dcuta(pA,pB)  pA.c--pB.c;
  dcuta(pA,s1)  pA.c--s1.c;
  dcuta(pB,pD1) pB.c--pD1.c;
  dcuta(s16,pD2) s16.c--pD2.c;
endfig;

  
end;
