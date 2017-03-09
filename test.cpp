#include <stdlib.h>
#include <cstdio>
#include <locale>

void test1() {
  std::locale("");
}
void test2() {
  ::setenv("LC_COLLATE",  "ja_JP.UTF-8", 1);
  ::setenv("LC_CTYPE",    "ja_JP.UTF-8", 1);
  ::setenv("LC_MESSAGES", "ja_JP.UTF-8", 1);
  ::setenv("LC_MONETARY", "ja_JP.UTF-8", 1);
  ::setenv("LC_NUMERIC",  "ja_JP.UTF-8", 1);
  ::setenv("LC_TIME",     "ja_JP.UTF-8", 1);
  std::locale("");
}
void test2b() {
  ::setenv("LC_COLLATE",  "", 1);
  ::setenv("LC_CTYPE",    "", 1);
  ::setenv("LC_MESSAGES", "", 1);
  ::setenv("LC_MONETARY", "", 1);
  ::setenv("LC_NUMERIC",  "", 1);
  ::setenv("LC_TIME",     "", 1);
  std::locale("");
}
void test3a() { std::locale("ja_JP.UTF-8"); }
void test3b() { std::locale(".UTF-8"); }
void test3c() { std::locale("Japanese_Japan.65001"); }
void test3d() { std::locale("ja-JP"); }
void test3e() { std::locale(".65001"); }

namespace std {
  //__attribute__((always_inline)) inline
  void
  locale::facet::_S_create_c_locale(__c_locale& _cloc, const char* _s, __c_locale _old) {
    _cloc = (__c_locale) ::newlocale(1 << LC_ALL, _s, (::locale_t) _old);
    if (!_cloc) {
      // This named locale is not supported by the underlying OS.
      std::runtime_error("locale::facet::_S_create_c_locale name not valid");
    }
  }

  //__attribute__((always_inline)) inline
  void
  locale::facet::_S_destroy_c_locale(__c_locale& _cloc) {
    if (_cloc && _S_get_c_locale() != _cloc)
      ::freelocale((::locale_t) _cloc);
  }

  //__attribute__((always_inline)) inline
  __c_locale
  locale::facet::_S_clone_c_locale(__c_locale& _cloc) throw() {
    return (__c_locale) ::duplocale((::locale_t) _cloc);
  }

  //__attribute__((always_inline)) inline
  __c_locale
  locale::facet::_S_lc_ctype_c_locale(__c_locale _cloc, const char* _s) {
    ::locale_t _dup = ::duplocale((::locale_t) _cloc);
    if (_dup == ::locale_t(0))
      new std::runtime_error("locale::facet::_S_lc_ctype_c_locale duplocale error");
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ > 2)
    ::locale_t _changed = ::newlocale(LC_CTYPE_MASK, _s, _dup);
#else
    ::locale_t _changed = ::newlocale(1 << LC_CTYPE, _s, _dup);
#endif
    if (_changed == ::locale_t(0)) {
      ::freelocale(_dup);
      std::runtime_error("locale::facet::_S_lc_ctype_c_locale newlocale error");
    }
    return (__c_locale) _changed;
  }
}

#include <cstdlib>
#include <stdint.h>
#include <windows.h>

static void atomic_write64(uint64_t volatile* const target, uint64_t value) {
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
  // ノーガードで書く。仕方がない。
  //
  *target = value;
#endif

}

struct libstdcxx_locale_hot_patcher: std::locale::facet {

  static bool patch_function(void* ptarget, void* preplace) {
    bool result = false;

    // 16 bytes で align されているっぽいので仮定してしまう。
    if (ptarget && (uintptr_t) ptarget % 8 == 0) {
      DWORD old_protect;
      ::VirtualProtect((LPVOID) ptarget, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect);
      int32_t distance32 = (int32_t) ((intptr_t) preplace - (intptr_t) ptarget - 5);
      if ((intptr_t) preplace == ((intptr_t) ptarget + 5) + distance32) {
        uint64_t* ptarget64 = (uint64_t*) ptarget;
        union { uint64_t intval; unsigned char data[8]; } body = { *ptarget64 };
        body.data[0] = 0xE9;
        memcpy(body.data + 1, &distance32, sizeof(distance32));
        atomic_write64(ptarget64, body.intval);
        result = true;
      }
      ::VirtualProtect((LPVOID) ptarget, sizeof(void*), old_protect, &old_protect);
    }
    return result;
  }

  static void error_exit() { std::exit(EXIT_FAILURE); }

  static void patch() {
    // そのままだと protected でアクセスできないので派生クラスを作って中から触る。
    ::HMODULE const cygstdcxx_dll = ::GetModuleHandle("cygstdc++-6.dll");
    ::FARPROC const create   = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_");
    ::FARPROC const destroy  = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet19_S_destroy_c_localeERPi");
    ::FARPROC const lone     = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet17_S_clone_c_localeERPi");
    ::FARPROC const lc_ctype = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc");
    if (!patch_function((void*) create  , (void*) &std::locale::facet::_S_create_c_locale  )) error_exit();
    if (!patch_function((void*) destroy , (void*) &std::locale::facet::_S_destroy_c_locale )) error_exit();
    if (!patch_function((void*) lone    , (void*) &std::locale::facet::_S_clone_c_locale   )) error_exit();
    if (!patch_function((void*) lc_ctype, (void*) &std::locale::facet::_S_lc_ctype_c_locale)) error_exit();

  }

  static void check() {
    ::HMODULE const cygstdcxx_dll = ::GetModuleHandle("cygstdc++-6.dll");
    ::FARPROC const create   = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_");
    ::FARPROC const destroy  = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet19_S_destroy_c_localeERPi");
    ::FARPROC const lone     = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet17_S_clone_c_localeERPi");
    ::FARPROC const lc_ctype = ::GetProcAddress(cygstdcxx_dll, "_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc");
    std::printf("original = %p\n", (void*) create  );
    std::printf("original = %p\n", (void*) destroy );
    std::printf("original = %p\n", (void*) lone    );
    std::printf("original = %p\n", (void*) lc_ctype);
    std::printf("replaced = %p\n", &std::locale::facet::_S_create_c_locale);
    std::printf("replaced = %p\n", &std::locale::facet::_S_destroy_c_locale);
    std::printf("replaced = %p\n", &std::locale::facet::_S_clone_c_locale);
    std::printf("replaced = %p\n", &std::locale::facet::_S_lc_ctype_c_locale);
    std::printf("WINVER=%04x\n", (int) WINVER);
  }
};

void test4check() {
  libstdcxx_locale_hot_patcher::check();
}

void test4() {
  libstdcxx_locale_hot_patcher::patch();
  std::locale("");
}

int main() {
  // test1();
  // test2();
  // test2b();
  // test3a();
  // test3b();
  // test3c();
  // test3d();
  // test3e();
  // test4check();
  test4();

  return 0;
}
