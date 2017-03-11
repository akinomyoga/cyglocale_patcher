/*?lwiki
 *
 * *概要
 *
 * Cygwin で `std::locale("")` や `std::locale("ja_JP.UTF-8")` をする。
 *
 * *使い方 (DLL にして使う場合)
 *
 * DLL は以下のようにして生成する。
 * &pre(!bash){
 * g++ -shared -O2 -s -D USE_AS_DLL -o libstdcxx_locale_patch.dll patcher.cpp
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
 * コンパイル時に `-Wl,-rpath,場所` を指定するか、
 * 環境変数 `LD_LIBRARY_PATH=場所:...` を指定するか、
 * 実行ファイルと同じディレクトリに .dll を置く。
 *
 * *使い方 (静的リンクして使う場合)
 *
 * この方法を取る場合は `USE_LOCOBJ_COUNT` は定義されていなければならない。
 * 普通にコンパイルして .o ファイルを得る。
 * main のプログラムでは以下のようにする。
 * &pre(!cpp,title=program.cpp){
 * int main() {
 *   int patch_libstdcxx_locale();
 *   patch_libstdcxx_locale(); // 一番最初に呼び出す。
 *   // 中身
 * }
 * }
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
 * 所が、スタックトレースを見ると問題の関数は DLL (/usr/bin/cygstdc++-6.dll) の内部で呼び出されている。
 *
 *   #12 0x498292c3 in cygstdc++-6!_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_ () from /usr/bin/cygstdc++-6.dll
 *   #13 0x49827569 in cygstdc++-6!_ZNSt6locale5_ImplC2EPKcj () from /usr/bin/cygstdc++-6.dll
 *   #14 0x49829ca8 in cygstdc++-6!_ZNSt6localeC2EPKc () from /usr/bin/cygstdc++-6.dll
 *   #15 0x0040129a in main () at hello.cpp:20
 *
 * 問題の関数を内部で呼び出しているような関数は DLL から沢山エクスポートされている。
 * それらを全て置き換えるのは非現実的だ。
 * 仕方がないので関数の実体の方を書き換える事にする。以下の関数を置き換える。
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
#include <stdlib.h>
#include <limits.h>
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
 * @def #define DEBUG_PRINT_LOCALE
 *
 * 定義されている時、`_S_destroy_c_locale` に変な値が渡される問題をデバグする為に、
 * `locale_t` の確保・解放を逐一出力する事を示す。
 *
 */
//#define DEBUG_PRINT_LOCALE
//#define DEBUG_PATCH_DUPLOCALE

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
    allocatedList.erase((locale_t) base);
    allocatedList.insert((locale_t) locobj);
#endif
#ifdef DEBUG_PRINT_LOCALE
    std::fprintf(stderr, "new %p <-(%s)- %p\n", locobj, locstr, base);
    std::fflush(stderr);
