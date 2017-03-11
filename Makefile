# -*= mode: makefile-gmake -*-

CXX := g++
CXXFLAGS := -g

all: demangle sample1.exe sample1d.exe sample1s.exe
.PHONY: all

demangle: demangle.o
	g++ -g -o $@ $^

sample1.exe: sample1.o
	g++ -g -o $@ $^

all: cyglocale_patcher.dll
cyglocale_patcher.dll: cyglocale_patcher.cpp
	g++ -O2 -s -shared -D USE_AS_DLL -o $@ $<

all: sample1d.exe
sample1d.o: sample1.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH_DLL -c -o $@ $<
sample1d.exe: sample1d.o cyglocale_patcher.dll
	$(CXX) -g -L . -o $@ $< -lcyglocale_patcher

all: sample1s.exe
sample1s.o: sample1.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH -c -o $@ $<
sample1s.exe: sample1s.o cyglocale_patcher.o
	$(CXX) -g -o $@ $^
