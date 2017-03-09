#include <iostream>
#include <cxxabi.h>

int main() {
  int status;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5facet18_S_create_c_localeERPiPKcS1_", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6locale5_ImplC2EPKcj", 0, 0, &status) << std::endl;
  std::cout << abi::__cxa_demangle("_ZNSt6localeC2EPKc", 0, 0, &status) << std::endl;
  return 0;
}
