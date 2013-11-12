include ../Makefile.in

CXXFLAGS += -I ../include
SMCC = rng.cc history.cc smc-exception.cc
SMCO = rng.o history.o smc-exception.o

all: libsmctc.a

.PHONY: clean

clean:
	-rm *.o
	-rm *.a
	-rm *~

libsmctc.a: $(SMCO) Makefile
	ar rcs libsmctc.a $(SMCO)
	mkdir -p ../lib
	cp libsmctc.a ../lib
