#!/bin/bash

[[ -d ext ]] || mkdir -p ext
cd ext
[[ -s gcc-5.4.0.tar.gz ]] || wget https://ftp.gnu.org/gnu/gcc/gcc-5.4.0/gcc-5.4.0.tar.gz

# gcc の中の libstdc++-v3 を修正してコンパイルする。
tar xzvf gcc-5.4.0.tar.gz --exclude=gcc/testsuite --exclude=libjava
cd gcc-5.4.0/libstdc++-v3
CFLAGS='-O2 -g' CXXFLAGS='-O2 -g' ./configure --prefix=$HOME/opt/libstdc++-6

# configure の一番最後で失敗するので
# ./config.status の multi_basedir を修正して再度実行する
sed '\|^multi_basedir="\./\.\./\.\."|s|\./\.\./\.\.|./..|' -i.bk config.status
./config.status

# 一旦 make してから修正を行う。
make -j4 # fork の問題か途中で失敗するかもしれないが実行しなおせば良い。
for f in src/c++98/c++locale.cc include/bits/time_members.h src/c++11/ctype_configure_char.cc; do
  mv "$f" "$f.bk"
  cp -L "$f.bk" "$f"
done
patch -p 1 < ../../libstdc++-v3-001.patch
patch -p 1 < ../../libstdc++-v3-002.patch

# 再度 make する。
make clean
make -j4
cp src/.libs/cygstdc++-6.dll ../..
