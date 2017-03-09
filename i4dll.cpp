/*?lwiki
 *
 * *概要
 *
 * Cygwin で `std::locale("")` や `std::locale("ja_JP.UTF-8")` をする。
 *
 * *使い方
 *
 * DLL は以下のようにして生成する。
 * &pre(!bash){
 * g++ -shared -O2 -s -o libstdcxx_locale_patch.dll i4dll.cpp
 * }
 *
 * プログラムの中では `use_libstdcxx_locale_patch()` (ダミーの関数) を何処かで呼び出す様にする。
 * &pre(!cpp,title=program.cpp){
 * int use_libstdcxx_locale_patch();
 * int main() {
 *   use_libstdcxx_locale_patch(); // 何処かで呼び出す。
 * }
 * }
 *
 * コンパイル時は `libstdcxx_locale_patch.dll` をリンクする。
 * &pre(!bash){
 * g++ -L . program.cpp -lstdcxx_locale_patch
 * }
 * 実行時は `libstdcxx_locale_patch.dll` が見つかる様にする。
 * コンパイル時に `-Wl,-RPATH,場所` を指定するか、
 * 環境変数 `LD_LIBRARY_PATH=場所:...` を指定するか、
 * 実行ファイルと同じディレクトリに .dll を置く。
 *
 * *経緯
 *
 * Cygwin では `std::locale("")` すると `std::runtime_error` になる。
 * `setlocale(LC_ALL, "");` は普通にできるのに納得がいかない。
 *
 * 問題がページ [1] に書かれているのと同じだとすれば、
 * libstdc++-v3/config/generic/c_locale.cc [2] が使われているのがいけない。
 * libstdc++-v3/config/gnu/c_locale.cc [3] の内容を使うようにすれば良い。
 * 問題の関数は cygstdc++-6.dll の中に入っている。
 * この DLL を適切な ./configure オプションでビルドし直して、
 * ソースコードを cygstdc++-6.dll と一緒に配布すれば良い気もするが、何か嫌だ。
 * 実のところ該当する関数だけ置き換えてリンクしてしまえば良いのではと考える。
 *
 * 所が、スタックトレースを見ると問題の関数は DLL (/usr/bin/cygstdc++-6.dll) の中で呼び出されている。
 *
 *   #12 0x498292c3 in cygstdc++-6!_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_ () from /usr/bin/cygstdc++-6.dll
 *   #13 0x49827569 in cygstdc++-6!_ZNSt6locale5_ImplC2EPKcj () from /usr/bin/cygstdc++-6.dll
 *   #14 0x49829ca8 in cygstdc++-6!_ZNSt6localeC2EPKc () from /usr/bin/cygstdc++-6.dll
 *   #15 0x0040129a in main () at hello.cpp:20
 *
 * 問題の関数を内部で呼び出しているような関数は DLL から沢山エクスポートされている。
 * それらを全て置き換えるのは非現実的だ。
 * 仕方がないので関数の実体を動的に書き換える事にする。以下の関数を置き換える。
 *
 * - `std::locale::facet::_S_create_c_locale(int*&, char const*, int*)`
 * - `std::locale::facet::_S_destroy_c_locale(int*&)`
 * - `std::locale::facet::_S_clone_c_locale(int*&)`
 * - `std::locale::facet::_S_lc_ctype_c_locale(int*, char const*)`
 *
 * 実際に置き換えてみるとエラーが発生する。
 * 幾つかの箇所で未初期化の値を `_S_destroy_c_locale` に渡している事が判明した。
 * これにより自分の確保した `locale_t` ではない壊れた `locale_t` が渡って問題になる。
 * 問題が発生しないように未初期化の変数をちゃんと初期化する様に、以下の関数にも修正が必要だった。
 *
 * - `std::__timepunct<char>::_M_initialize_timepunct(int*)`
 * - `std::ctype<char>::ctype(int*, char const*, bool, unsigned int)`
 * - `std::ctype<char>::ctype(char const*, bool, unsigned int)`
 *
 * しかしそれでも自前でビルドした DLL にしか有効でなかった。
 * Cygwin に付属の DLL では未だ未初期化の変数が残るようで segfault する。
 * 仕方がないので USE_LOCOBJ_COUNT というマクロを用意して、
 * このマクロが定義されている時は自分で確保したオブジェクトを記録する事にした。
 *
 *
 * [1] [[http://d.hatena.ne.jp/eagletmt/20090208/1234086332]]
 * [2] [[https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/config/locale/generic/c_locale.cc#L220]]
 * [3] [[https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/config/locale/gnu/c_locale.cc#L132]]
 *
 */

#include <stdio.h>
#include <locale.h>

#include <cstdio>
#include <cstring>
#include <locale>

