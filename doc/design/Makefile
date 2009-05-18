
.SUFFIXES:
.SUFFIXES: .tex .dvi .ps

all: design.ps

.tex.dvi:
	latex $<

.dvi.ps:
	dvips $< -o


design.dvi: fig/figure.1

fig/figure.1: fig/figure.mp
	(cd fig; mpost figure)
