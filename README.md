# cyglocale_patcher.cpp
Cygwin で一緒にリンクするだけで std::locale(locale_name) 及び std::codecvt に対応し duplocale のバグも修正する (Cygwin x86 2.7.0 用)

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

##仕組み (dirty hack)

基本、メモリ上にロードされた DLL のイメージにある関数を書き換える (hot patching)。
- cygstdc++-6.dll: JMP 命令による API hooking で 10-14 個の関数を置換。
- cygwin1.dll: duplocale 内の "call __loadlocale" 命令の行き先を書き換えて修正。
- ダミーの static 変数を定義することで初期化時に処理を実行。

クラッシュする危険性は 0 ではない。と思う。
