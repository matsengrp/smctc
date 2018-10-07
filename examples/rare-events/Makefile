include ../../Makefile.in

C = main.cc simfunctions.cc
O = main.o simfunctions.o
H = simfunctions.hh

CXXFLAGS += -I../../include -L../../lib
LFLAGS := -g -I../../include -L../../lib -static $(LFLAGS)


all: rare

.PHONY: clean

clean:
	-rm *.o
	-rm *~
	-rm rare

rare: $(C) $(H)
	$(CXX) $(CXXFLAGS) -c $(C)
	$(CXX) $(O) -lsmctc $(LFLAGS) -orare
	cp rare ../../bin