#if (__GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ <= 2)
# ifndef LC_CTYPE_MASK
#  define LC_CTYPE_MASK (1 << LC_CTYPE)
# endif
#endif

/*?lwiki
 * @def #define USE_LOCOBJ_COUNT
 *
 * libstdc++-v3 自体に未初期化の変数を destroy に渡すという問題がある。
 * `USE_LOCOBJ_COUNT` を定義すると、対策として `_S_create_c_locale` で生成した `locale_t` を記録し、
 * `_S_destroy_c_locale` では記録されている `locale_t` のみを削除するようにする。
 * 然しこの方法は完全ではない。たまたま仕様中の `locale_t` が未初期化の値に入っている可能性もあり、
 * その場合、未だ仕様中の `locale_t` が削除されてしまうという事態になり危険である。
 *
 * `USE_LOCOBJ_COUNT` が定義されていない場合は、既知の未初期化の原因の関数について上書きを行う。
 * しかし、この方法は自前で生成した cygstdc++-6.dll に対しては有効であったが、
 * どうやら Cygwin 付属の cygstdc++-6.dll に対しては有効ではないようだ。
 * 恐らく生成に使っているソースコードが違っていて、
 * 別の箇所でも未初期化のデータが生産されているという事になる。
 *
 * ''libstdc++v3 の問題''
 *
 * もっと書くと Cygwin の構成では `ctype::_M_c_locale_ctype` 及び、
 * `__timepunct::_M_c_locale_timepunct` が未初期化のまま
 * `locale::facet::_S_destroy_c_locale` に渡されるが、
 * `_S_destroy_c_locale` 自体が空の実装なので何も起こらないという事である。
 * しかし、ここで `_S_destroy_c_locale` を置き換えると問題になるのである。
 *
 */
#define USE_LOCOBJ_COUNT

/*?lwiki
 * @def #define USE_PRINT_ALLOC
 *
 * 定義されている時、`_S_destroy_c_locale` に変な値が渡される問題をデバグする為に、
 * `locale_t` の確保・解放を逐一出力する事を示す。
 *
 */
//#define USE_PRINT_ALLOC

#ifdef USE_LOCOBJ_COUNT
# if __cplusplus >= 201103L
#  include <unordered_set>
static std::unordered_set<locale_t> allocatedList;
# else
#  include <set>
static std::set<locale_t> allocatedList;
# endif
#endif

namespace std {

  void locale::facet::_S_create_c_locale(__c_locale& locobj, const char* locstr, __c_locale base) {
    if (!(locobj = (__c_locale) ::newlocale(1 << LC_ALL, locstr, (::locale_t) base)))
      throw std::runtime_error("std::locale::facet::_S_create_c_locale(__c_local, const char*, __clocale): failed");
#ifdef USE_LOCOBJ_COUNT
    allocatedList.insert((locale_t) locobj);
#endif
#ifdef USE_PRINT_ALLOC
    std::fprintf(stderr, "new %p\n", locobj);
    std::fflush(stderr);
#endif
  }

  void locale::facet::_S_destroy_c_locale(__c_locale& locobj) {
    if (locobj && _S_get_c_locale() != locobj) {
#ifdef USE_LOCOBJ_COUNT
      if (allocatedList.erase((locale_t) locobj) == 0) return;
#endif
#ifdef USE_PRINT_ALLOC
      std::fprintf(stderr, "free %p\n", locobj);
      std::fflush(stderr);
#endif
      ::freelocale((::locale_t) locobj);
    }
  }

  __c_locale locale::facet::_S_clone_c_locale(__c_locale& locobj) throw() {
    ::locale_t const result = ::duplocale((::locale_t) locobj);
#ifdef USE_LOCOBJ_COUNT
    allocatedList.insert(result);
#endif
#ifdef USE_PRINT_ALLOC
    std::fprintf(stderr, "dup %p\n", result);
    std::fflush(stderr);
#endif
    return (__c_locale) result;
  }

  __c_locale locale::facet::_S_lc_ctype_c_locale(__c_locale locobj, const char* locstr) {
    ::locale_t const base = ::duplocale((::locale_t) locobj);
    if (!base) {
      throw std::runtime_error("std::locale::facet::_S_lc_ctype_c_locale(__clocale, const char*): failed in duplocale");
    }

    ::locale_t const result = ::newlocale(LC_CTYPE_MASK, locstr, base);
    if (!result) {
      ::freelocale(base);
      throw std::runtime_error("std::locale::facet::_S_lc_ctype_c_locale(__clocale, const char*): failed in newlocale");
    }

#ifdef USE_LOCOBJ_COUNT
    allocatedList.insert(result);
#endif
#ifdef USE_PRINT_ALLOC
    std::fprintf(stderr, "ctype %p\n", result);
    std::fflush(stderr);
#endif
    return (__c_locale) result;
  }

