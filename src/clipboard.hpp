
#pragma once

#include "stringtools.hpp"
#include <windows.h>
#include <iostream>
#include <string>

#ifdef TRUE
  #undef TRUE
#endif
#ifdef FALSE
  #undef FALSE
#endif

using std::string;
using std::wstring;


namespace simpleclipboard {

string get_clipboard();
void set_clipboard(string str);

}
