#include <locale>
#include <iostream>

void test1() {
  std::locale("");
}

void test2() {
  std::ios_base::sync_with_stdio(false);

  std::setlocale(LC_CTYPE, "");
  std::wcout.imbue(std::locale(""));
  std::wcout << L"あいうえお1" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.UTF-8");
  std::wcout.imbue(std::locale("ja_JP.UTF-8"));
  std::wcout << L"あいうえお2" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.cp932");
  std::wcout.imbue(std::locale("ja_JP.cp932"));
  std::wcout << L"あいうえお" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.EUC-JP");
  std::wcout.imbue(std::locale("ja_JP.EUC-JP"));
  std::wcout << L"あいうえお" << std::endl;
}

void test3() {
  std::locale("ja_JP.cp932");
  std::locale("ja_JP.EUC-JP");
}

int use_libstdcxx_locale_patch();

int main() {
  use_libstdcxx_locale_patch();
  test2();
  //test3();
  return 0;
}
