# -*= mode: makefile-gmake -*-

CXX := g++
CXXFLAGS := -g

all:
.PHONY: all

all: cygbug-duplocale.exe cygbug-locale.exe demangle.exe
cygbug-duplocale.exe: cygbug-duplocale.o
	$(CXX) -g -o $@ $^
cygbug-locale.exe: cygbug-locale.o
	$(CXX) -g -o $@ $^
demangle.exe: demangle.o
	$(CXX) -g -o $@ $^

all: cyglocale_patcher.dll
cyglocale_patcher.dll: cyglocale_patcher.cpp
	$(CXX) -O2 -s -shared -D USE_AS_DLL -o $@ $<

all: sample1.exe sample1s.exe sample1d.exe
sample1.exe: sample1.o
	$(CXX) -g -o $@ $^
sample1s.exe: sample1.o cyglocale_patcher.o
	$(CXX) -g -o $@ $^
sample1d.o: sample1.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH_DLL -c -o $@ $<
sample1d.exe: sample1d.o cyglocale_patcher.dll
	$(CXX) -g -L . -o $@ $< -lcyglocale_patcher

clean:
	-rm -f *.o *.exe
