
#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <map>

using std::string;
using uint = unsigned int;

namespace VTS {
// Virtual Terminal Sequence

// source:
// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
// http://www.braun-home.net/michael/info/misc/VT100_commands.htm
//

//
// CURSOR
//

const string CURSOR_UP    = "\033[1A";
const string CURSOR_DOWN  = "\033[1B";
const string CURSOR_RIGHT = "\033[1C";
const string CURSOR_LEFT  = "\033[1D";
const string CURSOR_LEFTMOST  = "\033[0G";

inline string cursor_up(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "A");
}

inline string cursor_down(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "B");
}

inline string cursor_nlines_up(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "E");
}

inline string cursor_nlines_down(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "F");
}

inline string cursor_right(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "C");
}

inline string cursor_left(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "D");
}

inline string cursor_move_at_x(int pos){
  return "\033[" + std::to_string(pos + 1) + "G";
}

inline string cursor_move_at_y(int pos){
  return "\033[" + std::to_string(pos + 1) + "d";
}

inline string cursor_move_at_xy(int x, int y){
  return "\033[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) + "H";
}

const string CURSOR_SAVE    = "\033[s";
const string CURSOR_RESTORE = "\033[u";

const string CURSOR_HIDE = "\033[?25l";
const string CURSOR_REVEAL = "\033[?25h";

//
// viewport 
//

inline string viewport_scroll_down(const uint pos){
  return "\033[" + std::to_string(pos) + "T" + cursor_down(pos);
}

inline string viewport_scroll_up(const uint pos){
  return "\033[" + std::to_string(pos) + "S" + cursor_up(pos);
}

const string CLEAR_SCREEN = "\033[2J\033[3J";

//
// text changes 
//

const string CLEAR_LINE = "\033[0G\033[0K";
const string CLEAR_RIGHT = "\033[0K";

inline string delete_lines(const uint x){
  return x == 0 ? "" : ("\033[" + std::to_string(x) + "M");
}

//
// formatting 
//

const string FMT_RESET = "\033[0m";

const string BOLD     = "\033[1m";
const string BOLD_NOT = "\033[22m";

const string UNDERLINE     = "\033[4m";
const string UNDERLINE_NOT = "\033[24m";

//
// colors 
//

// foreground
const string FG_DEFAULT = "\033[39m";

const string FG_BLACK   = "\033[30m";
const string FG_RED     = "\033[31m";
const string FG_GREEN   = "\033[32m";
const string FG_YELLOW  = "\033[33m";
const string FG_BLUE    = "\033[34m";
const string FG_MAGENTA = "\033[35m";
const string FG_CYAN    = "\033[36m";
const string FG_WHITE   = "\033[37m";

const string FG_BRIGHT_BLACK   = "\033[90m";
const string FG_BRIGHT_RED     = "\033[91m";
const string FG_BRIGHT_GREEN   = "\033[92m";
const string FG_BRIGHT_YELLOW  = "\033[93m";
const string FG_BRIGHT_BLUE    = "\033[94m";
const string FG_BRIGHT_MAGENTA = "\033[95m";
const string FG_BRIGHT_CYAN    = "\033[96m";
const string FG_BRIGHT_WHITE   = "\033[97m";

