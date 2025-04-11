

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