  template<>
  void __timepunct<char>::_M_initialize_timepunct(__c_locale) {
    // "C" locale.
    if (!_M_data) _M_data = new __timepunct_cache<char>;

    _M_data->_M_date_format = "%m/%d/%y";
    _M_data->_M_date_era_format = "%m/%d/%y";
    _M_data->_M_time_format = "%H:%M:%S";
    _M_data->_M_time_era_format = "%H:%M:%S";
    _M_data->_M_date_time_format = "";
    _M_data->_M_date_time_era_format = "";
    _M_data->_M_am = "AM";
    _M_data->_M_pm = "PM";
    _M_data->_M_am_pm_format = "";

    // Day names, starting with "C"'s Sunday.
    _M_data->_M_day1 = "Sunday";
    _M_data->_M_day2 = "Monday";
    _M_data->_M_day3 = "Tuesday";
    _M_data->_M_day4 = "Wednesday";
    _M_data->_M_day5 = "Thursday";
    _M_data->_M_day6 = "Friday";
    _M_data->_M_day7 = "Saturday";

    // Abbreviated day names, starting with "C"'s Sun.
    _M_data->_M_aday1 = "Sun";
    _M_data->_M_aday2 = "Mon";
    _M_data->_M_aday3 = "Tue";
    _M_data->_M_aday4 = "Wed";
    _M_data->_M_aday5 = "Thu";
    _M_data->_M_aday6 = "Fri";
    _M_data->_M_aday7 = "Sat";

    // Month names, starting with "C"'s January.
    _M_data->_M_month01 = "January";
    _M_data->_M_month02 = "February";
    _M_data->_M_month03 = "March";
    _M_data->_M_month04 = "April";
    _M_data->_M_month05 = "May";
    _M_data->_M_month06 = "June";
    _M_data->_M_month07 = "July";
    _M_data->_M_month08 = "August";
    _M_data->_M_month09 = "September";
    _M_data->_M_month10 = "October";
    _M_data->_M_month11 = "November";
    _M_data->_M_month12 = "December";

    // Abbreviated month names, starting with "C"'s Jan.
    _M_data->_M_amonth01 = "Jan";
    _M_data->_M_amonth02 = "Feb";
    _M_data->_M_amonth03 = "Mar";
    _M_data->_M_amonth04 = "Apr";
    _M_data->_M_amonth05 = "May";
    _M_data->_M_amonth06 = "Jun";
    _M_data->_M_amonth07 = "Jul";
    _M_data->_M_amonth08 = "Aug";
    _M_data->_M_amonth09 = "Sep";
    _M_data->_M_amonth10 = "Oct";
    _M_data->_M_amonth11 = "Nov";
    _M_data->_M_amonth12 = "Dec";
  }

  ctype<char>::ctype(__c_locale, const mask* table, bool del, size_t refs)
    : facet(refs),
      _M_c_locale_ctype(_S_get_c_locale()),
      _M_del(table != 0 && del),
      _M_toupper(NULL),
      _M_tolower(NULL),
      _M_table(table ? table : classic_table())
  {
    std::memset(_M_widen, 0, sizeof(_M_widen));
    _M_widen_ok = 0;
    std::memset(_M_narrow, 0, sizeof(_M_narrow));
    _M_narrow_ok = 0;
  }

  ctype<char>::ctype(const mask* table, bool del, size_t refs)
    : facet(refs),
      _M_c_locale_ctype(_S_get_c_locale()),
      _M_del(table != 0 && del),
      _M_toupper(NULL),
      _M_tolower(NULL),
      _M_table(table ? table : classic_table())
  {
    std::memset(_M_widen, 0, sizeof(_M_widen));
    _M_widen_ok = 0;
    std::memset(_M_narrow, 0, sizeof(_M_narrow));
    _M_narrow_ok = 0;
  }
}

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <windows.h>

namespace {

