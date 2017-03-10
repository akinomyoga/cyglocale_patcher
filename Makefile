# -*= mode: makefile-gmake -*-

CXX := g++
CXXFLAGS := -g

all: demangle sample1.exe sample1d.exe sample1s.exe
.PHONY: all

demangle: demangle.o
	g++ -g -o $@ $^

sample1.exe: sample1.o
	g++ -g -o $@ $^

all: libstdcxx_locale_patch.dll
libstdcxx_locale_patch.dll: i4dll.cpp
	g++ -O2 -s -shared -D USE_AS_DLL -o $@ $<

all: sample1d.exe
sample1d.o: sample1.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH_DLL -c -o $@ $<
sample1d.exe: sample1d.o
	$(CXX) -g -L . -o $@ $^ -lstdcxx_locale_patch

all: sample1s.exe
sample1s.o: sample1.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH -c -o $@ $<
sample1s.exe: sample1s.o i4dll.o
	$(CXX) -g -o $@ $^
