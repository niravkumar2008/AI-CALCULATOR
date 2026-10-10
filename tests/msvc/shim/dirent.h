// Minimal dirent shim for MSVC (test build only).
#pragma once
#include <io.h>
#include <string>
struct dirent { char d_name[260]; };
struct DIR { intptr_t h; _finddata_t fd; bool first; dirent e; };
inline DIR* opendir(const char* p) {
  DIR* d = new DIR; std::string pat = std::string(p) + "/*";
  d->h = _findfirst(pat.c_str(), &d->fd); d->first = true;
  if (d->h == -1) { delete d; return nullptr; } return d;
}
inline dirent* readdir(DIR* d) {
  if (!d->first && _findnext(d->h, &d->fd) != 0) return nullptr;
  d->first = false; strncpy_s(d->e.d_name, d->fd.name, 259); return &d->e;
}
inline int closedir(DIR* d) { _findclose(d->h); delete d; return 0; }