#endif
  }

  void locale::facet::_S_destroy_c_locale(__c_locale& locobj) {
    if (locobj && _S_get_c_locale() != locobj) {
#ifdef USE_LOCOBJ_COUNT
      if (allocatedList.erase((locale_t) locobj) == 0) {
# ifdef DEBUG_PRINT_LOCALE
        std::fprintf(stderr, "ignore free %p\n", locobj);
        std::fflush(stderr);
# endif
        return;
      }
#endif
#ifdef DEBUG_PRINT_LOCALE
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
#ifdef DEBUG_PRINT_LOCALE
    std::fprintf(stderr, "dup %p <- %p\n", result, locobj);
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
#ifdef DEBUG_PRINT_LOCALE
    std::fprintf(stderr, "ctype %p <-(%s)- %p\n", result, locstr, locobj);
    std::fflush(stderr);
#endif
    return (__c_locale) result;
  }

  codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_out(
    state_type& state, const intern_type* src0,
    const intern_type* srcN, const intern_type*& src,
    extern_type* dst0, extern_type* dstN,
    extern_type*& dst) const
  {
    result ret = ok;
    state_type tmp_state(state);

    locale_t const old = uselocale((locale_t) _M_c_locale_codecvt);

    for (src = src0, dst = dst0; src < srcN && dst < dstN && ret == ok;) {
      const intern_type* srcNext = wmemchr(src, L'\0', srcN - src);
      if (!srcNext) srcNext = srcN;

      src0 = src;
      const size_t conv = wcsnrtombs(dst, &src, srcNext - src, dstN - dst, &state);
      if (conv == static_cast<size_t>(-1)) {
        for (; src0 < src; ++src0)
          dst += wcrtomb(dst, *src0, &tmp_state);
        state = tmp_state;
        ret = error;
      } else if (src && src < srcNext) {
        dst += conv;
        ret = partial;
      } else {
        src = srcNext;
        dst += conv;
      }

      if (src < srcN && ret == ok) {
        extern_type buf[MB_LEN_MAX];
        tmp_state = state;
        const size_t conv2 = wcrtomb(buf, *src, &tmp_state);
        if (conv2 > static_cast<size_t>(dstN - dst))
          ret = partial;
        else {
          memcpy(dst, buf, conv2);
          state = tmp_state;
          dst += conv2;
          ++src;
        }
      }
    }

    uselocale(old);

    return ret;
  }

  codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_in(
    state_type& state, const extern_type* src0,
    const extern_type* srcN, const extern_type*& src,
    intern_type* dst0, intern_type* dstN,
    intern_type*& dst) const
  {
    result ret = ok;
    state_type tmp_state(state);

    locale_t const old = uselocale((locale_t) _M_c_locale_codecvt);

    for (src = src0, dst = dst0; src < srcN && dst < dstN && ret == ok;) {
      const extern_type* srcNext;
      srcNext = (const extern_type*) memchr(src, '\0', srcN - src);
      if (!srcNext) srcNext = srcN;

      src0 = src;
      size_t conv = mbsnrtowcs(dst, &src, srcNext - src, dstN - dst, &state);
      if (conv == (size_t) -1) {
        for (;; ++dst, src0 += conv) {
          conv = mbrtowc(dst, src0, srcN - src0, &tmp_state);
          if (conv == (size_t) -1 || conv == (size_t) -2) break;
        }
        src = src0;
        state = tmp_state;
        ret = error;
      } else if (src && src < srcNext) {
        dst += conv;
        ret = partial;
      } else {
        src = srcNext;
        dst += conv;
      }

      if (src < srcN && ret == ok) {
        if (dst < dstN) {
          tmp_state = state;
          ++src;
          *dst++ = L'\0';
        } else
          ret = partial;
      }
    }

    uselocale(old);

    return ret;
  }

  int codecvt<wchar_t, char, mbstate_t>::do_encoding() const throw() {
    int ret = 0;
    locale_t const old = uselocale((locale_t) _M_c_locale_codecvt);
    if (MB_CUR_MAX == 1) ret = 1; // Note: MB_CUR_MAX depends on current locale
    uselocale(old);
    return ret;
  }

  int codecvt<wchar_t, char, mbstate_t>::do_max_length() const throw() {
#ifdef DEBUG_PRINT_LOCALE
    std::fprintf(stderr, "use %p\n", _M_c_locale_codecvt);
    std::fflush(stderr);
#endif
    locale_t const old = uselocale((locale_t) _M_c_locale_codecvt);
    int const ret = MB_CUR_MAX;
    uselocale(old);
    return ret;
  }

  int codecvt<wchar_t, char, mbstate_t>::do_length(
    state_type& state, const extern_type* src, const extern_type* srcN, size_t max) const
  {
    int ret = 0;
    state_type tmp_state(state);

    locale_t old = uselocale((locale_t) _M_c_locale_codecvt);

    wchar_t* dst = static_cast<wchar_t*>(__builtin_alloca(sizeof(wchar_t) * max));
    while (src < srcN && max) {
      const extern_type* srcNext;
      srcNext = (const extern_type*) memchr(src, '\0', srcN - src);
      if (!srcNext) srcNext = srcN;

      const extern_type* tmp_src = src;
      size_t conv = mbsnrtowcs(dst, &src, srcNext - src, max, &state);
      if (conv == (size_t) -1) {
        for (src = tmp_src;; src += conv) {
          conv = mbrtowc(0, src, srcN - src, &tmp_state);
          if (conv == (size_t) -1 || conv == (size_t) -2) break;
        }
        state = tmp_state;
        ret += src - tmp_src;
        break;
      }
      if (!src) src = srcNext;

      ret += src - tmp_src;
      max -= conv;

      if (src < srcN && max) {
        tmp_state = state;
        ++src;
        ++ret;
        --max;
      }
    }

    uselocale(old);

    return ret;
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
#include <sstream>
#include <stdint.h>
#include <windows.h>

namespace {

  void atomic_write64(uint64_t volatile* const target, uint64_t const value) {
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

  bool patch_function(void* ptarget, void* preplace) {
    bool result = false;

    // 16 bytes で align されているっぽいので仮定してしまう。
    if (ptarget && (uintptr_t) ptarget % 8 == 0) {
      DWORD old_protect;
      ::VirtualProtect((LPVOID) ptarget, 8, PAGE_EXECUTE_READWRITE, &old_protect);
      int32_t const distance32 = (int32_t) ((intptr_t) preplace - (intptr_t) ptarget - 5);
      if ((intptr_t) preplace == ((intptr_t) ptarget + 5) + distance32) {
        uint64_t* const ptarget64 = (uint64_t*) ptarget;
        union { uint64_t intval; unsigned char data[8]; } body = { *ptarget64 };
        body.data[0] = 0xE9; // JMP
        memcpy(body.data + 1, &distance32, sizeof(distance32));
        atomic_write64(ptarget64, body.intval);
        result = true;
      }
      ::VirtualProtect((LPVOID) ptarget, 8, old_protect, &old_protect);
    }
    // else std::fprintf(stderr, "tgt=%p rep=%p\n", ptarget, preplace);

    return result;
  }

  class dll_patcher {
    int patch_count;
    const char* dllName;
    HMODULE const targetDll;

  public:
    dll_patcher(const char* dllName):
      patch_count(0),
      dllName(dllName),
      targetDll(::GetModuleHandle(dllName)) {}

  private:
    void check(const char* symbol, bool result) {
      patch_count++;
      if (!result) {
        if (patch_count == 1) {
          std::ostringstream ostr;
          ostr << "patcher (" << dllName << "!" << symbol << "): failed";
          throw std::runtime_error(ostr.str());
        } else {
          // 中途半端な状態で失敗したらもう死ぬしか無い。
          std::fprintf(stderr, "patcher (%s!%s): failed to patch; exiting\n", dllName, symbol);
          std::exit(EXIT_FAILURE);
        }
      }
    }

  public:
    template<typename F>
    void patch_symbol(const char* symbol, F* replaceProc, bool forceUpdate = true) {
      if (!targetDll) return; // no target dll: not cygwin?
      bool done;
      FARPROC const targetProc = ::GetProcAddress(targetDll, symbol);
      if (!targetProc)
        done = !forceUpdate;
      else
        done = ::patch_function((void*) targetProc, (void*) replaceProc);
      check(symbol, done);
    }
    template<typename T, typename C>
    void patch_symbol(const char* symbol, T C::* replaceProc, bool forceUpdate = true) {
      union { T C::*original; void* transformed; } proc = { replaceProc };
      patch_symbol(symbol, proc.transformed, forceUpdate);
    }
    bool patch_symbol(const char* symbol, HMODULE replaceDll, bool forceUpdate = true) {
      if (!targetDll) return true; // no target dll: not cygwin?
      bool done;
      FARPROC const targetProc  = ::GetProcAddress(targetDll, symbol);
      FARPROC const replaceProc = ::GetProcAddress(replaceDll  , symbol);
      if (!targetProc)
        done = !forceUpdate;
      else if (!replaceProc)
        done = false;
      else
        done = patch_function((void*) targetProc, (void*) replaceProc);
      check(symbol, done);
    }
  };

  struct patch_libstdcxx_locale_impl1: std::locale::facet {
    typedef std::locale::facet base;
    static void patch(dll_patcher& patcher, HMODULE hInstanceDll) {
      patcher.patch_symbol("_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_", hInstanceDll);
      patcher.patch_symbol("_ZNSt6locale5facet19_S_destroy_c_localeERPi"     , hInstanceDll);
      patcher.patch_symbol("_ZNSt6locale5facet17_S_clone_c_localeERPi"       , hInstanceDll);
      patcher.patch_symbol("_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc"  , hInstanceDll, false); // 何故か拾えない?
    }
    static void patch(dll_patcher& patcher) {
      patcher.patch_symbol("_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_", &base::_S_create_c_locale  );
      patcher.patch_symbol("_ZNSt6locale5facet19_S_destroy_c_localeERPi"     , &base::_S_destroy_c_locale );
      patcher.patch_symbol("_ZNSt6locale5facet17_S_clone_c_localeERPi"       , &base::_S_clone_c_locale   );
      patcher.patch_symbol("_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc"  , &base::_S_lc_ctype_c_locale, false);
    }
  };
  struct patch_libstdcxx_locale_impl3: std::codecvt<wchar_t, char, mbstate_t> {
    typedef std::codecvt<wchar_t, char, mbstate_t> base;
    typedef patch_libstdcxx_locale_impl3 self;
    static void patch(dll_patcher& patcher, HMODULE hInstanceDll) {
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE6do_outERS0_PKwS4_RS4_PcS6_RS6_", hInstanceDll);
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE5do_inERS0_PKcS4_RS4_PwS6_RS6_" , hInstanceDll);
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE11do_encodingEv"                , hInstanceDll);
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE13do_max_lengthEv"              , hInstanceDll);
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE9do_lengthERS0_PKcS4_j"         , hInstanceDll);
    }

    // どうも protected なメンバ関数のアドレスは取れない様だ。
    // 呼び出せはするのでこのクラスにメンバ関数を実装する。
    codecvt_base::result my_out(
      state_type& state,
      const intern_type* src0, const intern_type* srcN, const intern_type*& src,
      extern_type* dst0, extern_type* dstN, extern_type*& dst) const
    {
      return base::do_out(state, src0, srcN, src, dst0, dstN, dst);
    }
    codecvt_base::result my_in(
      state_type& state,
      const extern_type* src0, const extern_type* srcN, const extern_type*& src,
      intern_type* dst0, intern_type* dstN, intern_type*& dst) const
    {
      return base::do_in(state, src0, srcN, src, dst0, dstN, dst);
    }
    int my_encoding() const throw() { return base::do_encoding(); }
    int my_max_length() const throw() { return base::do_max_length(); }
    int my_length(state_type& state, const extern_type* src, const extern_type* srcN, size_t max) const {
      return base::do_length(state, src, srcN, max);
    }

    static void patch(dll_patcher& patcher) {
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE6do_outERS0_PKwS4_RS4_PcS6_RS6_", &self::my_out       );
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE5do_inERS0_PKcS4_RS4_PwS6_RS6_" , &self::my_in        );
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE11do_encodingEv"                , &self::my_encoding  );
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE13do_max_lengthEv"              , &self::my_max_length);
      patcher.patch_symbol("_ZNKSt7codecvtIwc10_mbstate_tE9do_lengthERS0_PKcS4_j"         , &self::my_length    );
    }
  };

  struct patch_libstdcxx_locale_impl2: std::__timepunct<char> {
    typedef std::__timepunct<char> base;
    typedef patch_libstdcxx_locale_impl2 self;

#ifdef USE_LOCOBJ_COUNT
    // USE_LOCOBJ_COUNT であっても、しないよりはする方がまし。
    static const bool force_update = false;
#else
    static const bool force_update = false;
#endif

    static void patch(dll_patcher& patcher, HMODULE hInstanceDll) {
      // USE_LOCOBJ_COUNT であっても、しないよりはする方がまし。
      patcher.patch_symbol("_ZNSt11__timepunctIcE23_M_initialize_timepunctEPi", hInstanceDll, force_update);
      patcher.patch_symbol("_ZNSt5ctypeIcEC1EPiPKcbj", hInstanceDll, force_update);
      patcher.patch_symbol("_ZNSt5ctypeIcEC2EPiPKcbj", hInstanceDll, force_update);
      patcher.patch_symbol("_ZNSt5ctypeIcEC1EPKcbj"  , hInstanceDll, force_update);
      patcher.patch_symbol("_ZNSt5ctypeIcEC2EPKcbj"  , hInstanceDll, force_update);
    }

    void my_initialize_timepunct(std::__c_locale arg) {
      return base::_M_initialize_timepunct(arg);
    }

    static void patch(dll_patcher& patcher) {
      patcher.patch_symbol("_ZNSt11__timepunctIcE23_M_initialize_timepunctEPi", &self::my_initialize_timepunct, force_update);

      // ※残念ながらコンストラクタへのポインタは取得できない。
      // ctype<char>::ctype(__c_locale, const mask* table, bool del, size_t refs)
      // ctype<char>::ctype(const mask* table, bool del, size_t refs)
    }
  };

  struct scoped_virtual_protect {
    LPVOID addr;
    DWORD size;
    DWORD old_protect;
    scoped_virtual_protect(void* addr, DWORD size, DWORD protect): addr(addr), size(size) {
      ::VirtualProtect((LPVOID) addr, size, protect, &old_protect);
    }
    ~scoped_virtual_protect() {
      ::VirtualProtect((LPVOID) addr, size, old_protect, &old_protect);
    }
  };

  typedef unsigned char byte;
  typedef char* (loadlocale_t)(locale_t loc, int category, const char* new_locstr);

  loadlocale_t* original_loadlocale = 0;
  char* my_loadlocale(locale_t loc, int category, const char * new_locstr) {
#ifdef DEBUG_PATCH_DUPLOCALE
    std::fprintf(stderr, "__loadlocale %p\n", loc);
#endif

    std::string const save = new_locstr;
    const_cast<char*>(new_locstr)[0] = '\0';
    return original_loadlocale(loc, category, &save[0]);
  }

  // 関数 func の内容が data と一致しているか確認して、
  // 一致していたら func の続きを指すポインタを取得する。
  // 一致していなかったら 0 を返す。
  // data の要素が 0xCC の値の時はその要素は一致しなくて良いとする。
  template<int N>
  uint32_t* skip_checked_prefix(byte* func, byte (&data)[N]) {
    for (int i = 0; i < N; i++)
      if (data[i] != 0xCC && func[i] != data[i])
        goto fail;
    return (uint32_t*) (func + N);

  fail:
#ifdef DEBUG_PATCH_DUPLOCALE
    for (int i = 0; i < N; i++) {
      if (data[i] != 0xCC && func[i] != data[i])
        std::fprintf(stderr, " %02x", func[i]);
      else
        std::fprintf(stderr, " \x1b[1m%02x\x1b[m", func[i]);

      if ((i + 1) % 16 == 0 || i + 1 == N)
        std::fputc('\n', stderr);
    }
#endif
    return 0;
  }

  bool patch_duplocale() {
    HMODULE cygwinDll = ::GetModuleHandle("cygwin1.dll");
    if (cygwinDll == NULL) return false; // fail: not cygwin?

    byte* const duplocale_thunk = (byte*) ::GetProcAddress(cygwinDll, "duplocale");
    if (!duplocale_thunk) return false;

    // read IAT Entry
#ifdef DEBUG_PATCH_DUPLOCALE
    std::fprintf(stderr, "duplocale_thunk = %p\n", duplocale_thunk);
#endif
    byte* duplocale;
    {
      scoped_virtual_protect(duplocale_thunk, 5, PAGE_EXECUTE_READ);
      if (duplocale_thunk[0] == 0x68)
        duplocale = (byte*) *(uint32_t*) (duplocale_thunk + 1);
      else
        return false;
    }

    // read duplocale body
#ifdef DEBUG_PATCH_DUPLOCALE
    std::fprintf(stderr, "duplocale = %p\n", duplocale);
#endif
    byte* duplocale_r;
    {
      static byte duplocale_data[] = {
        0x83, 0xEC, 0x1C,
        0x64, 0xA1, 0x04, 0x00, 0x00, 0x00,
        0x8B, 0x54, 0x24, 0x20,
        0x2D, 0xE4, 0x2A, 0x00, 0x00,
        0x89, 0x04, 0x24,
        0x89, 0x54, 0x24, 0x04,
        0xE8,
      };

      scoped_virtual_protect _(duplocale, sizeof(duplocale_data) + 4, PAGE_EXECUTE_READ);

      uint32_t* poffset;
      if (uint32_t* poffset = skip_checked_prefix(duplocale, duplocale_data)) {
        duplocale_r = (byte*) (uintptr_t) ((uint32_t) poffset + 4 + *poffset);
      } else
        return false;
    }

    // read duplocale_r body
#ifdef DEBUG_PATCH_DUPLOCALE
    std::fprintf(stderr, "duplocale_r = %p\n", duplocale_r);
#endif
    {
      static byte duplocale_r_data[] = {
        0x55,
        0x57,
        0x56,
        0x53,
        0x81, 0xec, 0x7c, 0x01, 0x00, 0x00,
        0x8b, 0x9c, 0x24, 0x94, 0x01, 0x00, 0x00,
        0x83, 0xfb, 0xff,
        0x0f, 0x84, 0xe6, 0x00, 0x00, 0x00,
        0x81, 0xfb, 0x60, 0xCC, 0xCC, 0xCC,
        0x0f, 0x84, 0xea, 0x00, 0x00, 0x00,
        0xb9, 0x58, 0x00, 0x00, 0x00,
        0x8d, 0x7c, 0x24, 0x10,
        0x89, 0xde,
        0x8d, 0x6c, 0x24, 0x30,
        0xf3, 0xa5,
        0x8d, 0xbc, 0x24, 0x40, 0x01, 0x00, 0x00,
        0xbe, 0x01, 0x00, 0x00, 0x00,
        0x8b, 0x94, 0xf3, 0x2c, 0x01, 0x00, 0x00,
        0x85, 0xd2,
        0x74, 0x25,
        0x8d, 0x44, 0x24, 0x10,
        0xc7, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xc7, 0x47, 0x04, 0x00, 0x00, 0x00, 0x00,
        0x89, 0x6c, 0x24, 0x08,
        0x89, 0x74, 0x24, 0x04,
        0x89, 0x04, 0x24,
        0xe8,
      };

      scoped_virtual_protect _(duplocale_r, sizeof(duplocale_r_data) + 4, PAGE_EXECUTE_READWRITE);

      if (uint32_t* poffset = skip_checked_prefix(duplocale_r, duplocale_r_data)) {
        original_loadlocale = (loadlocale_t*) (uintptr_t) ((uint32_t) poffset + 4 + *poffset);
        *poffset = (uint32_t) (uintptr_t) &my_loadlocale - ((uint32_t) poffset + 4);
#ifdef DEBUG_PATCH_DUPLOCALE
        std::fprintf(stderr, "loadlocale replaced at %p\n", poffset);
#endif
      } else
        return false;
    }
  }

}

#ifdef USE_AS_DLL
extern "C" BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    {
      dll_patcher patcher("cygstdc++-6.dll");
      patch_libstdcxx_locale_impl1::patch(patcher, hinstDLL);
      patch_libstdcxx_locale_impl2::patch(patcher, hinstDLL);
      patch_libstdcxx_locale_impl3::patch(patcher, hinstDLL);
      patch_duplocale();
    }
    break;
  }
  return TRUE;
}

int use_libstdcxx_locale_patch() {return 0;}

#else
int patch_libstdcxx_locale() {
  dll_patcher patcher("cygstdc++-6.dll");
  patch_libstdcxx_locale_impl1::patch(patcher);
  patch_libstdcxx_locale_impl2::patch(patcher);
  patch_libstdcxx_locale_impl3::patch(patcher);
  patch_duplocale();
  return 0;
}

#endif
