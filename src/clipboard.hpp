    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

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
