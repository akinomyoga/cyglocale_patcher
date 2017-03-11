#include <locale>

int main() {
  std::locale("");
  return 0;
}

/*?lwiki
 * なんと cygwin では `std::locale("")` しただけでエラーになる。
 * (一方で VC なら大丈夫だった。)
 *
 * &pre(!cpp,title=test.cpp){
 * #include <locale>
 *
 * int main() {
 *   std::locale("");
 *   return 0;
 * }
 * }
 *
 * &pre(!sh){
 * $ g++ -std=c++14 -o test.exe test.cpp
 * $ ./test.exe
 * terminate called after throwing an instance of 'std::runtime_error'
 *  what():  locale::facet::_S_create_c_locale name not valid
 * }
*
* `""` は valid な引数 (C++14 §22.3.1.2/6) なので、
* `std::runtime_error` は起こらない (C++14 §22.3.1.2/5) はずである。
* これでは locale-specific native environment (C11 7.11.1.1/3) の
* `std::locale` オブジェクトが作れない…。
*/
