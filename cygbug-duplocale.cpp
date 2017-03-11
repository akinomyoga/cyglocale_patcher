#include <stdlib.h>
#include <locale.h>

// Cygwin で duplocale で作ったロケールを uselocale に渡して
// MB_CUR_MAX を参照するとクラッシュする。
//
// Note: MB_CUR_MAX は __locale_mb_cur_max() に展開される。
// Note: newlocale で作成したロケールを直接使った場合は問題ない。
// Note: duplocale の代わりに newlocale(0, "C", loc) を使った場合も問題ない。

int main() {
  locale_t const loc = newlocale(1 << LC_ALL, "", NULL);
  locale_t const dup = duplocale(loc);
  locale_t const old = uselocale(dup);
  int const ret = MB_CUR_MAX; // ここで突然死
  uselocale(old);
  return ret;
}
