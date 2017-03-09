#include <iostream>

void patch_libstdcxx_locale();

int main() {
  patch_libstdcxx_locale();
  //std::locale("");
  //std::locale("hoge");

  std::ios_base::sync_with_stdio(false);

  std::locale("");
  std::locale("ja_JP.UTF-8");
  std::locale("ja_JP.cp932");
  return 0;
  std::locale("ja_JP.EUC-JP");

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

  return 0;
}
