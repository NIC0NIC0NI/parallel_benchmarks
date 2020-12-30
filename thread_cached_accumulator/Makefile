default:sum_test

include ../Makefile.inc

sum_test:test.cpp accumulator.h $(PSRC) $(PINC)
	g++ -O2 $(POPT) $< $(PSRC) -o $@ $(PLIB) 