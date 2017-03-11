#include <stdlib.h>
#include <locale>
#include <iostream>
#include <stdexcept>

void test1() {
  std::locale("");
  std::locale("ja_JP.UTF-8");
  std::locale("ja_JP.cp932");
  std::locale("ja_JP.EUC-JP");
}

void try_locale(const char* locstr) {
  try {
    std::locale(locstr);
  } catch (std::runtime_error&) {
    std::cerr << "failed in std::locale(\"" << locstr << "\")" << std::endl;
  }
}

void test1b() {
  std::cerr << "setting env to \"ja_JP.UTF-8\"" << std::endl;
  ::setenv("LC_COLLATE",  "ja_JP.UTF-8", 1);
  ::setenv("LC_CTYPE",    "ja_JP.UTF-8", 1);
  ::setenv("LC_MESSAGES", "ja_JP.UTF-8", 1);
  ::setenv("LC_MONETARY", "ja_JP.UTF-8", 1);
  ::setenv("LC_NUMERIC",  "ja_JP.UTF-8", 1);
  ::setenv("LC_TIME",     "ja_JP.UTF-8", 1);
  try_locale("");

  std::cerr << "setting env to \"\"" << std::endl;
  ::setenv("LC_COLLATE",  "", 1);
  ::setenv("LC_CTYPE",    "", 1);
  ::setenv("LC_MESSAGES", "", 1);
  ::setenv("LC_MONETARY", "", 1);
  ::setenv("LC_NUMERIC",  "", 1);
  ::setenv("LC_TIME",     "", 1);
  try_locale("");
}

void test1c() {
  try_locale("ja_JP.UTF-8");
  try_locale(".UTF-8");
  try_locale("Japanese_Japan.65001");
  try_locale("ja-JP");
  try_locale(".65001");
}

void test2full() {
  std::ios_base::sync_with_stdio(false);

  std::setlocale(LC_CTYPE, "");
  std::wcout.imbue(std::locale(""));
  std::wcout << L"あいうえお1" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.UTF-8");
  std::wcout.imbue(std::locale("ja_JP.UTF-8"));
  std::wcout << L"あいうえお2" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.cp932");
  std::wcout.imbue(std::locale("ja_JP.cp932"));
  std::wcout << L"あいうえお3" << std::endl;

  std::setlocale(LC_CTYPE, "ja_JP.EUC-JP");
  std::wcout.imbue(std::locale("ja_JP.EUC-JP"));
  std::wcout << L"あいうえお4" << std::endl;
}

void test2setlocale() {
  std::ios_base::sync_with_stdio(false);
  std::setlocale(LC_CTYPE, "");
  std::wcout << L"あいうえお1" << std::endl;
  std::setlocale(LC_CTYPE, "ja_JP.UTF-8");
  std::wcout << L"あいうえお2" << std::endl;
  std::setlocale(LC_CTYPE, "ja_JP.cp932");
  std::wcout << L"あいうえお3" << std::endl;
  std::setlocale(LC_CTYPE, "ja_JP.EUC-JP");
  std::wcout << L"あいうえお4" << std::endl;
}

void test2imbue() {
  std::ios_base::sync_with_stdio(false);
  std::wcout.imbue(std::locale(""));
  std::wcout << L"あいうえお1" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.UTF-8"));
  std::wcout << L"あいうえお2" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.cp932"));
  std::wcout << L"あいうえお3" << std::endl;
  std::wcout.imbue(std::locale("ja_JP.EUC-JP"));
  std::wcout << L"あいうえお4" << std::endl;
}

#include <cstdio>
#include <string>

void test3_write(std::wstring const& ws, std::locale const& loc) {
  typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_t;
  static const int buffer_size = 0x100;

  codecvt_t const& cvt = std::use_facet<codecvt_t>(loc);
  std::mbstate_t state = {0};
  char buffer[buffer_size];

  wchar_t const* const src0 = &ws[0];
  wchar_t const* const srcN = &ws[0] + ws.size();
  char* const dst0 = buffer;
  char* const dstN = buffer + buffer_size;
  wchar_t const* src = src0;
  char* dst = dst0;

  while (src < srcN) {
    codecvt_t::result result = cvt.out(state, src, srcN, src, dst, dstN, dst);
    bool processed = false;
    if (dst > dst0) {
      std::fwrite(dst0, sizeof(char), dst - dst0, stdout);
      dst = dst0;
      processed = true;
    }
    if (result == codecvt_t::error) {
      std::fputc('?', stdout);
      src++;
      processed = true;
    }
    if (!processed) {
      std::printf("(error)");
      break;
    }
  }
}
void test3() {
  test3_write(L"あいうえお1", std::locale(""));
  std::fputc('\n', stdout);
  test3_write(L"あいうえお2", std::locale("ja_JP.UTF-8"));
  std::fputc('\n', stdout);
  test3_write(L"あいうえお3", std::locale("ja_JP.EUC-JP"));
  std::fputc('\n', stdout);
  test3_write(L"あいうえお4", std::locale("ja_JP.cp932"));
  std::fputc('\n', stdout);
}

int main() {
#ifdef USE_PATCH_DLL
  bool use_libstdcxx_locale_patch();
  use_libstdcxx_locale_patch();
#endif

  //test2full();
  //test2setlocale();
  test2imbue(); // basic_filebuf::_M_convert_to_external conversion error
  //test3();

  return 0;
}
