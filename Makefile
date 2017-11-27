CXX:=g++
CXXFLAGS:= -Wall -O3 -std=gnu++14

.PHONY: all clean

all: paper.pdf hex

hex: hex.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<


paper.pdf: paper.tex
	pdflatex -shell-escape -halt-on-error paper.tex
	bibtex paper
	pdflatex -shell-escape -halt-on-error paper.tex
	pdflatex -shell-escape -halt-on-error paper.tex

clean:
	-rm -f $(addprefix paper., aux bbl blg log out pdf)
	-rm -f hex
