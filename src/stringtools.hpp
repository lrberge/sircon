    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "metastringvec.hpp"
#include "constants.hpp"

#include <cmath>
#include <string>
#include <iostream>
#include <vector>
// for utf8/utf16 conversions
#include <windows.h>
// NOTA: we never want TRUE/FALSE to be defined because it messes up with R
#ifdef TRUE
  #undef TRUE
#endif
#ifdef FALSE
  #undef FALSE
#endif

using uchar = unsigned char;
using uint = unsigned int;
using std::string;
using std::wstring;
using std::vector;

// Implementation notes:
// - the user should use this class as a regular std::string
// - x[i]: returns the "string" associated to the i's myltibyte unicode point
// - it's less efficient than std::string but at least handles MB characters
// 

namespace stringtools {

extern string valid_word_char;

//
// utf8 ------------------------------------------------------------------------ 
//


wstring utf8_to_utf16(char p_utf8, bool& is_error);
wstring utf8_to_utf16(string& str, bool& is_error);

string utf16_to_utf8(wchar_t p_utf16, bool& is_error);
string utf16_to_utf8(wstring& str, bool& is_error);

namespace utf8 {

inline bool is_starting_byte(const uchar c){
  // c <= 127: ascii character
  // c >= 192: first byte of a UTF-8 byte sequence
  // all continuation UTF-8 bytes are within 128-191
  return c <= 127 || c >= 192;
}

inline bool is_continuation(const uchar c){
  return !(is_starting_byte(c));
}

inline void move_i_to_start_of_next_utf8_char(const string &str, uint &i){
  // we always move i, unless we're after the end of the string
  // in which case we set i = n
  
  const uint &n = str.size();
  if(i + 1 < n){
    // note that we don't check if the first byte is a valid utf8 starting byte
    ++i;
    while(i < n && is_continuation(str[i])){
      ++i;
    }
  } else {
    i = n;
  }
}

inline void move_i_to_end_of_next_utf8_char(const string &str, uint &i){
  // we always move i, unless we're after the end of the string
  // in which case we don't move
  
  const uint &n = str.size();
  if(i + 1 < n){
    // note that we don't check if the first byte is a valid utf8 starting byte
    ++i;
    while(i + 1 < n && is_continuation(str[i + 1])){
      ++i;
    }
  }
  
}

inline uint count_wide_chars(string str){
  uint n = 0;
  for(const char &c : str){
    if(is_starting_byte(c)){
      ++n;
    }
  }
  
  return n;
}

} // namespace utf8

//
// inline ----------------------------------------------------------------------
//

inline bool is_escaped(const string line, uint index){
  // i is the position of the character being escaped
  // ie: hello \" folks
  //            ^ i would be here and we would return true
  
  int i = index - 1;
  bool res = false;
  while(i >= 0 && line[i--] == '\\'){
    res = !res;
  }
  
  return res;
}

inline bool is_digit(const uchar c){
  return c >= '0' && c <= '9';
}

inline bool is_ascii_letter(const uchar c){
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_enter(const char c){
  return c == '\r' || c == '\n';
}

inline bool is_control_char(const uchar c){
  
  if( (c >= 32 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126) ){
    for(auto &valid_control_char : valid_word_char){
      if(c == valid_control_char){
        return false;
      }
    }
    
    return true;
  }
  
  return false;
}

inline bool is_word_char(const uchar c){
  return !is_control_char(c);
}

inline bool is_starting_word_char(const uchar c){  
  return !is_digit(c) && !is_control_char(c);
}

inline bool is_largeq_than_4(int x){
  return x >= 4;
}

inline string clean_VTS_markup(string x){
  string res;
  uint n = x.size();
  uint i = 0;
  while(i < n){
    const char &c = x[i];
    if(c == 27){
      ++i;
      while(i < n && !is_ascii_letter(x[i])){
        ++i;
      }
      ++i;
    } else {
      res.push_back(c);
      ++i;
    }
  }
  
  return res;
}

inline bool is_quote(const uchar c){
  return c == '"' || c == '\'' || c == '`';
}

inline bool is_nonquote_control(const uchar c){
  return is_control_char(c) && !is_quote(c);
}

inline bool is_nonspace_control(const uchar c){
  return is_control_char(c) && c != ' ';
}

inline bool is_WS(const uchar c){
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool is_int_inside(const string &x){
  if(x.empty()){
    return false;
  }
  
  for(const auto &c : x){
    if(c < '0' || c > '9'){
      return false;
    }
  }
  
  return true;
}

inline bool no_nonspace_char(const string &x){
  for(const auto &c : x){
    if(c != '\t' && c != ' ' && c != '\r' && c != '\n'){
      return false;
    }
  }
  return true;
}

// only works for ascii
inline string to_lower(const string &x){
  string res;
  for(const auto &c : x){
    if(c >= 'A' && c <= 'Z'){
      res += (c + 32);
    } else {
      res += c;
    }
  }
  
  return res;
}

// only works for ascii
inline string to_upper(const string &x){
  string res;
  for(const auto &c : x){
    if(c >= 'a' && c <= 'z'){
      res += (c - 32);
    } else {
      res += c;
    }
  }
  
  return res;
}

inline bool is_quote_paren_open(const uchar c){
  return is_quote(c) || c == '(' || c == '[' || c == '{';
}

inline bool is_opening_paren(const uchar c){
  return c == '(' || c == '[' || c == '{';
}

inline bool is_closing_paren(const uchar c){
  return c == ')' || c == ']' || c == '}';
}

inline bool is_paren(const uchar c){
  return c == '(' || c == '[' || c == '{' || c == ')' || c == ']' || c == '}';
}

inline bool is_paren_adjacent(string str, uint cursor_pos){
  const uint &n = str.size();
  if(n == 0){
    return false;
  }
  
  if(cursor_pos >= 1 && cursor_pos - 1 < n && is_paren(str[cursor_pos - 1])){
    return true;
  } else if(cursor_pos < n && is_paren(str[cursor_pos])){
    return true;
  }
  
  return false;
}

inline uint64_t hash_string(const string &x){
  return std::hash<string>{}(x);
}

uint64_t hash_string_vector(const vector<string> &x);

inline string ascii_printable(const char c){
  switch(c){
    case '\n': return "\\n";
    case '\t': return "\\t";
    case '\r': return "\\r";
    case 1: return "ctrl-A";
    case 2: return "ctrl-B";
    case 3: return "ctrl-C";
    case 4: return "ctrl-D";
    case 5: return "ctrl-E";
    case 6: return "ctrl-F";
    case 7: return "ctrl-G";
    case 8: return "ctrl-H";
    case 11: return "ctrl-K";
    case 12: return "ctrl-L";
    case 14: return "ctrl-N";
    case 15: return "ctrl-O";
    case 16: return "ctrl-P";
    case 17: return "ctrl-Q";
    case 18: return "ctrl-R";
    case 19: return "ctrl-S";
    case 20: return "ctrl-T";
    case 21: return "ctrl-U";
    case 22: return "ctrl-V";
    case 23: return "ctrl-W";
    case 24: return "ctrl-X";
    case 25: return "ctrl-Y";
    case 26: return "ctrl-Z";
    case 27: return "Esc";
    case 28: return "ctrl-\\";
    case 29: return "ctrl-]";
    case 30: return "ctrl-^";
    case 31: return "ctrl-_";
    default: {
      string res;
      res += c;
      return res;
    }
  }
  return "ERROR";
}

inline string ascii_printable(const string &str){
  string res;
  for(const auto &c : str) res += ascii_printable(c);
  return res;
}

inline string debug_utf8(const string &x){
  string res;
  bool utf8_open = false;
  for(const auto &c : x){
    unsigned char uc = c;
    if(uc < 128){
      if(utf8_open){
        utf8_open = false;
        res += "}";
      }
      
      res += ascii_printable(uc);
    } else {
      if(!utf8_open){
        utf8_open = true;
        res += "{";
      } else {
        res += " + ";
      }
      
      res += std::to_string(static_cast<unsigned int>(uc));
    }
  }
  
  if(utf8_open){
    res += "}";
  }
  
  return res;
}

inline string pair_quote_paren(const uchar c){
  switch(c){
    case '"': return "\"";
    case '\'': return "'";
    case '`': return "`";
    case '(': return ")";
    case '[': return "]";
    case '{': return "}";
    case ')': return "(";
    case ']': return "[";
    case '}': return "{";
  }
  
  return "AUTOMATCH-ERROR";
}

inline char pair_quote_paren_char(const uchar c){
  switch(c){
    case '"':  return '"';
    case '\'': return '\'';
    case '`':  return '`';
    case '(':  return ')';
    case '[':  return ']';
    case '{':  return '}';
    case ')':  return '(';
    case ']':  return '[';
    case '}':  return '{';
  }
  
  return '0';
}

inline bool any_open_paren_before(const string &x, const char target, int i = -100){
  // we start at i inclusive -- paren means ([{ -- we assume the code is well formatted
  // target can be either an open or a closed paren
  
  const uint n = x.size();
  
  if(i == -100){
    i = n - 1;
  }
  
  int n_open = 0;
  for(; i >= 0 ; --i){
    if(is_closing_paren(x[i])){
      --n_open;
    } else if(is_opening_paren(x[i])){
      if(n_open >= 0){
        if(x[i] == target || x[i] == pair_quote_paren_char(target)){
          return true;
        } else {
          return false;
        }
      } else {
        ++n_open;
      }
    }
  }
  
  return false;
}


inline bool is_string_in(const string &x, const vector<string> &set){
  for(const auto &str : set){
    if(x == str){
      return true;
    }
  }
  
  return false;
}

inline bool is_char_in(const char &x, const string &set){
  for(const auto &c : set){
    if(x == c){
      return true;
    }
  }
  
  return false;
}

inline void move_i_to_non_WS_if_i_WS(string str, int &i, int side){
  // NOTE: we don't move i if it is not a WS
  // OUTPUT:
  // - if only WS past i: i => n
  // - if only WS beore i: i => -1
  
  if(side == SIDE::RIGHT){
    while(i < static_cast<int>(str.size()) && is_WS(str[i])){
      ++i;
    }
  } else {
    while(i >= 0 && is_WS(str[i])){
      --i;
    }
  }
}

string shorten(const string &x, const uint nmax, const string ellipsis = "\u2026");
vector<string> shorten(const vector<string> &all_x, const uint nmax, const string ellipsis = "\u2026");
void shorten_inplace(vector<string> &all_x, const uint nmax, const string ellipsis = "\u2026");

inline bool is_open_paren_left(string line, uint index, const uchar closing_paren){
  // NOTA: in this function index == n is valid
  // it finds if there is an open paren left
  // there is no check as to whether the variable closing_paren is valid
  
  int i = static_cast<int>(index) - 1;
  const uchar open_paren = pair_quote_paren(closing_paren)[0];
  int n_open = 0;
  
  while(i >= 0){
    if(line[i] == open_paren){
      ++n_open;
      if(n_open > 0){
        return true;
      }
    } else if(line[i] == closing_paren){
      --n_open;
    }
    
    --i;
  }
  
  return false;
}

inline bool is_letter_adjacent(string line, uint cursor){
  // wether at cursor - 1 OR cursor, there is a letter
  
  const uint &n = line.size();
  if(n == 0){
    return false;
  }
  
  if(cursor >= 1 && cursor - 1 < n && is_word_char(line[cursor - 1])){
    return true;
  }
  
  if(cursor < n && is_word_char(line[cursor])){
    return true;
  }
  
  return false;
}

inline bool is_same_quote_open_before(string line, uint index, const uchar target_quote){
  uint i = 0;
  uint &n = index;
  bool open_quote = false;
  while(i < n){
    if(line[i] == target_quote){
      open_quote = !open_quote;
    } else if(is_quote(line[i])){
      const uchar new_quote = line[i];
      ++i;
      while( i < n && !( line[i] == new_quote && !is_escaped(line, i) ) ){
        ++i;
      }
    }
    
    ++i;
  }
  
  return open_quote;
}

void display_charvalue(string str, bool newline = false);

inline uint word_jump(const string &line, const uint index, const int side){
  // side: 0 == left
  
  /* DESIRED BEHAVIOR, SIDE = RIGHT
  * 
  * ON MOVE:
  * 
  * A)
  * |[space]+[control or word]+[space]
  *                            |
  * we go right after the control or word, just before the space
  * 
  * B)
  * |[space]+[control][word]+[space]
  *                          |
  * when a word is preceded by a single control: we go after the word
  * 
  * C)
  * |[space]+[control]{2,}[word]+[space]
  *                       |
  * when a word is preceded by 2+ controls, we go to the end of the controls
  * */
  
  if(side == SIDE::RIGHT){
        
    const uint n = line.size();
    uint i = index;
    
    // we ignore the spaces
    while(i < n && line[i] == ' '){
      ++i;
    }
    
    if(i == n){
      return n;
    }
    
    if(is_control_char(line[i])){
      if(i + 1 < n && is_word_char(line[i + 1])){
        // => we go to the word
      
      } else {
        ++i;
        while(i < n && is_nonspace_control(line[i])){
          ++i;
        }
        
        return i;
      }
    }
    
    ++i;
    while(i < n && is_word_char(line[i])){
      ++i;
    }
    
    return i;
    
  } else {
    
    int i = index - 1;
    
    // we ignore the spaces
    while(i >= 0 && line[i] == ' '){
      --i;
    }
    
    if(i < 0){
      return 0;
    }
    
    if(is_control_char(line[i])){
      if(i - 1 >= 0 && is_word_char(line[i - 1])){
        // => we go to the word
      
      } else {
        --i;
        while(i >= 0 && is_nonspace_control(line[i])){
          --i;
        }
        
        return i + 1;
      }
    }
    
    --i;
    while(i >= 0 && is_word_char(line[i])){
      --i;
    }
    
    return i + 1;
    
  }
  
  return 0;
}

inline uint word_jump_default(const string &line, const uint index, const int side){
  // this is a bit ugly... but at least I don't have to refactor everything
  const string valid_bak = valid_word_char;
  valid_word_char = "_";
  uint res = word_jump(line, index, side);
  valid_word_char = valid_bak;
  return res;
}

inline uint word_delete(const string &line, const uint index, const int side,
                        const bool stop_at_slash = false){
  
  /* DESIRED BEHAVIOR, SIDE = RIGHT
  * 
  * ON DELETE:
  * 
  * A) 
  * |[space][control or word]+[space]
  *  ~~~~~~~~~~~~~~~~~~~~~~~~~
  * if there is a single space, we delete everything up to the end of the word/control
  * 
  * B) 
  * |[space]{2,}[control or word]+[space]
  *  ~~~~~~~~~~~
  * if there are 2+ spaces, we only delete the spaces
  * 
  * C)
  * |[space][control]+[word]+[space]
  *  ~~~~~~~~~~~~~~~~~
  * we delete up to the word
  * 
  * */
  
  if(side == SIDE::RIGHT){
        
    const uint n = line.size();
    uint i = index;
    
    // we ignore the spaces
    if(i == n){
      return n;
    }
    
    if(line[i] == ' '){
      if(i + 1 < n && line[i + 1] == ' '){
        ++i;
        while(i < n && line[i] == ' '){
          ++i;
        }
        
        return i;
      } else {
        ++i;
      }
    }
    
    if(is_control_char(line[i])){
      ++i;
      
      if(stop_at_slash && line[i - 1] == '/'){
        
        while(i < n && line[i] == '/'){
          ++i;
        }
        
      }
      
      while(i < n && is_nonspace_control(line[i]) && !(stop_at_slash && line[i] == '/')){
        ++i;
      }
      
      return i;
    } else {
      ++i;
      while(i < n && is_word_char(line[i])){
        ++i;
      }
      
      return i;
    }
    
  } else {
    
    int i = index - 1;
    
    // we ignore the spaces
    if(i < 0){
      return 0;
    }
    
    if(line[i] == ' '){
      if(i - 1 >= 0 && line[i - 1] == ' '){
        --i;
        while(i >= 0 && line[i] == ' '){
          --i;
        }
        
        return i + 1;
      } else {
        --i;
      }
    }
    
    if(is_control_char(line[i])){
      
      --i;
      
      if(stop_at_slash && line[i + 1] == '/'){
        
        while(i >= 0 && line[i] == '/'){
          --i;
        }
        
      }
      
      while(i >= 0 && is_nonspace_control(line[i]) && !(stop_at_slash && line[i] == '/')){
        --i;
      }
      
      return i + 1;
    } else {
      --i;
      while(i >= 0 && is_word_char(line[i])){
        --i;
      }
      
      return i + 1;
    }
    
  }
  
  return 0;
}

inline uint word_delete_default(const string &line, const uint index, const int side){
  // this is a bit ugly... but at least I don't have to refactor everything
  const string valid_bak = valid_word_char;
  valid_word_char = "_";
  uint res = word_delete(line, index, side);
  valid_word_char = valid_bak;
  return res;
}

inline vector<string> setdiff(const vector<string> &x, const vector<string> &y){
  // this is a slow and naive algorithm => if I need to apply this to large
  // vectors, I should improve this. Not the case atm.
  // 
  
  vector<string> res;
  for(const auto &xi : x){
    bool found = false;
    for(const auto &yi : y){
      if(xi == yi){
        found = true;
        break;
      }
    }
    
    if(!found){
      res.push_back(xi);
    }
  }
  
  return res;
}

inline void print_string_vector(vector<string> x){
  const uint n = x.size();
  std::cout << "{";
  if(n > 0){
    std::cout << x[0];
  }
  for(uint i=1 ; i<n ; ++i){
    std::cout << ", " << x[i];
  }
  std::cout << "}\n";
}

inline size_t size_no_vts(const string &x){
  size_t clean_size = 0;
  const size_t n = x.size();
  uint i = 0;
  while(i < n){
    
    if(x[i] == 27){
      ++i;
      while(i < n && !is_ascii_letter(x[i])){
        ++i;
      }
      ++i;
    } else {
      ++clean_size;
      utf8::move_i_to_start_of_next_utf8_char(x, i);
    }
    
  }
  
  return clean_size;
}

inline uint max_size(const vector<string> &all_x){
  uint res = 0;
  for(const auto &x : all_x){
    if(res < size_no_vts(x)){
      res = size_no_vts(x);
    }
  }
  return res;
}

inline string dquote(const string &x){
  return "\"" + x + "\"";
}

inline string bquote(const string &x){
  return "`" + x + "`";
}

inline bool str_contains(const string &x, const uchar c){
  return x.find(c) != string::npos;
}

inline bool str_contains(const string &x, const string &target){
  
  if(target.empty()){
    return true;
  }
  
  const size_t n = x.size();
  size_t i = 0;
  
  while(i < n){
    if(x[i] == target[0]){
      
      if(i + target.size() - 1 >= n){
        return false;
      }
      
      bool ok = true;
      for(size_t j = 1 ; j < target.size() ; ++j){
        if(x[i + j] != target[j]){
          ok = false;
          break;
        }
      }
      
      if(ok){
        return true;
      }
    }
    
    ++i;
  }
  
  return false;
}


inline bool starts_with(const string &x, const string &target){
  
  string val{target};
  
  if(x.size() < val.size()){
    return false;
  }
  
  if(val.empty()){
    return false;
  }
  
  for(size_t i=0 ; i<val.size() ; ++i){
    if(x[i] != val[i]){
      return false;
    }
  }
  
  return true;
}


inline bool starts_with(const string &x, std::initializer_list<string> all_targets){
  
  const vector<string> &all_values = all_targets;
  
  if(x.empty()){
    return false;
  }
  
  if(all_values.empty()){
    return false;
  }
  
  const size_t n = x.size();
  bool found = false;
  for(const auto &val : all_values){
    if(n < val.size()){
      continue;
    }
    
    found = true;
    for(size_t i = 0 ; i < val.size() ; ++i){
      if(x[i] != val[i]){
        found = false;
        break;
      }
    }
    
    if(found){
      return true;
    }
  }
  
  return found;
}


inline bool ends_with(const string &x, const string &val, bool ingore_WS = false){
  
  if(x.size() < val.size()){
    return false;
  }
  
  if(val.empty()){
    return false;
  }
  
  int n_x = x.size();
  const int n_val = val.size();
  
  if(ingore_WS){
    while(n_x >= 1 && x[n_x - 1] == ' '){
      --n_x;
    }
    
    if(n_x == 0 || n_x < n_val){
      return false;
    }
  }
  
  for(int i = 0 ; i < n_val ; ++i){
    if(x[n_x - 1 - i] != val[n_val - 1 - i]){
      return false;
    }
  }
  
  return true;
}


inline bool ends_with_ignore_WS(const string &x, const string &val){
  return ends_with(x, val, true);
}

inline bool ends_with(const string &x, const char c, bool ignore_WS = false){
  if(x.empty()){
    return false;
  }
  
  int i = x.size() - 1;
  
  if(ignore_WS){
    move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
  }
  
  if(i >= 0 && x[i] == c){
    return true;
  }
  
  return false;
}

inline bool ends_with_ignore_WS(const string &x, const char c){
  return ends_with(x, c, true);
}


inline vector<string>& append_right(vector<string> &x, string val){
  
  for(auto &s : x){
    s += val;
  }
  
  return x;
}

inline vector<string>& append_left(vector<string> &x, string val){
  
  for(auto &s : x){
    s.insert(s.begin(), val.begin(), val.end());
  }
  
  return x;
}

inline string extract_word(const string &x, int &i){
  
  move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  const int n = x.size();
  
  string res;
  while(i < n && is_word_char(x[i])){
    res += x[i++];
  }
  
  return res;
}

inline string extract_until_next_unescaped_char(const string &x, int &i, const char c){
  const int n = x.size();
  string res;
  while(i < n && !(x[i] == c && !is_escaped(x, i))){
    res += x[i++];
  }
  
  return res;
}
  
inline string trim_WS(const string &x){
  const int &n = x.size();
  int i_end = n - 1;
  
  while(i_end >= 0 && x[i_end] == ' '){
    --i_end;
  }
  
  if(i_end < 0){
    return "";
  }
  
  int i_start = 0;
  while(i_start < i_end && x[i_start] == ' '){
    ++i_start;
  }
  
  return x.substr(i_start, i_end - i_start + 1);
}

inline void trim_WS_inplace(string &x){
  int i_end = static_cast<int>(x.size()) - 1;
  
  while(i_end >= 0 && x[i_end] == ' '){
    --i_end;
  }
  
  if(i_end < 0){
    x.clear();
    return;
  }
  
  if(i_end < static_cast<int>(x.size()) - 1){
    x.erase(i_end + 1);
  }
  
  int i_start = 0;
  while(i_start < i_end && x[i_start] == ' '){
    --i_start;
  }
  
  if(i_start != 0){
    x.erase(x.begin(), x.begin() + i_start);
  }
  
}

inline string trim_WS_rm_quotes(const string &x){
  string res = trim_WS(x);
  if(res.size() < 2){
    return res;
  }
  
  if(is_quote(res[0])){
    char quote = res[0];
    if(res.back() == quote){
      res.pop_back();
      res.erase(res.begin());
    }
  }
  
  return res;
}

inline string delete_until(const string &x, string value){
  // NOTA: we accept the `+` as special character, when last
  
  if(value.empty()){
    util::error_msg("str::delete_until(", dquote(x), ", ", dquote(value), "): ",
                    "the argument `value` cannot be empty.");
    return "";
  }
  
  const int n_x = x.size();
  
  bool is_plus = false;
  if(value.size() > 1 && value.back() == '+'){
    value.pop_back();
    is_plus = true;
  }
  
  const int n_val = value.size();
  
  for(int i = 0 ; i < n_x ; ++i){
    
    if(i + n_val - 1 >= n_x){
      return "";
    }
    
    if(x[i] == value[0]){
      bool found = true;
      for(int j = 1 ; j < n_val ; ++j){
        if(x[i + j] != value[j]){
          found = false;
          break;
        }
      }
      
      if(found){
        int index = i + n_val;
        
        if(is_plus){
          const char last_char = value.back();
          while(index < n_x && x[index] == last_char){
            ++index;
          }
        }
        
        if(index >= n_x){
          return "";
        }
        
        return x.substr(index);
      }
      
    }
  }
  
  return "";
}

inline string delete_until(const string &x, const char *pvalue){
  return delete_until(x, string{pvalue});
}

inline string delete_until(const string &x, std::initializer_list<string> all_values_init){
  // we stop at the first match
  
  const vector<string> all_values(all_values_init);
  
  if(all_values.empty()){
    return x;
  }
  
  if(all_values.size() == 1){
    return delete_until(x, all_values[0]);
  }
  
  string res;
  bool init = true;
  for(const auto &v : all_values){
    const string tmp = delete_until(x, v);
    
    if(init){
      init = false;
      res = tmp;
    } else if(tmp.size() > res.size()){
      res = tmp;
    }
  }
  
  return res;
}

inline string delete_after(const string &x, const string &value){
  if(value.empty()){
    util::error_msg("str::delete_after(", dquote(x), ", ", dquote(value), "): ",
                    "the argument `value` cannot be empty.");
    return "";
  }
  
  const int n_x = x.size();
  const int n_val = value.size();
  
  for(int i = 0 ; i < n_x ; ++i){
    
    if(i + n_val - 1 >= n_x){
      return x;
    }
    
    if(x[i] == value[0]){
      bool found = true;
      for(int j = 1 ; j < n_val ; ++j){
        if(x[i + j] != value[j]){
          found = false;
        }
      }
      
      if(found){
        return x.substr(0, i);
      }
      
    }
  }
  
  return x;
}

inline string delete_after(const string &x, const char *pvalue){
  return delete_after(x, string{pvalue});
}

inline string delete_after(const string &x, std::initializer_list<string> all_values_init){
  
  const vector<string> all_values(all_values_init);
  
  if(all_values.empty()){
    return x;
  }
  
  if(all_values.size() == 1){
    return delete_after(x, all_values[0]);
  }
  
  string res;
  bool init = true;
  for(const auto &v : all_values){
    const string tmp = delete_after(x, v);
    
    if(tmp.empty()){
      return "";
    }
    
    if(init){
      init = false;
      res = tmp;
    } else if(tmp.size() < res.size()){
      res = tmp;
    }
  }
  
  return res;
}



inline string delete_pattern_internal(const string &str, const vector<string> &all_targets){
  
  string x = str;
  int n = x.size();
  int i = 0;
  
  for(const auto &target : all_targets){
    
    if(target.empty()){
      continue;
    }
    
    const int n_target = target.size();
    
    while(i < n - n_target + 1){
      
      if(x[i] == target[0]){
        bool found = true;
        for(int j = 1 ; j < n_target ; ++j){
          if(x[i + j] != target[j]){
            found = false;
            break;
          }
        }
        
        if(found){
          x.erase(x.begin() + i, x.begin() + i + n_target);
          n = x.size();
        } else {
          ++i;
        }
        
      } else {
        ++i;
      }
    
    }
  }
  
  return x;
}

inline string delete_pattern(const string &str, std::initializer_list<string> all_values_init){
  const vector<string> all_targets(all_values_init);
  return delete_pattern_internal(str, all_targets);
}

inline string delete_pattern(const string &x, const char *target){
  const vector<string> all_targets{target};
  return delete_pattern_internal(x, all_targets);
}

inline int count_char(const string &x, const char c){
  int res = 0;
  for(auto &xi : x){
    if(xi == c){
      ++res;
    }
  }
  
  return res;
}

string str_replace(const string &x, const string &from, const string &to);

inline vector<string> str_split(const string &x, std::initializer_list<string> all_values){
  // NOTA: `+` is a special character when last
  // => it works like a simplified regex, the last item it repeated
  // 
  
  vector<string> all_values_v = vector<string>{all_values};
  
  if(all_values_v.empty()){
    util::error_msg("str::str_split(", dquote(x), ", [empty]): ",
                    "the argument `all_values` cannot be empty.");
    return vector<string>{x};
  }
  
  const int n_x = x.size();
  const size_t n_values = all_values_v.size();
  
  // is there the special character `+`?
  vector<bool> is_plus(n_values, false);
  for(size_t i = 0 ; i < n_values ; ++i){
    string &value = all_values_v[i];
    if(value.size() > 1 && value.back() == '+'){
      is_plus[i] = true;
      value.pop_back();
    }
  }
  
  vector<string> res;
  string tmp;
  
  int i = 0;
  while(i < n_x){
    
    for(size_t k = 0 ; k < all_values_v.size() ; ++k){
      string &value = all_values_v[k];
      
      if(!value.empty() && x[i] == value[0]){
        bool found = true;
        for(size_t j = 1 ; j < value.size() ; ++j){
          if(x[i + j] != value[j]){
            found = false;
          }
        }
        
        if(found){
          
          res.push_back(tmp);
          i += value.size();
          
          if(is_plus[k]){
            const char last_char = value.back();
            while(i < n_x && x[i] == last_char){
              ++i;
            }
          }
          
          tmp.clear();
          
          continue;
        }
        
      }
    }
    
    
    tmp += x[i++];
    
  }
  
  res.push_back(tmp);
  
  return res;
}

inline vector<string> str_split(const string &x, const string &value){
  return str_split(x, {value});
}

inline vector<string> str_split(const string &x, const char *pvalue){
  return str_split(x, string{pvalue});
}

vector<string> str_split_at_width(const string &x, const int width);

inline void right_fill_with_space_inplace(string &x, const int n, const bool ignore_vts = true){
  
  if(n < 0){
    util::error_msg("right_fill_with_space_inplace: argument n cannot be negative.");
    return;
  }
  
  const size_t nx_clean = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
  if(nx_clean < static_cast<size_t>(n)){
    x += string(n - nx_clean, ' ');
  }
  
}

inline string right_fill_with_space(const string &x, const int n, const bool ignore_vts = false){
  
  if(n < 0){
    util::error_msg("right_fill_with_space: argument n cannot be negative.");
    return x;
  }
  
  string res = x;
  right_fill_with_space_inplace(res, n, ignore_vts);
  
  return res;
}

inline void right_fill_with_space_inplace(vector<string> &all_x, const int n = 0, bool ignore_vts = false){
  
  if(n < 0){
    util::error_msg("right_fill_with_space_inplace: argument n cannot be negative.");
    return;
  }
  
  size_t max_n = n;
  if(max_n == 0){
    if(all_x.size() < 2){
      return;
    }
    
    for(const auto &x : all_x){
      const size_t s = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
      if(s > max_n){
        max_n = s;
      }
    }
  }
  
  for(auto &x : all_x){
    const size_t s = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
    if(s < max_n){
      x += string(max_n - s, ' ');
    }
  }
}

inline vector<string> right_fill_with_space(const vector<string> &all_x, const int n = 0, bool ignore_vts = false){
  
  if(n < 0 || (n == 0 && all_x.size() < 2)){
    return all_x;
  }
  
  vector<string> res = all_x;
  
  right_fill_with_space_inplace(res, n, ignore_vts);
  
  return res;
}


inline void left_fill_with_space_inplace(string &x, const int n, const bool ignore_vts = false){
  
  if(n < 0){
    util::error_msg("left_fill_with_space_inplace: argument n cannot be negative.");
    return;
  }
  
  const size_t nx_clean = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
  if(nx_clean < static_cast<size_t>(n)){
    x = string(n - nx_clean, ' ') + x;
  }
  
}

inline string left_fill_with_space(const string &x, const int n, const bool ignore_vts = false){
  
  if(n < 0){
    util::error_msg("left_fill_with_space: argument n cannot be negative.");
    return x;
  }
  
  string res = x;
  left_fill_with_space_inplace(res, n, ignore_vts);
  
  return res;
}

inline void left_fill_with_space_inplace(vector<string> &all_x, const int n = 0, bool ignore_vts = false){
  
  if(n < 0){
    util::error_msg("left_fill_with_space_inplace: argument n cannot be negative.");
    return;
  }
  
  size_t max_n = n;
  if(max_n == 0){
    if(all_x.size() < 2){
      return;
    }
    
    for(const auto &x : all_x){
      const size_t s = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
      if(s > max_n){
        max_n = s;
      }
    }
  }
  
  for(auto &x : all_x){
    const size_t s = ignore_vts ? size_no_vts(x) : utf8::count_wide_chars(x);
    if(s < max_n){
      x = string(max_n - s, ' ') + x;
    }
  }
}

inline vector<string> left_fill_with_space(const vector<string> &all_x, const int n = 0, bool ignore_vts = false){
  
  if(n < 0 || (n == 0 && all_x.size() < 2)){
    return all_x;
  }
  
  vector<string> res = all_x;
  left_fill_with_space_inplace(res, n, ignore_vts);
  
  return res;
}


inline string collapse(const vector<string> &x, const string &sep){
  string res = x.empty() ? "" : x.at(0);
  for(size_t i = 1 ; i < x.size() ; ++i){
    res += sep + x[i];
  }
  
  return res;
}

class EnumOpts {
  bool _is_or = false;
  string quote = "";
  
public:
  
  EnumOpts &add_or(){
    _is_or = true;
    return *this;
  }
  
  EnumOpts &squote(){
    quote = '\'';
    return *this;
  }
  
  EnumOpts &dquote(){
    quote = '"';
    return *this;
  }
  
  EnumOpts &bquote(){
    quote = '`';
    return *this;
  }
  
  bool has_quote() const {
    return !quote.empty();
  }
  
  string get_quote() const {
    return quote;
  }
  
  bool is_or() const {
    return _is_or;
  }
};

string enumerate(const vector<string> &x, const EnumOpts opts = EnumOpts());


vector<string> align_on_columns(const vector<string> &x, const size_t width, 
                                const string sep = " | ", bool ignore_vts = false);
  

inline string bool_to_string(bool x){
  return x ? "true" : "false";
}

inline string nth(int n){
  switch(n){
    case 1: return "first";
    case 2: return "second";
    case 3: return "third";
    case 4: return "fourth";
    case 5: return "fifth";
    case 6: return "sixth";
    case 7: return "seventh";
    default: {
      int rest = n % 10;
      return std::to_string(n) + (rest == 1 ? "st" : (rest == 2 ? "nd" : (rest == 3 ? "rd" : "th")));
    }
  }
  
  return "ERROR nth()";
}

inline string fit_screen(const vector<string> &all_values, int width = 80){
  
  string res;
  string line;
  bool first_line = true;
  
  for(const auto &v : all_values){
    
    if(first_line){
      first_line = false;
      line = v;
      
    } else {
      const int n = v.size();
      if(static_cast<int>(line.size()) + n > width){
        
        if(res.empty()){
          res = line;
        } else {
          res += "\n" + line;
        }
        
        line = v;
        
      } else {
        line += ", " + v;
      }
    }
  }
  
  if(res.empty()){
    res = line;
  } else {
    res += "\n" + line;
  }
  
  return res;
}

inline uint go_to_next_inner_paren(const string &x, int i, bool left){
  // i corresponds to a cursor position
  
  const int n = x.size();
  
  if(left){
    
    //    hey(you(are(you(ok))))|
    // => hey(you(are(you(|ok))))
    
    // starting point: skipping adjacent
    --i;
    while(i >= 0 && is_paren(x[i])){
      --i;
    }
    
    while(i >= 0 && !is_paren(x[i])){
      --i;
    }
    
  } else {
    
    // starting point: skipping adjacent
    while(i < n && is_paren(x[i])){
      ++i;
    }
    
    while(i < n && !is_paren(x[i])){
      ++i;
    }
  }
  
  //
  // return
  //
  
  if(i >= 0 && i < n && is_paren(x[i])){
    
    // right
    //    |hello(you()) ; how(are)
    // =>  hello(|you()) ; how(are)
    
    if(is_opening_paren(x[i])){
      return i + 1;
    } else {
      //    hello(you()) ; |how(are)
      // => hello(you()|) ; how(are)
      return i;
    }
    
  } else if(i <= 0) {
    return 0;
  } else {
    return n;
  }
  
}

//
// non-inline ------------------------------------------------------------------ 
//

vector<uint> contextual_selection(const string &, const uint, const uint, const bool has_moved = false);
void set_valid_word_char(string x);

//
// string_utf8 -----------------------------------------------------------------
//

class string_utf8 {
  string line;
  bool is_wide = false;
  uint wide_line_size = 0;
  
  uint wide_to_narrow_count_at(uint, uint) const;
  
public:
  
  // constructors
  string_utf8() = default;
  string_utf8(const string &x): line(), is_wide(false), wide_line_size(0) { push_back(x); };
  string_utf8(const char *x): line(), is_wide(false), wide_line_size(0) { push_back(string{x}); };
  
  // operators
  uchar operator[](uint) const;
  string at(uint) const;
  
  // modify content
  void clear(){
    line.clear();
    is_wide = false;
    wide_line_size = 0;
  }
  
  void insert(uint, string);
  void push_back(string);
  void push_back(char);
  void erase(uint, uint);
  string substr(uint, uint) const;
  vector<string_utf8> split(uint) const;
  
  uint word_jump(uint index_wide, int side) const;
  uint word_delete(uint index_wide, int side, const bool stop_at_slash = false) const;
  
  uint word_jump_default(uint index_wide, int side) const{
    const string valid_bak = valid_word_char;
    valid_word_char = "_";
    uint res = word_jump(index_wide, side);
    valid_word_char = valid_bak;
    return res;
  }
  
  uint word_delete_default(uint index_wide, int side) const {
    const string valid_bak = valid_word_char;
    valid_word_char = "_";
    uint res = word_delete(index_wide, side, true);
    valid_word_char = valid_bak;
    
    return res;
  }
  
  uint size() const { return wide_line_size; };
  bool empty() const { return line.empty(); };
  uint narrow_size() const { return line.size(); };
  
  string str() { return line; };
  const string& str() const { return line; };
  
  uint narrow_to_wide_index(uint) const;
  uint wide_to_narrow_index(uint) const;
  
};

string_utf8 trim_WS(const string_utf8 &x);

string_utf8 shorten(const string_utf8 &x, const uint nmax, const string ellipsis = "\u2026");
vector<string_utf8> shorten(const vector<string_utf8> &all_x, const uint nmax, const string ellipsis = "\u2026");
void shorten_inplace(vector<string_utf8> &all_x, const uint nmax, const string ellipsis = "\u2026");


//
// MatchedStrings --------------------------------------------------------------
//

// class telling the starting and ending positions of string matches
class MatchInfo {
  vector<uint> start;
  vector<uint> end;
  int n = 0;
public:
  
  MatchInfo() = default;
  MatchInfo(uint index, uint len): n(1){
    start.push_back(index);
    end.push_back(index + len);
  }
  MatchInfo(uint len): n(1){
    start.push_back(0);
    end.push_back(len);
  }
  
  void add(uint new_start, uint len){
    // we always order and merge if appropriate
    uint new_end = new_start + len;
    if(start.empty()){
      start.push_back(new_start);
      end.push_back(new_start + len);
      ++n;
      
    } else if(new_start > end[n - 1]){
      start.push_back(new_start);
      end.push_back(new_start + len);
      ++n;
      
    } else if(new_end < start[0]){
      start.insert(start.begin(), new_start);
      end.insert(end.begin(), new_end);
      ++n;
      
    } else {
      /* General algorithm
      * 
      * Let [ correspond to start values and ] to end values. The existing 
      * sequences are of the form
      *              [     ] [ ]        [   ]
      * CASE 1:  [ ]  => non overlap => we just add at the right location
      * CASE 2:       [ => new_start is included in a range
      * CASE 3:         ] => new_end is included in a range
      * 
      * CASE 1 is simple, we just insert the values at the right location.
      * 
      * CASE 2: we don't insert but may just have to update the existing set (note 
      *   that depending on the ending value, we may delete entries)
      * 
      * CASE 3: we also don't insert and just update the existing values with possibly
      *   the deletion of some entries
      * 
      * We try to handle all the cases at once
      * 
      * */
      
      int i = 0;
      while(i < n && start[i] < new_start){
        ++i;
      }
      // out, we have, either:
      // - start[i] >= new_start
      // - i == n
      // 
      
      bool is_start_included = false;
      if(i > 0 && end[i - 1] >= new_start){
        is_start_included = true;
      }
      
      // this is the would be index of the new starting position
      int index = i;
      
      bool is_end_included = false;
      i = index == 0 ? 0 : index - 1;
      while(i < n && end[i] < new_end){
        ++i;
      }
      // out, we have, either:
      // - end[i] >= new_end
      // - i == n
      // 
      int index_bis = i;
      
      if(i < n && start[i] < new_end){
        is_end_included = true;
      }
      
      // example I -------------------------------------------------------------
      // 
      // start: 1 11
      //   end: 3 17
      //
      // new_start:  9
      //   new_end: 14
      // 
      // n: 2
      // index:     1
      // index_bis: 1
      // is_start_included: false
      // is_end_included:   true
      // 
      //
      // example II ------------------------------------------------------------
      // 
      // start: 1 11
      //   end: 3 17
      //
      // new_start:  2
      //   new_end: 14
      // 
      // n: 2
      // index:     1
      // index_bis: 1
      // is_start_included: true
      // is_end_included:   true
      // 
      //
      // example III -----------------------------------------------------------
      // 
      // start: 1 11
      //   end: 3 17
      //
      // new_start:  5
      //   new_end: 19
      // 
      // n: 2
      // index:     1
      // index_bis: 2
      // is_start_included: false
      // is_end_included:   false
      // 
      // example IV ------------------------------------------------------------
      // 
      // start: 1 11
      //   end: 3 17
      //
      // new_start: 12
      //   new_end: 16
      // 
      // n: 2
      // index:     2
      // index_bis: 1
      // is_start_included: true
      // is_end_included:   true
      // 
      // example V -------------------------------------------------------------
      // 
      // start: 1 11
      //   end: 3 17
      //
      // new_start: 12
      //   new_end: 19
      // 
      // n: 2
      // index:     2
      // index_bis: 2
      // is_start_included: true
      // is_end_included:   false
      // 
      // example VI ------------------------------------------------------------
      // 
      // start: 7 11
      //   end: 9 17
      //
      // new_start: 5
      //   new_end: 6
      // 
      // n: 2
      // index:     0
      // index_bis: 0
      // is_start_included: false
      // is_end_included:   false
      // 
      // example VII -----------------------------------------------------------
      // 
      // start: 7 11
      //   end: 9 17
      //
      // new_start:  5
      //   new_end: 12
      // 
      // n: 2
      // index:     0
      // index_bis: 1
      // is_start_included: false
      // is_end_included:   true
      // 
      
      if(is_start_included){
        --index;
        new_start = start[index];
      }
      
      if(is_end_included){
        new_end = end[index_bis];
      }
      
      int n_del = (index_bis + is_end_included) - index;
      if(n_del > 0){
        start.erase(start.begin() + index, start.begin() + index + n_del - 1);
        end.erase(end.begin() + index, end.begin() + index + n_del - 1);
      }
      // then we insert
      start.insert(start.begin() + index, new_start);
      end.insert(end.begin() + index, new_end);
      
      n = start.size();
    }
  }
  
  uint size() const { return n; }
  
  uint start_at(int i) const { 
    if(i >= n){
      std::cout << "ERROR: wrong indexing in MatchInfo.start_at, " << i;
      std::cout << " is greater than the number of elements " << n << "\n";
      return 0;
    }
    return start[i]; 
  }
  
  uint end_at(int i) const { 
    if(i >= n){
      std::cout << "ERROR: wrong indexing in MatchInfo.end_at, " << i;
      std::cout << " is greater than the number of elements " << n << "\n";
      return 0;
    }
    return end[i]; 
  }
};

class StringMatch {
  // class with the string and the index at which the matching is found
  string target; 
  MetaStringVec all_matches;
  vector<MatchInfo> all_match_info;
  vector<uint> all_id;
  string cause_no_match;
  uint target_size_wide = 0;
  
  void check_size_str_match_info(){
    if(all_matches.size() != all_match_info.size()){
      std::cerr << "In StringMatch, the size of vectors matched (" << all_matches.size();
      std::cerr << " differ from the size of the match info (" << all_match_info.size() << ").\n";
    }
  }
  
  void check_size_str_id(){
    if(all_matches.size() != all_id.size()){
      std::cerr << "In StringMatch, the size of vectors matched (" << all_matches.size();
      std::cerr << " differ from the size of the identifiers (" << all_id.size() << ").\n";
      all_id.clear();
    }
  }
  
public:
  StringMatch() = default;

  StringMatch(string target_in, MetaStringVec str_vec):
    target(target_in), all_matches(str_vec)
  {
    target_size_wide = utf8::count_wide_chars(target);
    // By default: if no index is provided, they're all at 0
    const uint n = all_matches.size();
    for(uint i=0 ; i<n ; ++i){
      all_match_info.push_back(MatchInfo(target_size_wide));
      all_id.push_back(i);
    }
    
    if(all_matches.empty()){
      cause_no_match = all_matches.get_cause_empty();
    }
  };
  
  StringMatch(string target_in, MetaStringVec str_vec, vector<MatchInfo> match_info_vec):
    target(target_in), all_matches(str_vec), all_match_info(match_info_vec) 
  {
    target_size_wide = utf8::count_wide_chars(target);
    check_size_str_match_info();
  };
  
  StringMatch(string target_in, MetaStringVec str_vec, vector<MatchInfo> match_info_vec,
              vector<uint> all_id_vec):
    target(target_in), all_matches(str_vec), all_match_info(match_info_vec), all_id(all_id_vec) 
  {
    target_size_wide = utf8::count_wide_chars(target);
    check_size_str_match_info();
    check_size_str_id();
  };
  
  StringMatch(string target_in, MetaStringVec str_vec, vector<MatchInfo> match_info_vec, string reason):
    target(target_in), all_matches(str_vec), all_match_info(match_info_vec), cause_no_match(reason)
  {
    target_size_wide = utf8::count_wide_chars(target);
    check_size_str_match_info();
  };
  
  StringMatch(string target_in, MetaStringVec str_vec, vector<MatchInfo> match_info_vec,
              vector<uint> all_id_vec, string reason):
    target(target_in), all_matches(str_vec), all_match_info(match_info_vec), 
    all_id(all_id_vec), cause_no_match(reason)
  {
    target_size_wide = utf8::count_wide_chars(target);
    check_size_str_match_info();
    check_size_str_id();
  };
  
  uint size() const { return all_matches.size(); };
  bool empty() const { return all_matches.empty(); };
  
  string string_at(uint i) const { return i < all_matches.size() ? all_matches.string_at(i) : ""; }
  
  MatchInfo match_info_at(uint i) const { 
    return i < all_match_info.size() ? all_match_info[i] : MatchInfo(target_size_wide);
  }
  
  uint id_at(uint i) const {
    if(all_id.size() == 0){
      std::cerr << "In StringMatch.id_at: cannot access id since it is empty\n";
      return 0;
    }
    return i < all_id.size() ? all_id[i] : 0; 
  }
  
  StringMatch& set_cause_no_match(const string &x){ 
    cause_no_match = x;
    return *this;
  }
  
  StringMatch& default_cause_no_match(string x){
    if(cause_no_match.empty()){
      cause_no_match = x;
    }
    return *this;
  }
  
  string get_cause_no_match() const { return cause_no_match; }
  string get_target() const { return target; }
  uint get_target_size_wide() const { return target_size_wide; }
  
  MetaStringVec at(size_t i) const {
    if(i > all_matches.size()){
      throw std::out_of_range("The index requested is larger than the current number of matches.");
    }
    
    return all_matches.at(i);
  }
  
  bool has_meta(const string &x) const {
    return all_matches.has_meta(x);
  }
  
  vector<string> get_meta_vector(const string &x) const {
    return all_matches.meta(x);
  }
  
  vector<string> get_matches() const {
    return all_matches.get_string_vec();
  }
  
};

StringMatch string_match(const string &query, MetaStringVec choices);
inline StringMatch string_match(const string &query, const vector<string> &choices){
  MetaStringVec msv = choices;
  return string_match(query, msv);
}

//
// ParenMatcher ----------------------------------------------------------------
//

class ParenMatcher {
  ParenMatcher() = delete;
public:
  bool pair_found = false;
  uint i_start = UNSET::UINT;
  uint i_end = UNSET::UINT;
  
  ParenMatcher(string, uint);
};

//
// inline string_utf8 functions ------------------------------------------------ 
//

inline bool is_paren_adjacent(string_utf8 x, uint cursor_pos){
  if(x.size() == 0){
    return false;
  }
  
  uint narrow_cursor_pos = x.wide_to_narrow_index(cursor_pos);
  const string &str = x.str();
  
  return is_paren_adjacent(str, narrow_cursor_pos);
}

vector<uint> contextual_selection(const string_utf8 &, const uint, const uint);


} // namespace stringtools
