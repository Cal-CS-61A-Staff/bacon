CC=g++
CFLAGS=-pthread -std=c++11 -O3 -I $(IDIR)

IDIR =include
ODIR=obj

_DEPS = stdafx.h params.h analysis.h strategy.h dice.h hog.h
DEPS = $(patsubst %,$(IDIR)/%, $(_DEPS))

_OBJ = main.o hog.o strategy.o analysis.o 
OBJ = $(patsubst %,$(ODIR)/%, $(_OBJ))

OUTPUTNAME = bacon
OUTPUTDIR = bin/
HOGCONV = hogconv.py
HOGCONVBIN = hogconv

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUTPUTNAME) : $(OBJ)
	$(CC) -o $(OUTPUTDIR)$@ $^ $(CFLAGS)

.PHONY: clean install
	
clean :
	rm -rf $(ODIR)/*
	rm -rf .vs
	rm -f bin/*.pdb
	rm -f bin/*.iobj
	rm -f bin/*.ipdb
	rm -f bin/*.ilk
	
install:
	rm -f /usr/bin/$(OUTPUTNAME)
	cp $(OUTPUTDIR)$(OUTPUTNAME) /usr/bin
	cp $(HOGCONV) /usr/bin/$(HOGCONVBIN)
	chmod +x /usr/bin/$(HOGCONV)

install_user:
	rm -f ~/bin/$(OUTPUTNAME)
	cp $(OUTPUTDIR)$(OUTPUTNAME) ~/bin
	cp $(HOGCONV) ~/bin/$(HOGCONVBIN)
	chmod +x ~/bin/$(HOGCONVBIN)