const std::map<char, int> HEX_MAP = { 
  {'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5}, {'6', 6}, {'7', 7}, {'8', 8}, {'9', 9}, 
  {'a', 10}, {'b', 11}, {'c', 12}, {'d', 13}, {'e', 14}, {'f', 15} ,
  {'A', 10}, {'B', 11}, {'C', 12}, {'D', 13}, {'E', 14}, {'F', 15} 
};

inline string hex_to_decimal(const string &x){
  if(x.size() != 2){
    std::cerr << "INTERNAL ERROR: The function hex_to_decimal() only works with 2 digits inputs. Please update it.\n";
    return "0";
  }
  int dec = 16 * HEX_MAP.at(x.at(0)) + HEX_MAP.at(x.at(1));
  return std::to_string(dec);
}

inline string fg_rgb(const string &rgb){
  if(rgb.size() != 7){
    std::cerr << "Internal error: fg_rgb(\"" << rgb << "\"): invalid color, it must be of the form #rrggbb.\n";
    return FG_YELLOW;
  }
  
  string r = rgb.substr(1, 2);
  string g = rgb.substr(3, 2);
  string b = rgb.substr(5, 2);
  
  return ("\033[38;2;" + hex_to_decimal(r) + ";" + hex_to_decimal(g) + ";" + hex_to_decimal(b) + "m");
}

inline string fg_rgb(int r, int g, int b){
  return ("\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m");
}

inline string fg_grey(int v){
  string vstr = std::to_string(v);
  return ("\033[38;2;" + vstr + ";" + vstr + ";" + vstr + "m");
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit => color list
inline string fg_256(int v){
  return ("\033[38;5;" + std::to_string(v) + "m");
}

// background
const string BG_DEFAULT = "\033[49m";

const string BG_BLACK   = "\033[40m";
const string BG_RED     = "\033[41m";
const string BG_GREEN   = "\033[42m";
const string BG_YELLOW  = "\033[43m";
const string BG_BLUE    = "\033[44m";
const string BG_MAGENTA = "\033[45m";
const string BG_CYAN    = "\033[46m";
const string BG_WHITE   = "\033[47m";

const string BG_BRIGHT_BLACK   = "\033[100m";
const string BG_BRIGHT_RED     = "\033[101m";
const string BG_BRIGHT_GREEN   = "\033[102m";
const string BG_BRIGHT_YELLOW  = "\033[103m";
const string BG_BRIGHT_BLUE    = "\033[104m";
const string BG_BRIGHT_MAGENTA = "\033[105m";
const string BG_BRIGHT_CYAN    = "\033[106m";
const string BG_BRIGHT_WHITE   = "\033[107m";

inline string bg_rgb(int r, int g, int b){
  return ("\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m");
}

inline string bg_grey(int v){
  string vstr = std::to_string(v);
  return ("\033[48;2;" + vstr + ";" + vstr + ";" + vstr + "m");
}

inline string bg_256(int v){
  return ("\033[48;5;" + std::to_string(v) + "m");
}

const string RESET_FG_BG = FG_DEFAULT + BG_DEFAULT;

const std::map<std::string, std::string> all_html_colors = {
  {"alice_blue", "\033[38;2;240;248;255m"},
  {"antique_white", "\033[38;2;250;235;215m"},
  {"aqua", "\033[38;2;0;255;255m"},
  {"aquamarine", "\033[38;2;127;255;212m"},
  {"azure", "\033[38;2;240;255;255m"},
  {"beige", "\033[38;2;245;245;220m"},
  {"bisque", "\033[38;2;255;228;196m"},
  {"black", "\033[38;2;0;0;0m"},
  {"blanched_almond", "\033[38;2;255;235;205m"},
  {"blue", "\033[38;2;0;0;255m"},
  {"blue_violet", "\033[38;2;138;43;226m"},
  {"brown", "\033[38;2;165;42;42m"},
  {"burly_wood", "\033[38;2;222;184;135m"},
  {"cadet_blue", "\033[38;2;95;158;160m"},
  {"chartreuse", "\033[38;2;127;255;0m"},
  {"chocolate", "\033[38;2;210;105;30m"},
  {"coral", "\033[38;2;255;127;80m"},
  {"cornflower_blue", "\033[38;2;100;149;237m"},
  {"cornsilk", "\033[38;2;255;248;220m"},
  {"crimson", "\033[38;2;220;20;60m"},
  {"cyan", "\033[38;2;0;255;255m"},
  {"dark_blue", "\033[38;2;0;0;139m"},
  {"dark_cyan", "\033[38;2;0;139;139m"},
  {"dark_golden_rod", "\033[38;2;184;134;11m"},
  {"dark_gray", "\033[38;2;169;169;169m"},
  {"dark_grey", "\033[38;2;169;169;169m"},
  {"dark_green", "\033[38;2;0;100;0m"},
  {"dark_khaki", "\033[38;2;189;183;107m"},
  {"dark_magenta", "\033[38;2;139;0;139m"},
  {"dark_olive_green", "\033[38;2;85;107;47m"},
  {"dark_orange", "\033[38;2;255;140;0m"},
  {"dark_orchid", "\033[38;2;153;50;204m"},
  {"dark_red", "\033[38;2;139;0;0m"},
  {"dark_salmon", "\033[38;2;233;150;122m"},
  {"dark_sea_green", "\033[38;2;143;188;143m"},
  {"dark_slate_blue", "\033[38;2;72;61;139m"},
  {"dark_slate_gray", "\033[38;2;47;79;79m"},
  {"dark_slate_grey", "\033[38;2;47;79;79m"},
  {"dark_turquoise", "\033[38;2;0;206;209m"},
  {"dark_violet", "\033[38;2;148;0;211m"},
  {"deep_pink", "\033[38;2;255;20;147m"},
  {"deep_sky_blue", "\033[38;2;0;191;255m"},
  {"dim_gray", "\033[38;2;105;105;105m"},
  {"dim_grey", "\033[38;2;105;105;105m"},
  {"dodger_blue", "\033[38;2;30;144;255m"},
  {"fire_brick", "\033[38;2;178;34;34m"},
  {"floral_white", "\033[38;2;255;250;240m"},
  {"forest_green", "\033[38;2;34;139;34m"},
  {"fuchsia", "\033[38;2;255;0;255m"},
  {"gainsboro", "\033[38;2;220;220;220m"},
  {"ghost_white", "\033[38;2;248;248;255m"},
  {"gold", "\033[38;2;255;215;0m"},
  {"golden_rod", "\033[38;2;218;165;32m"},
  {"gray", "\033[38;2;128;128;128m"},
  {"grey", "\033[38;2;128;128;128m"},
  {"green", "\033[38;2;0;128;0m"},
  {"green_yellow", "\033[38;2;173;255;47m"},
  {"honey_dew", "\033[38;2;240;255;240m"},
  {"hot_pink", "\033[38;2;255;105;180m"},
  {"indian_red", "\033[38;2;205;92;92m"},
  {"indigo", "\033[38;2;75;0;130m"},
  {"ivory", "\033[38;2;255;255;240m"},
  {"khaki", "\033[38;2;240;230;140m"},
  {"lavender", "\033[38;2;230;230;250m"},
  {"lavender_blush", "\033[38;2;255;240;245m"},
  {"lawn_green", "\033[38;2;124;252;0m"},
  {"lemon_chiffon", "\033[38;2;255;250;205m"},
  {"light_blue", "\033[38;2;173;216;230m"},
  {"light_coral", "\033[38;2;240;128;128m"},
  {"light_cyan", "\033[38;2;224;255;255m"},
  {"light_golden_rod_yellow", "\033[38;2;250;250;210m"},
  {"light_gray", "\033[38;2;211;211;211m"},
  {"light_grey", "\033[38;2;211;211;211m"},
  {"light_green", "\033[38;2;144;238;144m"},
  {"light_pink", "\033[38;2;255;182;193m"},
  {"light_salmon", "\033[38;2;255;160;122m"},
  {"light_sea_green", "\033[38;2;32;178;170m"},
  {"light_sky_blue", "\033[38;2;135;206;250m"},
  {"light_slate_gray", "\033[38;2;119;136;153m"},
  {"light_slate_grey", "\033[38;2;119;136;153m"},
  {"light_steel_blue", "\033[38;2;176;196;222m"},
  {"light_yellow", "\033[38;2;255;255;224m"},
  {"lime", "\033[38;2;0;255;0m"},
  {"lime_green", "\033[38;2;50;205;50m"},
  {"linen", "\033[38;2;250;240;230m"},
  {"magenta", "\033[38;2;255;0;255m"},
  {"maroon", "\033[38;2;128;0;0m"},
  {"medium_aqua_marine", "\033[38;2;102;205;170m"},
  {"medium_blue", "\033[38;2;0;0;205m"},
  {"medium_orchid", "\033[38;2;186;85;211m"},
  {"medium_purple", "\033[38;2;147;112;219m"},
  {"medium_sea_green", "\033[38;2;60;179;113m"},
  {"medium_slate_blue", "\033[38;2;123;104;238m"},
  {"medium_spring_green", "\033[38;2;0;250;154m"},
  {"medium_turquoise", "\033[38;2;72;209;204m"},
  {"medium_violet_red", "\033[38;2;199;21;133m"},
  {"midnight_blue", "\033[38;2;25;25;112m"},
  {"mint_cream", "\033[38;2;245;255;250m"},
  {"misty_rose", "\033[38;2;255;228;225m"},
  {"moccasin", "\033[38;2;255;228;181m"},
  {"navajo_white", "\033[38;2;255;222;173m"},
  {"navy", "\033[38;2;0;0;128m"},
  {"old_lace", "\033[38;2;253;245;230m"},
  {"olive", "\033[38;2;128;128;0m"},
  {"olive_drab", "\033[38;2;107;142;35m"},
  {"orange", "\033[38;2;255;165;0m"},
  {"orange_red", "\033[38;2;255;69;0m"},
  {"orchid", "\033[38;2;218;112;214m"},
  {"pale_golden_rod", "\033[38;2;238;232;170m"},
  {"pale_green", "\033[38;2;152;251;152m"},
  {"pale_turquoise", "\033[38;2;175;238;238m"},
  {"pale_violet_red", "\033[38;2;219;112;147m"},
  {"papaya_whip", "\033[38;2;255;239;213m"},
  {"peach_puff", "\033[38;2;255;218;185m"},
  {"peru", "\033[38;2;205;133;63m"},
  {"pink", "\033[38;2;255;192;203m"},
  {"plum", "\033[38;2;221;160;221m"},
  {"powder_blue", "\033[38;2;176;224;230m"},
  {"purple", "\033[38;2;128;0;128m"},
  {"rebecca_purple", "\033[38;2;102;51;153m"},
  {"red", "\033[38;2;255;0;0m"},
  {"rosy_brown", "\033[38;2;188;143;143m"},
  {"royal_blue", "\033[38;2;65;105;225m"},
  {"saddle_brown", "\033[38;2;139;69;19m"},
  {"salmon", "\033[38;2;250;128;114m"},
  {"sandy_brown", "\033[38;2;244;164;96m"},
  {"sea_green", "\033[38;2;46;139;87m"},
  {"sea_shell", "\033[38;2;255;245;238m"},
  {"sienna", "\033[38;2;160;82;45m"},
  {"silver", "\033[38;2;192;192;192m"},
  {"sky_blue", "\033[38;2;135;206;235m"},
  {"slate_blue", "\033[38;2;106;90;205m"},
  {"slate_gray", "\033[38;2;112;128;144m"},
  {"slate_grey", "\033[38;2;112;128;144m"},
  {"snow", "\033[38;2;255;250;250m"},
  {"spring_green", "\033[38;2;0;255;127m"},
  {"steel_blue", "\033[38;2;70;130;180m"},
  {"tan", "\033[38;2;210;180;140m"},
  {"teal", "\033[38;2;0;128;128m"},
  {"thistle", "\033[38;2;216;191;216m"},
  {"tomato", "\033[38;2;255;99;71m"},
  {"turquoise", "\033[38;2;64;224;208m"},
  {"violet", "\033[38;2;238;130;238m"},
  {"wheat", "\033[38;2;245;222;179m"},
  {"white", "\033[38;2;255;255;255m"},
  {"white_smoke", "\033[38;2;245;245;245m"},
  {"yellow", "\033[38;2;255;255;0m"},
  {"yellow_green", "\033[38;2;154;205;50m"},
  {"term_default", "\033[39m"},
  {"term_black", "\033[30m"},
  {"term_red", "\033[31m"},
  {"term_green", "\033[32m"},
  {"term_yellow", "\033[33m"},
  {"term_blue", "\033[34m"},
  {"term_magenta", "\033[35m"},
  {"term_cyan", "\033[36m"},
  {"term_white", "\033[37m"},
  {"term_bright_black", "\033[90m"},
  {"term_bright_red", "\033[91m"},
  {"term_bright_green", "\033[92m"},
  {"term_bright_yellow", "\033[93m"},
  {"term_bright_blue", "\033[94m"},
  {"term_bright_magenta", "\033[95m"},
  {"term_bright_cyan", "\033[96m"},
  {"term_bright_white", "\033[97m"},
};


  
} // namespace VTS
