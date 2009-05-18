%%%
%%
%% Alignment macros
%%
%%%

%%
%% Align all boxes horizontally along their centers
%%
vardef halign@#(suffix first)(text tt) =
  if str @# = "":
    ypart(first.c-
      forsuffixes $=tt:
	if islastsuffix($)(tt):
	  $.c)
      else:
	$.c) = ypart($.c-
      fi
    endfor
  else:
    ypart(first.@#-
      forsuffixes $=tt:
	if islastsuffix($)(tt):
	  $.@#)
      else:
	$.@#) = ypart($.@#-
      fi
    endfor
  fi
  = 0;
enddef;

%%
%% Align all boxes vertically along their centers
%%
vardef valign@#(suffix first)(text tt) =
  if str @# = "":
    xpart(first.c-
      forsuffixes $=tt:
	if islastsuffix($)(tt):
	  $.c)
      else:
	$.c) = xpart($.c-
      fi
    endfor
  else:
    xpart(first.@#-
      forsuffixes $=tt:
	if islastsuffix($)(tt):
	  $.@#)
      else:
	$.@#) = xpart($.@#-
      fi
    endfor
  fi
  = 0;
enddef;


%%%
%%
%% Sizing macros
%%
%%%

%%
%% Set x-size of all boxes to be equal.  May be used as lvalue.
%%
def eqxsize(suffix first)(text tt) =
  xpart(first.e-first.w)
  forsuffixes $=tt: = xpart($.e-$w) endfor
enddef;

%%
%% Set y-size of all boxes to be equal.  May be used as lvalue.
%%
def eqysize(suffix first)(text tt) =
  ypart(first.n-first.s)
  forsuffixes $=tt: = ypart($.n-$s) endfor
enddef;

%%
%% Set x-size and y-size of all boxes equal.  May be used as lvalue.
%% using a pair.
%%
def eqsize(suffix first)(text tt) =
   (xpart(first.w-first.e), ypart(first.n-first.s))
  forsuffixes $=tt: = (xpart($.w-$e), ypart($.n-$s)) endfor
enddef;


%%%
%%
%% Suffix macros
%%
%%%

%%
%% Return last suffix in list.
%%
vardef lastsuffix(text tt) =
  save _i,_j;
  _i := _j := 0;
  forsuffixes $=tt: _i:=_i+1; endfor;
  forsuffixes $=tt:
    if incr _j = _i: $ fi
  endfor
enddef;

%%
%% Return true if suffix `ss' is the last suffix in `tt'.
%%
vardef islastsuffix(suffix ss)(text tt) =
  save _st,_r; string _st; boolean _r;
  _r := false;
  forsuffixes $=tt: _st := str $; endfor;
  if str ss = _st: _r := true; fi
  _r
enddef;


%%%
%%
%% Spacing macros
%%
%%%

%%
%% Set the x-spacing between all boxes to be equal.  May be used as lvalue.
%%
def xspace(suffix first)(text tt) =
  -xpart(first.e-
  forsuffixes $=tt:
    if islastsuffix($)(tt):
      $.w)
    else:
      $.w) = -xpart($.e-
    fi
  endfor
enddef;

%%
%% Set the y-spacing between all boxes to be equal.  May be used as lvalue.
%%
def yspace(suffix first)(text tt) =
  -ypart(first.n-
  forsuffixes $=tt:
    if islastsuffix($)(tt):
      $.s)
    else:
      $.s) = -ypart($.n-
    fi
  endfor
enddef;


%%%
%%
%% Dropshadow
%%
%%%
pair shadowoff;
shadowoff = (2pt,-2pt);

%input mpattern;
%
%beginpattern(shadow_pattern);
%  u:=.3mm;
%  fill unitsquare scaled u;
%  fill unitsquare scaled u shifted (u,u);
%  patternbbox(0,0,2u,2u);
%endpattern;

%%
%% Dropshadow on a list of boxes
%%
vardef dropshadow(text t) =
  fixsize(t); fixpos(t);
  forsuffixes s=t:
    drawshadow bpath.s;
  endfor;
  forsuffixes s=t:
    unfill bpath.s;
  endfor;
enddef;

%%
%% Dropshadow on a single path
%%
vardef drawshadow expr p =
  fill p shifted shadowoff withcolor .4white;
%  fill p shifted shadowoff withpattern shadow_pattern;
  unfill p;
enddef;


%%%
%%
%% Picture maker
%%
%%%
pair mkpicoff, mkpicoff.c;
pair mkpicoff.nw, mkpicoff.w, mkpicoff.sw, mkpicoff.s;
pair mkpicoff.se, mkpicoff.e, mkpicoff.ne, mkpicoff.n;
mkpicoff    := (-.5,.5);    mkpicoff.c  := (-.5,.5);
mkpicoff.nw := (0,0);       mkpicoff.w  := (0,.5);
mkpicoff.sw := (0,1);       mkpicoff.s  := (.5,1);
mkpicoff.se := (1,1);       mkpicoff.e  := (1,.5);
mkpicoff.ne := (1,0);       mkpicoff.n  := (.5,0);

%%
%% Make picture with a specified size.  First argument (if present)
%% after picture will be the xsize (may be set to 0 to keep size).
%% Next argument (if present) will be ysize.  An alignment position
%% may be given as suffix to the function (nw,w,sw,s,se,e,ne,n,c).  If
%% no suffix is given, `c' will be assumed.  If a pair is given as an
%% argument, it will be used as on offset of the picture within the
%% new picture (relative to the alignment).
%%
vardef mkpic@#(expr inarg)(text tt) =
  save ll,lr,ul,ur,off, inpic;
  save outpic, st, xsz, ysz, oxsz, oysz, ?;
  pair ll,lr,ul,ur,off;
  picture outpic, inpic;
  string st;
  numeric xsz, ysz, oxsz, oysz;
  boolean ?;

  inpic := if picture inarg: inarg
  elseif string inarg: inarg infont defaultfont scaled defaultscale
  else: nullpicture
  fi;
  outpic := inpic;
  
  xsz := oxsz := xpart(lrcorner inpic - llcorner inpic);
  ysz := oysz := ypart(ulcorner inpic - llcorner inpic);
  ? := false;
  off := (0,0);

  %% Parse arguments
  for ss=tt:
    if numeric ss:
      if ss <> 0:
	if not ?: xsz := ss else: ysz := ss fi;
      fi;
      ? := true;
    elseif pair ss:
      off := ss;
    fi;
  endfor;

  ul := ulcorner inpic;
  st := str @#;
 
  %% Adjust for x-position
  if (st = "") or (st = "n") or (st = "c") or (st = "s"):
    ul := ul + (-.5xsz + .5oxsz, 0);
  elseif (st = "ne") or (st = "e") or (st = "se"):
    ul := ul + (-xsz + oxsz, 0);
  fi;

  %% Adjust for y-position
  if (st = "") or (st = "w") or (st = "c") or (st = "e"):
    ul := ul + (0, .5ysz - .5oysz);
  elseif (st = "sw") or (st = "s") or (st = "se"):
    ul := ul + (0, ysz - oysz);
  fi;

  ul := ul + off;
  ll := ul + (0,-ysz);
  lr := ul + (xsz,-ysz);
  ur := ul + (xsz,0);
  
  setbounds outpic to ll--lr--ur--ul--cycle;
  outpic
enddef;


%%%
%%
%% Bus boxes
%%
%%%
if unknown defaultdx:
  input boxes;
fi

newinternal default_busarrlen, default_busarrdir;
default_busarrlen := 3bp;
default_busarrdir := 45;

vardef hbusit@#(text tt) =
  beginbox_("hbuspath_","sizebus_",@#,tt);
  generic_declare(pair) _n.sw, _n.s, _n.se, _n.e, _n.ne, _n.n, _n.nw, _n.w;
  0 = xpart (@#nw-@#sw) = ypart(@#se-@#sw);
  0 = xpart(@#ne-@#se) = ypart(@#ne-@#nw);
  @#w = .5[@#nw,@#sw];
  @#s = .5[@#sw,@#se];
  @#e = .5[@#ne,@#se];
  @#n = .5[@#ne,@#nw];
  @#ne-@#c = @#c-@#sw = (@#dx,@#dy) + .5*(urcorner pic_@# - llcorner pic_@#);
  endbox_(clearb_,@#);
enddef;

def hbuspath_(suffix $) =
  $.w --
  ($.nw+(($.nw-$w)+(0,$.arrlen)) rotated -$.arrdir) --
  (xpart(($.nw+(($.nw-$w)+(0,$.arrlen)) rotated -$.arrdir)),ypart($.nw)) --
  (xpart(($.ne+(($.ne-$e)+(0,$.arrlen)) rotated $.arrdir)),ypart($.ne)) --
  ($.ne+(($.ne-$e)+(0,$.arrlen)) rotated $.arrdir) --
  $.e --
  ($.se-(($e-$.se)+(0,$.arrlen)) rotated -$.arrdir) --
  (xpart(($.se-(($e-$.se)+(0,$.arrlen)) rotated -$.arrdir)),ypart($.se)) --
  (xpart(($.sw-(($w-$.sw)+(0,$.arrlen)) rotated $.arrdir)),ypart($.sw)) --
  ($.sw-(($w-$.sw)+(0,$.arrlen)) rotated $.arrdir) --
  cycle
enddef;

vardef vbusit@#(text tt) =
  beginbox_("vbuspath_","sizebus_",@#,tt);
  generic_declare(pair) _n.sw, _n.s, _n.se, _n.e, _n.ne, _n.n, _n.nw, _n.w;
  0 = xpart (@#nw-@#sw) = ypart(@#se-@#sw);
  0 = xpart(@#ne-@#se) = ypart(@#ne-@#nw);
  @#w = .5[@#nw,@#sw];
  @#s = .5[@#sw,@#se];
  @#e = .5[@#ne,@#se];
  @#n = .5[@#ne,@#nw];
  @#ne-@#c = @#c-@#sw = (@#dx,@#dy) + .5*(urcorner pic_@# - llcorner pic_@#);
  endbox_(clearb_,@#);
enddef;

def vbuspath_(suffix $) =
  $.n --
  ($.ne+(($.ne-$n)+($.arrlen,0)) rotated -$.arrdir) --
  (xpart($.ne),ypart($.ne+(($.ne-$n)+($.arrlen,0)) rotated -$.arrdir))--
  (xpart($.se),ypart($.se+(($.se-$s)+($.arrlen,0)) rotated $.arrdir)) --
  ($.se+(($.se-$s)+($.arrlen,0)) rotated $.arrdir) --
  $.s --
  ($.sw-(($.s-$sw)+($.arrlen,0)) rotated -$.arrdir) --
  (xpart($.sw),ypart($.sw-(($.s-$sw)+($.arrlen,0)) rotated -$.arrdir)) --
  (xpart($.nw),ypart($.nw-(($.n-$nw)+($.arrlen,0)) rotated $.arrdir)) --
  ($.nw-(($.n-$nw)+($.arrlen,0)) rotated $.arrdir) --
  cycle
enddef;

def sizebus_(suffix $) =
  if unknown $.dx: $.dx=defaultdx; fi
  if unknown $.dy: $.dy=defaultdy; fi
  if unknown $.arrlen: $.arrlen=default_busarrlen; fi
  if unknown $.arrdir: $.arrdir=default_busarrdir; fi
enddef;



%%%
%%
%% Creating circles and boxes as pictures.
%%
%%%

%%
%% Create circle containing the picture (and optionally filled)
%%
vardef circlepic(text tt) =
  save inpic,outpic; picture inpic,outpic;
  save bound; path bound;
  save colp, argp; boolean colp, argp;
  save fillcol; color fillcolor;
  save xsz, ysz;
  colp := argp := false;
  
  for ss=tt:
    if not argp:
      inpic := if picture ss: ss
      elseif string ss: ss infont defaultfont scaled defaultscale
      else: nullpicture
      fi;
      argp := true;
    elseif color ss:
      colp := true;
      fillcolor := ss;
    fi;
  endfor;

  xsz := xpart(lrcorner inpic - llcorner inpic);
  ysz := ypart(ulcorner inpic - llcorner inpic);

  bound := fullcircle
  xscaled ((xsz++ysz)+defaultdx) yscaled ((xsz++ysz)+defaultdy)
  shifted (.5(xsz,ysz) + llcorner inpic);

  outpic := nullpicture;
  if colp: addto outpic contour bound withcolor fillcolor; fi;
  addto outpic also inpic;
  addto outpic doublepath bound;
  outpic
enddef;

%%
%% Create rectangle containing the picture (and optionally filled)
%%
vardef boxpic(text tt) =
  save inpic,outpic; picture inpic,outpic;
  save bound; path bound;
  save colp, argp; boolean colp, argp;
  save fillcol; color fillcolor;
  colp := argp := false;

  for ss=tt:
    if not argp:
      inpic := if picture ss: ss
      elseif string ss: ss infont defaultfont scaled defaultscale
      else: nullpicture
      fi;
      argp := true;
    elseif color ss:
      colp := true;
      fillcolor := ss;
    fi;
  endfor;

  bound := (lrcorner inpic + (defaultdx,-defaultdy)) --
  (llcorner inpic + (-defaultdx,-defaultdy)) --
  (ulcorner inpic + (-defaultdx,defaultdy)) --
  (urcorner inpic + (defaultdx,defaultdy)) -- cycle;
  
  outpic := nullpicture;
  if colp: addto outpic contour bound withcolor fillcolor; fi;
  addto outpic also inpic;
  addto outpic doublepath bound;
  outpic
enddef;



%%%
%%
%% Misc macros.
%%
%%%

%%
%% Fill all boxes using the given specification `w'.
%%
vardef fillall(text t)(text w) =
  fixsize(t); fixpos(t);
  forsuffixes s=t:
    fill bpath.s w;
  endfor;
enddef;


%%
%% Magnifying glass
%%
vardef magnify(expr magpos, magfact, magsize) =
  save magpic; picture magpic;
  magpic := currentpicture;

  clip magpic to fullcircle scaled magsize shifted magpos;
  unfill fullcircle scaled (magsize*magfact) shifted magpos;
  draw magpic shifted -magpos scaled magfact shifted magpos;
  draw fullcircle scaled (magsize*magfact) shifted magpos;
enddef;


newinternal termlength;
termlength := 3;

%%
%% Draw expression with a terminating end (i.e. ``----|'')
%%
def drawterm expr p =
  draw (-(.5termlength,0)--(.5termlength,0))
  rotated (angle(direction infinity of p)+90)
  shifted point infinity of p;
  draw p;
enddef;


vardef midcutting(suffix ss) =
  save ce, no, so, i, cw, ch, nc;
  ce := .5xpart(ss.w+ss.e);
  no := ypart(ss.n); so := ypart(ss.s);
  cw := 5bp; ch := 5bp;
  nc := 4;
    
  ss.nw--(ce-.5nc*cw-.25cw,no)--
  for i=-.5nc step 1 until .5nc-1:
    (ce+cw*i,no+.5ch)--(ce+cw*i+.5cw,no-.5ch)--
  endfor
  (ce+.5nc*cw-.25cw,no)--ss.ne--ss.se--(ce+.5nc*cw-.25cw,so)--
  for i=.5nc-.5 step -1 until -.5nc:
    (ce+cw*i,so-.5ch)--(ce+cw*i-.5cw,so+.5ch)--
  endfor
  (ce-.5nc*cw-.25cw,so)--ss.sw--cycle
enddef;
