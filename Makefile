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
	g++ -O2 -s -shared -D USE_AS_DLL -o $@ $<
i4.exe: i4.o | libstdcxx_locale_patch.dll
	g++ -g -L . -o $@ $^ -lstdcxx_locale_patch

all: i1d.exe
i1d.o: impl0.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH_DLL -c -o $@ $<
i1d.exe: i1d.o
	$(CXX) -g -L . -o $@ $^ -lstdcxx_locale_patch

all: i1s.exe
i1s.o: impl0.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -D USE_PATCH -c -o $@ $<
i1s.exe: i1s.o i4dll.o
	$(CXX) -g -o $@ $^
