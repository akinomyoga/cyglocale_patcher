#include <iostream>
#include <cxxabi.h>

int main() {
  int status;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5_ImplC2EPKcj", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6localeC2EPKc", 0, 0, &status) << std::endl;

  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet19_S_destroy_c_localeERPi"     , 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet17_S_clone_c_localeERPi"       , 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet20_S_lc_ctype_c_localeEPiPKc"  , 0, 0, &status) << std::endl;

  std::cout << abi::__cxa_demangle("_ZNSt11__timepunctIcE23_M_initialize_timepunctEPi", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt5ctypeIcEC1EPiPKcbj", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt5ctypeIcEC2EPiPKcbj", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt5ctypeIcEC1EPKcbj"  , 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt5ctypeIcEC2EPKcbj"  , 0, 0, &status) << std::endl;
  return 0;
}
