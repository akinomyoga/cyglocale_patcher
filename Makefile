# -*= mode: makefile-gmake -*-

CXX := g++
CXXFLAGS := -g

all: demangle impl0.exe impl1 impl2.dll impl2.exe
.PHONY: all

demangle: demangle.o
	g++ -g -o $@ $^

impl0.exe: impl0.o
	g++ -g -o $@ $^

impl1: impl1.o impl1patch.o
	g++ -g -o $@ $^

impl2.dll: impl2dll.o impl1patch.o
	g++ -g -shared -o $@ $^

impl2.exe: impl2.o
	g++ -g -L . -o $@ $^ -limpl2

all: libstdcxx_locale_patch.dll i4.exe
libstdcxx_locale_patch.dll: i4dll.cpp
	g++ -O2 -s -shared -o $@ $<
i4.exe: i4.o | libstdcxx_locale_patch.dll
	g++ -g -L . -o $@ $^ -lstdcxx_locale_patch
