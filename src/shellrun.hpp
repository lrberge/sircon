    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#pragma once

#include "util.hpp"
#include "constants.hpp"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

using std::string;
using std::vector;

string run_shell_command(const string &cmd, const string &args);






