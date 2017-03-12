#include <stdlib.h>
#include <locale.h>

volatile int var;

int main(void) {
  locale_t const loc = newlocale(LC_ALL_MASK, "", NULL);
  locale_t const dup = duplocale(loc);
  locale_t const old = uselocale(dup);
  var = MB_CUR_MAX; /* crashes here */
  uselocale(old);
  freelocale(dup);
  freelocale(loc);
  return 0;
}
