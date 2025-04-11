    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include <filesystem>
#include <vector>
#include <string>

using std::vector;
using std::string;
namespace fs = std::filesystem;
using uint = unsigned int;


namespace CSTATE {
  const int ALT      = 2;
  const int CTRL     = 8;
  const int SHIFT    = 16;
  const int ENHANCED = 256;
};

namespace UNICODE_KEY {
  const char CTRL_A = 1;
  const char CTRL_B = 2;
  const char CTRL_C = 3;
  const char CTRL_D = 4;
  const char CTRL_E = 5;
  const char CTRL_F = 6;
  const char CTRL_G = 7;
  const char CTRL_H = 8;
  const char CTRL_I = 9;
  const char CTRL_J = 10;
  const char CTRL_K = 11;
  const char CTRL_L = 12;
  const char CTRL_M = 13;
  const char CTRL_N = 14;
  const char CTRL_O = 15;
  const char CTRL_P = 16;
  const char CTRL_Q = 17;
  const char CTRL_R = 18;
  const char CTRL_S = 19;
  const char CTRL_T = 20;
  const char CTRL_U = 21;
  const char CTRL_V = 22;
  const char CTRL_W = 23;
  const char CTRL_X = 24;
  const char CTRL_Y = 25;
  const char CTRL_Z = 26;
};

namespace KEYS {
  const char BS    = 8;
  const char TAB   = 9;
  const char ENTER = 13;
  const char ESC   = 27;
  const char END   = 35;
  const char HOME  = 36;
  const char LEFT  = 37;
  const char UP    = 38;
  const char RIGHT = 39;
  const char DOWN  = 40;
  const char DEL   = 46;
};

namespace SIDE {
  enum {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    
    LEFTMOST,
    RIGHTMOST,
    TOP,
    BOTTOM,
  };
}


const char NOT_A_QUOTE = '0';
const string NO_STRING_MATCH = "___NO_STRING_MATCH___";
const string NO_RETURN = "___NO_RETURN___";

