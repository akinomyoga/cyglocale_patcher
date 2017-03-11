# cyglocale_patcher.cpp
Cygwin で一緒にリンクするだけで std::locale(locale_name) 及び std::codecvt に対応する (Cygwin x86 2.7.0 用)

ライセンスは GPLv3 (ランタイムライブラリ例外適用) です。

##使い方
普通に動かそうとしても Cygwin では動かないプログラムでも…
```cpp
// program.cpp (あなたのプログラムの例)
#include <locale>
#include <iostream>
int main() {
  std::ios_base::sync_with_stdio(false);
  std::wcout.imbue(std::locale(""));
  std::wcout << L"あいうえお1" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.UTF-8"));
  std::wcout << L"あいうえお2" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.cp932"));
  std::wcout << L"あいうえお3" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.EUC-JP"));
  std::wcout << L"あいうえお4" << std::endl;
  return 0;
}
```
```sh
[i686-pc-cygwin]$ g++ program.cpp && ./a.exe
terminate called after throwing an instance of 'std::runtime_error'
  what():  locale::facet::_S_create_c_locale name not valid
Aborted (コアダンプ)
```
`cyglocale_patcher.cpp` を一緒にリンクすれば (無理やり) 動く
```sh
[i686-pc-cygwin]$ g++ program.cpp cyglocale_patcher.cpp && ./a.exe
あいうえお1
あいうえお2
����������3 # ← sjis
����������4 # ← ujis
```