  static void atomic_write64(uint64_t volatile* const target, uint64_t const value) {
#if !defined(__clang__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) || defined(__clang__)
    //
    // __sync_bool_compare_and_swap を使う
    //
    // * gcc の場合はマニュアルに 4.1.0 以降に少なくとも載っている [1]。
    //   4.0.0 のマニュアルの目次 [2] で builtin のある頁を探して見てみたがなかった。
    //   atomic や lock が含まれる見出しはなかった。
    //   もしかすると何処かにあるのかもしれないが取り敢えずない物と考える。
    //
    //   [1] https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html
    //   [2] https://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/
    //
    // * clang では実装されたのが 2009/4 [3] であり Clang 1.0 リリースが 2009/10 [4] なので多分大丈夫。
    //
    //   [3] https://github.com/Microsoft/clang/commit/0002d23aaf10f307273dab5facda01c137283d22
    //   [4] https://ja.wikipedia.org/wiki/Clang
    //
    while (!__sync_bool_compare_and_swap(target, *target, value));
#elif defined(__CYGWIN64__)
    //
    // 64 bit 環境ではアラインされている限り直接書いて大丈夫?
    //
    *target = value;
#elif WINVER >= 0x502
    //
    // InterlockedExchange64 を使う
    //
    // * Win Server 2003, Win Vista 以降で使える[5]。
    //   少なくとも WINVER >= 0x502 [6] でなければ使えない。
    //
    //   [5] https://msdn.microsoft.com/ja-jp/library/windows/desktop/ms683593(v=vs.85).aspx
    //   [6] https://msdn.microsoft.com/ja-jp/library/6sehtctf.aspx
    //
    //   #define WINVER       0x0502
    //   #define _WIN32_WINNT 0x0502
    //   #include <windows.h>
    //
    //   の様にしても良いかもしれないがそれだと XP で実行できなくなる。
    //
    ::InterlockedExchange(target, value);
#else
    //
    // ノーガードで書く。まあ大丈夫だろう。
    //
    *target = value;
#endif
  }

  static bool patch_function(void* ptarget, void* preplace) {
    bool result = false;

    // 16 bytes で align されているっぽいので仮定してしまう。
    if (ptarget && (uintptr_t) ptarget % 8 == 0) {
      DWORD old_protect;
      ::VirtualProtect((LPVOID) ptarget, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect);
      int32_t const distance32 = (int32_t) ((intptr_t) preplace - (intptr_t) ptarget - 5);
      if ((intptr_t) preplace == ((intptr_t) ptarget + 5) + distance32) {
        uint64_t* const ptarget64 = (uint64_t*) ptarget;
        union { uint64_t intval; unsigned char data[8]; } body = { *ptarget64 };
        body.data[0] = 0xE9; // JMP
        memcpy(body.data + 1, &distance32, sizeof(distance32));
        atomic_write64(ptarget64, body.intval);
        result = true;
      }
      ::VirtualProtect((LPVOID) ptarget, sizeof(void*), old_protect, &old_protect);
    }

    return result;
  }

  static void error_throw() {
    throw std::runtime_error("failed to patch libstdc++-6 locale");
  }
  static void error_exit(const char* name) {
    // 中途半端な状態で失敗したらもう死ぬしか無い。
    std::fprintf(stderr, "libstdcxx_locale_patch (%s): failed to patch; exiting\n", name);
    std::exit(EXIT_FAILURE);
  }

  static bool patch_symbol(HMODULE targetDll, HMODULE replaceDll, const char* symbol, bool forceUpdate = true) {
    FARPROC const targetProc  = ::GetProcAddress(targetDll , symbol);
    FARPROC const replaceProc = ::GetProcAddress(replaceDll, symbol);
    if (!targetProc) return !forceUpdate;
    if (!replaceProc) return false;
    return patch_function((void*) targetProc, (void*) replaceProc);
  }

  static void patch_libstdcxx_locale(HMODULE hInstanceDll) {
    ::HMODULE const cygstdcxxDll = ::GetModuleHandle("cygstdc++-6.dll");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_")) error_throw();
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt6locale5facet19_S_destroy_c_localeERPi"     )) error_exit("_S_destroy_c_locale");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt6locale5facet17_S_clone_c_localeERPi"       )) error_exit("_S_clone_c_locale");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc"  ), false) error_exit("_S_lc_ctype_c_locale");

#ifdef USE_LOCOBJ_COUNT
    // USE_LOCOBJ_COUNT であっても、しないよりはする方がまし。
    patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt11__timepunctIcE23_M_initialize_timepunctEPi");
    patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC1EPiPKcbj");
    patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC2EPiPKcbj");
    patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC1EPKcbj"  );
    patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC2EPKcbj"  );
#else
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt11__timepunctIcE23_M_initialize_timepunctEPi")) error_exit("__timepunct::__timepunct");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC1EPiPKcbj")) error_exit("ctype::ctype");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC2EPiPKcbj")) error_exit("ctype::ctype");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC1EPKcbj"  )) error_exit("ctype::ctype");
    if (!patch_symbol(cygstdcxxDll, hInstanceDll, "_ZNSt5ctypeIcEC2EPKcbj"  )) error_exit("ctype::ctype");
#endif
  }
}

extern "C" BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    patch_libstdcxx_locale(hinstDLL);
    break;
  }
  return TRUE;
}

int use_libstdcxx_locale_patch() {return 0;}
