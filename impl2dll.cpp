#include <windows.h>
#include <cstdio>

void patch_libstdcxx_locale();

extern "C" BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    patch_libstdcxx_locale();
    break;
  }
  return TRUE;
}

void impl2dll() {}
