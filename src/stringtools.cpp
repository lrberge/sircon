    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#include "stringtools.hpp"

namespace stringtools {

string valid_word_char = "._";
void set_valid_word_char(string x){
  valid_word_char = x;
}

using namespace utf8;

// debug function
void display_charvalue(string str, bool newline){
  std::cout << (newline ? "\n" : "") << "uchar '" << str << "' = ";
  for(size_t i=0 ; i<str.size() ; ++i){
    unsigned char c = static_cast<unsigned char>(str[i]);
    std::cout << " " << static_cast<unsigned int>(c);
  }
  std::cout << std::endl;
}

void string_utf8::insert(uint wide_at, string str){
  // we can insert full strings and not just strings of size 1 wide char
  
  uint narrow_n = str.size();
  uint wide_n = count_wide_chars(str);
  
  if(is_wide || narrow_n > wide_n){
    
    uint narrow_at = wide_at;
    if(!is_wide){
      is_wide = true;
    } else {
      narrow_at = wide_to_narrow_index(wide_at);
    }
    
    line.insert(narrow_at, str);
    
  } else {
    // here wide and narrow representations are equivalent
    line.insert(wide_at, str);
  }
  
  wide_line_size += wide_n;
}

void string_utf8::push_back(string str){
  insert(wide_line_size, str);
}

void string_utf8::push_back(char c){
  insert(wide_line_size, string{c});
}

void string_utf8::erase(uint wide_i, uint wide_n){
  if(is_wide){
    uint narrow_i = wide_to_narrow_index(wide_i);
    uint narrow_n = wide_to_narrow_count_at(narrow_i, wide_n);
    
    line.erase(narrow_i, narrow_n);
    wide_line_size -= wide_n;
    
  } else {
    // narrow = wide
    line.erase(wide_i, wide_n);
    wide_line_size = line.size();
  }
}

string string_utf8::substr(uint wide_pos, uint wide_n) const {
  
  string res;
  
  if(wide_pos >= wide_line_size){
    return res;
  }
  
  if(is_wide){
    const uint narrow_pos = wide_to_narrow_index(wide_pos);
    uint narrow_n = wide_to_narrow_count_at(narrow_pos, wide_n);
    
    res = line.substr(narrow_pos, narrow_n);
  } else {
    res = line.substr(wide_pos, wide_n);
  }
  
  return res;
}

vector<string_utf8> string_utf8::split(uint wide_i) const {
  
  const uint narrow_i = is_wide ? wide_to_narrow_index(wide_i) : wide_i;
  
  string before = line.substr(0, narrow_i);
  string after = line.substr(narrow_i, line.size() - narrow_i);
  
  vector<string_utf8> res(2);
  res[0] = string_utf8(before);
  res[1] = string_utf8(after);
  
  return res;
}

inline uint string_utf8::wide_to_narrow_count_at(uint narrow_i, uint wide_n) const {
  // narrow_i: the conversion to narrow must have been performed beforehand
  
  if(is_wide){
    uint narrow_n = 0;
    uint n_done = 0;
    while(n_done < wide_n){
      ++n_done;
      ++narrow_n;
      while(narrow_i + narrow_n < line.size() && is_continuation(line[narrow_i + narrow_n])){
        ++narrow_n;
      }
    }
    
    return narrow_n;
    
  } else {
    // narrow = wide
    return wide_n;
  }
  
}

uint string_utf8::wide_to_narrow_index(uint wide_index) const{
  // the index is the STARTING character
  // 
  
  if(!is_wide){
    return wide_index;
  }
  
  if(wide_index == wide_line_size){
    return line.size();
  }
  
  if(wide_index == 0){
    return 0;
  }
  
  // wide, not a special case
  
  uint narrow_index = 0;
  for(uint wide_i = 0 ; wide_i < wide_index ; ++wide_i){
    // at each loop iteration, we go to the next starting byte
    //
    // start of this loop: we're at the starting byte of a character
    narrow_index++;
    // if we end up at starting byte: good
    // else: we go to the next starting byte
    // we cannot reach the end of the string w/t it being a strating byte bc this case is handled before
    
    while(!is_starting_byte(line[narrow_index])){
      narrow_index ++;
    }
  }
  
  return narrow_index;
}

uint string_utf8::narrow_to_wide_index(uint narrow_index) const{
  // narrow index refers to a STARTING byte
  // this function gives the unicode index of that byte
  
  if(!is_wide){
    return narrow_index;
  }
  
  if(narrow_index == line.size()){
    return wide_line_size;
  }
  
  if(wide_line_size == 0){
    return 0;
  }
  
  // wide, not a special case
  
  uint wide_index = 0;
  uint narrow_i = 0;
  while(wide_index <= wide_line_size && narrow_i < narrow_index){
    ++wide_index;
    // NOTA: we always move narrow_i to the ending byte of the *next* utf8 char
    // so that ++narrow_i is always the starting byte of a new char
    move_i_to_end_of_next_utf8_char(line, narrow_i);
  }

  return wide_index;
}

uint string_utf8::word_jump(uint wide_i, int side) const {
  
  if(wide_line_size == 0){ return 0; }
  
  uint wide_pos;
  if(is_wide){
    // NOTE: wide_i goes from 0 to n with n the wide_line_size
    uint narrow_i = wide_to_narrow_index(wide_i);
    uint new_narrow_pos = stringtools::word_jump(line, narrow_i, side);
    
    wide_pos = narrow_to_wide_index(new_narrow_pos);
    
  } else {
    wide_pos = stringtools::word_jump(line, wide_i, side);
  }
  
  return wide_pos;
}

uint string_utf8::word_delete(uint wide_i, int side) const {
  
  if(wide_line_size == 0){
    return 0;
  }
  
  uint wide_pos;
  if(is_wide){
    // NOTE: wide_i goes from 0 to n with n the wide_line_size
    uint narrow_i = wide_to_narrow_index(wide_i);
    uint new_narrow_pos = stringtools::word_delete(line, narrow_i, side);
    // we convert narrow_pos into wide_res
    
    wide_pos = narrow_to_wide_index(new_narrow_pos);
    
  } else {
    wide_pos = stringtools::word_delete(line, wide_i, side);
  }
  
  return wide_pos;
}

uchar string_utf8::operator[](uint wide_i) const {
  uint narrow_i = wide_to_narrow_index(wide_i);
  return line[narrow_i];
}

string string_utf8::at(uint wide_i) const {
  string res;
  uint narrow_i = wide_to_narrow_index(wide_i);
  res += line[narrow_i];
  while(narrow_i + 1 < line.size() && is_continuation(line[narrow_i + 1])){
    res += line[narrow_i + 1];
  }
  
  return res;
}


//
// Other functions 
//

// these functions rely on windows.h



string utf16_to_utf8_internal(wchar_t *p_utf16, const int char_size_in, bool &is_error){
  const DWORD flag{WC_ERR_INVALID_CHARS};
  
  int char_size_out = WideCharToMultiByte(CP_UTF8, flag, p_utf16, char_size_in, nullptr, 0, nullptr, nullptr);
  if(char_size_out == 0){
    std::cout << std::endl << "Error when converting to multibyte character: " << std::endl;
    
    DWORD err = GetLastError();

    if(err == ERROR_INSUFFICIENT_BUFFER){ std::cout << "A supplied buffer size was not large enough, or it was incorrectly set to NULL." << std::endl; }
    else if(err == ERROR_INVALID_FLAGS){ std::cout << "The values supplied for flags were not valid." << std::endl; }
    else if(err == ERROR_INVALID_PARAMETER){ std::cout << "Any of the parameter values was invalid." << std::endl; }
    else if(err == ERROR_NO_UNICODE_TRANSLATION){ std::cout << "Invalid Unicode was found in a string." << std::endl; }
    else { std::cout << "Unidentified error" << std::endl; }
    
    is_error = true;
    return "";
  }
  
  string utf8_string(static_cast<size_t>(char_size_out), '\0');
  WideCharToMultiByte(CP_UTF8, flag, p_utf16, char_size_in, &utf8_string[0], char_size_out, nullptr, nullptr);
  
  return utf8_string;
}

string utf16_to_utf8(wchar_t utf16_char, bool &is_error){
  return utf16_to_utf8_internal(&utf16_char, 1, is_error);
}

string utf16_to_utf8(wstring &str, bool &is_error){
  return utf16_to_utf8_internal(str.data(), str.size(), is_error);
}


wstring utf8_to_utf16_internal(char *p_utf8, const int char_size_in, bool &is_error){
  
  int char_size_out = MultiByteToWideChar(CP_UTF8, 0, p_utf8, char_size_in, nullptr, 0);
  if(char_size_out == 0){
    std::cout << std::endl << "Error when converting to wide character: " << std::endl;
    
    DWORD err = GetLastError();

    if(err == ERROR_INSUFFICIENT_BUFFER){ std::cout << "A supplied buffer size was not large enough, or it was incorrectly set to NULL." << std::endl; }
    else if(err == ERROR_INVALID_FLAGS){ std::cout << "The values supplied for flags were not valid." << std::endl; }
    else if(err == ERROR_INVALID_PARAMETER){ std::cout << "Any of the parameter values was invalid." << std::endl; }
    else if(err == ERROR_NO_UNICODE_TRANSLATION){ std::cout << "Invalid Unicode was found in a string." << std::endl; }
    else { std::cout << "Unidentified error" << std::endl; }
    
    is_error = true;
    return L"";
  }
  
  wstring utf16_string(static_cast<size_t>(char_size_out), '\0');
  MultiByteToWideChar(CP_UTF8, 0, p_utf8, char_size_in, &utf16_string[0], char_size_out);
  
  return utf16_string;
}

wstring utf8_to_utf16(char utf8_char, bool &is_error){
  return utf8_to_utf16_internal(&utf8_char, 1, is_error);
}

wstring utf8_to_utf16(string &str, bool &is_error){
  return utf8_to_utf16_internal(str.data(), str.size(), is_error);
}


string_utf8 trim_WS(const string_utf8 &x){
  
  if(x.size() == 0 || !(is_WS(x[0]) || is_WS(x[x.size() - 1]))){
    return x;
  }
  
  string line = trim_WS(x.str());
  return string_utf8{line};
}

//
// string_match -----------------------------------------------------------------
//

inline bool is_same_letter(const char c1, const char c2){
  if(c1 == c2){
    return true;
    
  } else if(c1 > c2){
    if(c1 - c2 == 32){
      return c1 >= 'a' && c1 <= 'z';
    }
    
  } else {
    if(c2 - c1 == 32){
      return c2 >= 'a' && c2 <= 'z';
    }
  }
  
  return false;
}

inline bool startmatch_misspell(const string x, const string y, const uint index_y = 0){
  /*
  *  Algorithm:
  * 
  * - deletion:  x = 'bon' y = 'bnjour'  => true
  * 
  * - insertion: x = 'bn'  y = 'bonjour' => true
  * 
  * - swap:      x = 'bon' y = 'bnojour' => true
  *              x = 'bno' y = 'bonjour' => true
  * 
  * */
  
  const uint nx = x.size();
  const uint ny = y.size();
  
  if(y.size() + 1 < x.size() + index_y){
    return false;
  }
  
  uint i = 0;
  while(i < nx && i + index_y < ny && is_same_letter(x[i], y[index_y + i])){
    ++i;
  }
  
  if(i == nx || i + index_y == ny){
    // we went all the way => fine
    // note that the second condition corresponds to a deletion
    return true;
  }
  
  // i represents the index of the first unmatch
  
  //
  // step 1: deletion 
  //
  
  // x = 'bonjour' y = 'bojour_les_gens
  //        |             |
  // i = 2
  
  int i_save = i;
  if(i + 1 < nx && is_same_letter(x[i + 1], y[index_y + i])){
    ++i;
    while(i + 1 < nx && i + index_y < ny && is_same_letter(x[i + 1], y[index_y + i])){
      ++i;
    }
    
    if(i + 1 == nx){
      // this is the only way to match
      return true;
    }
    
    // we continue
    i = i_save;
  }
  
  //
  // step 2: inclusion 
  //
  
  // x = 'bojour' y = 'bonjour_les_gens
  //        |            |
  // i = 2
  
  
  if(index_y + i + 1 < ny && is_same_letter(x[i], y[index_y + i + 1])){
    ++i;
    while(i + 1 < nx && i + index_y < ny && is_same_letter(x[i], y[index_y + i + 1])){
      ++i;
    }
    
    if(i + 1 == nx){
      // this is the only way to match
      return true;
    }
    
    // we continue
    i = i_save;
  }
  
  //
  // step 3: swap 
  //
  
  // x = 'bnojour' y = 'bonjour_les_gens
  //       |             |
  // i = 1
  
  if(i + 1 < nx && index_y + i + 1 < ny && is_same_letter(x[i], y[index_y + i + 1]) && is_same_letter(x[i + 1], y[index_y + i])){
    i += 2;
    while(i + 1 < nx && i + index_y < ny && is_same_letter(x[i], y[index_y + i])){
      ++i;
    }
    
    if(i + 1 == nx){
      // this is the only way to match
      return true;
    }
  }
  
  return false;
}

inline bool inclusion_misspell(const string x, const string y, uint &index){
  
  const uint nx = x.size();
  const uint ny = y.size();
  
  if(nx > ny + 1 || nx == 0){
    return false;
  }
  
  // we always match the first letter
  for(uint j=0 ; j<=(ny - nx + 1) ; ++j){
    if(is_same_letter(x[0], y[j])){
      if(startmatch_misspell(x, y, j)){
        index = j;
        return true;
      }
    }
  }
  
  return false;
}

inline bool any_uppercase(string x){
  for(char &c : x){
    if(c >= 'A' && c <= 'Z'){
      return true;
    }
  }
  return false;
}

inline bool startmatch(const string x, const string y, const bool strict_case, const uint index_y = 0){
  // we look at whether x is included in y, starting from index_y
  // ex: x = 'bon', y = 'bonjour' => true
  // ex: x = 'bon', y = 'bonjour', index_y = 1 => false (x = 'on' => true)
  // 
  
  if(y.size() < x.size() + index_y){
    return false;
  }
  
  for(uint i=0 ; i<x.size() ; ++i){
    if(strict_case){
      if(x[i] != y[i + index_y]){
        return false;
      }
    } else {
      if(!is_same_letter(x[i], y[i + index_y])){
        return false;
      }
    }
  }
  
  return true;
}

inline bool inclusion_simple(const string x, const string y, const bool strict_case, uint &index){
  // we check if x is included in y
  // ex: 'heur' in 'rosa bonheur'
  
  if(y.size() < x.size()){
    return false;
  }
  
  const uint nx = x.size();
  const uint ny = y.size();
  
  for(uint j=0 ; j<=(ny - nx) ; ++j){
    if(startmatch(x, y, strict_case, j)){
      index = j;
      return true;
    }
  }
  
  return false;
}

StringMatch string_match(const string &query, MetaStringVec choices){
  
  /***************************************************************************** 
  * The algorithm is as follows
  * 
  * Output: a list of potential matches, the order is related to matching strenght
  * 
  * Here is the strength of each match
  * 1a) startmatch with case (only if x contains an uppercase letter)
  * 1b) startmatch without case
  * if x is 2+ letters:
  *   2) inclusion
  * if x is 4+ letters:
  *   3) inclusion with misspells
  * 
  * If x is a sequence of words, then we also match as follows:
  * 4) all words are included 
  * 5) all words are included with misspells
  * 
  *
  *****************************************************************************/
  
  string x = trim_WS(query);
  
  const uint n = x.size();
  if(x.size() == 0){
    return StringMatch(query, choices).default_cause_no_match("empty query");
  }
  
  const uint n_choices = choices.size();
  if(n_choices == 0){
    return StringMatch(query, choices).default_cause_no_match("no choice available");
  }
  
  const uint nx_wide = utf8::count_wide_chars(x);
  ptr_vec_str pchoices_str = choices.get_string_vec_ptr();
  vec_str &choices_str = *pchoices_str;
  
  vector<MatchInfo> all_match_info;
  vector<uint> all_id;
  vector<bool> is_done(n_choices, false);
  
  uint index = 0;
  
  string msg = n_choices == 1 ? "the only choice does not match" : ("no match found among " + std::to_string(n_choices) + " choices");
  
  //
  // step 1: startmatch 
  //
  
  #define ADD_MATCH(the_match)           \
    all_match_info.push_back(the_match); \
    all_id.push_back(i);                 \
    is_done[i] = true;
  
  const bool any_upper = any_uppercase(x);
  
  if(any_upper){
    for(uint i=0 ; i<n_choices ; ++i){
      if(startmatch(x, choices_str[i], true)){
        ADD_MATCH(MatchInfo(0, nx_wide))
      }
    }
    
    for(uint i=0 ; i<n_choices ; ++i){
      if(!is_done[i] && startmatch(x, choices_str[i], false)){
        ADD_MATCH(MatchInfo(0, nx_wide))
      }
    }
  } else {
    for(uint i=0 ; i<n_choices ; ++i){
      if(startmatch(x, choices_str[i], false)){
        ADD_MATCH(MatchInfo(0, nx_wide))
      }
    }
  }
  
  if(n < 2){
    choices.select(all_id);
    return StringMatch(query, choices, all_match_info, all_id, msg);
  }
  
  //
  // step 2: inclusion 
  //
  
  if(any_upper){
    for(uint i=0 ; i<n_choices ; ++i){
      if(!is_done[i] && inclusion_simple(x, choices_str[i], true, index)){
        ADD_MATCH(MatchInfo(index, nx_wide))
      }
    }
    
    for(uint i=0 ; i<n_choices ; ++i){
      if(!is_done[i] && inclusion_simple(x, choices_str[i], false, index)){
        ADD_MATCH(MatchInfo(index, nx_wide))
      }
    }
  } else {
    for(uint i=0 ; i<n_choices ; ++i){
      if(!is_done[i] && inclusion_simple(x, choices_str[i], false, index)){
        ADD_MATCH(MatchInfo(index, nx_wide))
      }
    }
  }
  
  //
  // step 3: inclusion + misspells 
  //
  
  if(n >= 4){
    for(uint i=0 ; i<n_choices ; ++i){
      if(!is_done[i] && inclusion_misspell(x, choices_str[i], index)){
        ADD_MATCH(MatchInfo(index, nx_wide))
      }
    }
  }
  
  //
  // step 4: word inclusion 
  //
  
  // we go to the first space
  uint i = 0;
  while(i < n && x[i] != ' '){
    ++i;
  }
  
  // if no space, we return
  if(i == n){
    choices.select(all_id);
    return StringMatch(query, choices, all_match_info, all_id, msg);
  }
  
  vector<string> all_words;
  all_words.push_back(x.substr(0, i));
  while(i < n){
    string tmp;
    while(i < n && x[i] == ' '){
      ++i;
    }
    
    while(i < n && x[i] != ' '){
      tmp += x[i++];
    }
    
    if(!tmp.empty()){
      all_words.push_back(tmp);
    }
  }
  
  bool any_large_word = false;
  const uint n_words = all_words.size();
  vector<uint> word_sizes;
  for(auto &w : all_words){
    uint n_letters = utf8::count_wide_chars(w);
    word_sizes.push_back(n_letters);
    any_large_word = any_large_word || is_largeq_than_4(n_letters);
  }
  
  // inclusion without misspell
  for(uint i=0 ; i<n_choices ; ++i){
    if(is_done[i]){
      continue;
    }
    
    bool ok = true;
    MatchInfo current_match;
    for(uint idx=0 ; idx<n_words ; ++idx){
      if(inclusion_simple(all_words[idx], choices_str[i], false, index)){
        current_match.add(index, word_sizes[idx]);
      } else {
        ok = false;
        break;
      }
    }
    
    if(ok){
      ADD_MATCH(current_match)
    }
  }
  
  // inclusion with misspell
  if(any_large_word){
    for(uint i=0 ; i<n_choices ; ++i){
      if(is_done[i]){
        continue;
      }
      
      bool ok = true;
      MatchInfo current_match;
      for(uint idx=0 ; idx<n_words ; ++idx){
        if(is_largeq_than_4(word_sizes[idx])){
          // misspell
          if(inclusion_misspell(all_words[idx], choices_str[i], index)){
            current_match.add(index, word_sizes[idx]);
          } else {
            ok = false;
            break;
          } 
        } else {
          // no misspell
          if(inclusion_simple(all_words[idx], choices_str[i], false, index)){
            current_match.add(index, word_sizes[idx]);
          } else {
            ok = false;
            break;
          }
        }
      }
      
      if(ok){
        ADD_MATCH(current_match)
      }
    }
  }
  
  choices.select(all_id);
  return StringMatch(query, choices, all_match_info, all_id, msg);
  
}

ParenMatcher::ParenMatcher(string line, uint cursor){
  const uint &n = line.size();
  
  if(n < 5){
    return;
  }
  
  //
  // step 1: we check whether the cursor is adjacent to a paren 
  //
  
  // priority to the right
  
  if(cursor < n && is_paren(line[cursor])){
    if(is_closing_paren(line[cursor])){
      i_end = cursor;
    } else {
      i_start = cursor;
    }
  } else if(cursor > 0 && is_paren(line[cursor - 1])){
    if(is_closing_paren(line[cursor - 1])){
      i_end = cursor - 1;
    } else {
      i_start = cursor - 1;
    }
  } else {
    return;
  }
  
  //
  // step 2: we find the match
  //
  
  const bool we_seek_a_closing_paren = i_end == UNSET::UINT;
  
  const char open_paren = we_seek_a_closing_paren ? line[i_start] : pair_quote_paren_char(line[i_end]);
  const char closing_paren = pair_quote_paren_char(open_paren);
  vector<uint> open_stack;
  
  uint i = 0;
  while(i < n){
    const char c = line[i];
    
    if(is_quote(c)){
      const char quote = c;
      ++i;
      
      while( i < n && !( line[i] == quote && !is_escaped(line, i) ) ){
        ++i;
      }
      
    } else if(c == open_paren){
      open_stack.push_back(i);
      
    } else if(c == closing_paren){
      if(!open_stack.empty()){
        uint index_open = open_stack.back();
        
        if(we_seek_a_closing_paren){
          if(index_open == i_start){
            i_end = i;
            pair_found = true;
            return;
          }
        } else {
          if(i == i_end){
            i_start = index_open;
            pair_found = true;
            return;
          }
        }
        
        open_stack.pop_back();
      }
    }
    
    ++i;
  }
  
}


vector<uint> contextual_selection(const string &x, const uint cursor_left, 
                                  const uint cursor_right, const bool has_moved){
  /* Algorithm
  * 
  * objective: to select a wider selection than the current one
  * def `selection`: all elements within the first paren/quote superset
  * def `container`: a paren/quote
  * 
  * if we cannot find a selection => select all
  * 
  * RETURN: vector of length 2 correpsonding to starting/end of cursor positions
  * 
  * */
  
  const uint n = x.size();
  
  // special cases
  if(cursor_left == 0 || cursor_right >= n){
    return vector<uint>{0, n};
  }
  
  vector<char> all_containers;
  vector<char> all_positions;
  uint i = 0;
  while(i < cursor_left){
    const char c = x[i];
    
    if(is_quote(c)){
      uint i_bak = i;
      const char quote = c;
      
      ++i;
      while( i < cursor_left && !( x[i] == quote && !is_escaped(x, i) ) ){
        ++i;
      }
      
      if(i >= cursor_left){
        // we save the quote and quit the loop
        all_containers.push_back(quote);
        all_positions.push_back(i_bak + 1);
        break;
      }
      
    } else if(is_opening_paren(c)){
      all_containers.push_back(c);
      all_positions.push_back(i + 1);
      
    } else if(is_closing_paren(c)){
      if(all_containers.empty()){
        // multi line or syntax error, we ignore
        
      } else if(c == pair_quote_paren_char(all_containers.back())){
        all_containers.pop_back();
        all_positions.pop_back();
        
      } else {
        // multi line or syntax error, we ignore
      }
    }
    
    ++i;
  }
  
  if(all_containers.empty()){
    // no containers => select all
    return vector<uint>{0, n};
  }
  
  bool in_quote = is_quote(all_containers.back());
  uint i_left = all_positions.back();
  const char container_left = all_containers.back();
  all_containers.clear();
  
  //
  // branch 1: quote 
  //
  
  // quote is the easy case
  if(in_quote){
    const char &quote = container_left;
    while(i < n && !( x[i] == quote && !is_escaped(x, i) )){
      ++i;
    }
    
    if(i < cursor_right){
      // the quote ended before the cursor => select all
      return vector<uint>{0, n};
    }
    
    // the !has_moved should be useless here since a quote cannot
    // contain anything by definition
    if(!has_moved && i_left == cursor_left && i == cursor_right){
      return contextual_selection(x, cursor_left - 1, cursor_right + 1, true);
    }
    
    return vector<uint>{i_left, i};
  }
  
  //
  // branch 2 : paren
  //
  
  
  // we find the index of the closing element
  uint i_right = 0;
  while(i < n){
    const char c = x[i];
    if(is_quote(c)){
      const char &quote = c;
      ++i;
      while( i < n && !( x[i] == quote && !is_escaped(x, i) ) ){
        ++i;
      }
      
    } else if(is_opening_paren(c)){
      all_containers.push_back(c);
      
    } else if(is_closing_paren(c)){
      
      if(all_containers.empty()){
        if(c == pair_quote_paren_char(container_left)){
          // we're done!
          i_right = i;
          break;
          
        } else {
          // multi line or syntax error, we ignore
        }
      } else if(c == pair_quote_paren_char(all_containers.back())){
        all_containers.pop_back();
      } else {
        // multi line or syntax error, we ignore
      }
    }
    
    ++i;
  }
  
  if(i_right < cursor_right){
    // right cursor is past the closing paren
    return vector<uint>{0, n};
  }
  
  if(!has_moved && i_left == cursor_left && i_right == cursor_right){
    return contextual_selection(x, cursor_left - 1, cursor_right + 1, true);
  }
  
  return vector<uint>{i_left, i_right};
}

vector<uint> contextual_selection(const string_utf8 &x, const uint cursor_left, const uint cursor_right){
  const uint n = x.size();
  if(cursor_left == 0 || cursor_right >= x.size()){
    return vector<uint>{0, n};
  }
  
  // we transform to narrow
  const string &str = x.str();
  const uint narrow_cleft = x.wide_to_narrow_index(cursor_left);
  const uint narrow_cright = x.wide_to_narrow_index(cursor_right);
  
  vector<uint> res = contextual_selection(str, narrow_cleft, narrow_cright);
  
  // and we rewiden
  const uint i_left = x.narrow_to_wide_index(res[0]);
  const uint i_right = x.narrow_to_wide_index(res[1]);
  
  return vector<uint>{i_left, i_right};
}


vector<string> align_on_columns(const vector<string> &x, const size_t width, 
                                const string sep, bool ignore_vts){
  
  
  vector<string> res;
  
  size_t n = x.size();
  size_t n_sep = sep.size();
  
  vector<size_t> all_string_sizes(n);
  double avg_string_size = 0;
  for(size_t i = 0 ; i < n ; ++i){
    
    const size_t w = ignore_vts ? size_no_vts(x[i]) : x[i].size();
    
    all_string_sizes[i] = w;
    avg_string_size += w;
  }
  
  avg_string_size /= n;
  
  //
  // Algorithm 1 
  //
  
  // we first find a tentative nber of columns based on the first row
  uint n_cols = 0;
  uint current_width = 0;
  while(n_cols < n){
    const uint w = all_string_sizes[n_cols] + (n_cols == 0 ? 0 : n_sep); 
    if(current_width + w < width){
      current_width += w;
      ++n_cols;
    } else {
      break;
    }
  }
  
  vector<size_t> all_col_width;
  vector<double> all_col_avg_width;
  while(n_cols > 0){
  
    all_col_width.clear();
    all_col_avg_width.clear();
    for(size_t i = 0 ; i < n_cols ; ++i){
      all_col_width.push_back(0);
      all_col_avg_width.push_back(0);
    }
    
    uint line_width = 0;
    bool reduce_n_cols = false;
    for(size_t i = 0 ; i < n ; ++i){
      const uint w = all_string_sizes[i]; 
      const uint index_col = i % n_cols;
      
      if(w > all_col_width[index_col]){
        all_col_width[index_col] = w;
      }
      
      all_col_avg_width[index_col] += w;
      
      if(index_col == 0){
        line_width = all_col_width[index_col];
      } else {
        line_width += all_col_width[index_col] + n_sep;
      }
      
      if(i == n - 1){
        // we flush
        for(size_t j = index_col + 1 ; j < all_col_width.size() ; ++j){
          line_width += all_col_width[j] + n_sep;
        }
      }
      
      if(line_width > width){
        reduce_n_cols = true;
        break;
      }
      
    }
    
    if(reduce_n_cols){
      --n_cols;
    } else {
      // we're good
      break;
    }
    
  }
  
  
  //
  // Are the sizes "correct" ?
  //
  
  // we flag the values that are too large 
  
  bool any_too_large = false;
  
  if(n_cols == 0){
    any_too_large = true;
    
  } else {
    
    // the column avgs
    double n_lines = static_cast<double>(n) / n_cols;
    for(auto &val : all_col_avg_width){
      val /= n_lines;
    }
    
    for(size_t i = 0 ; i < n ; ++i){
      const uint w = all_string_sizes[i]; 
      const uint index_col = i % n_cols;
      
      if(w > 25 && w > 2.5 * all_col_avg_width[index_col]){
        any_too_large = true;
        break;
      }
      
    }
    
  }
  
  
  if(!any_too_large){
    // OK, we're done
    
    string line;
    for(size_t i = 0 ; i < n ; ++i){
      const uint index_col = i % n_cols;
      int w = all_col_width[index_col];
      
      const string el_padded = right_fill_with_space(x[i], w, ignore_vts);
      
      if(index_col == 0){
        if(i > 0){
          res.push_back(line);
        }
        line = el_padded;
      } else {
        line += sep + el_padded;
      }
      
    }
    
    res.push_back(line);
    
    return res;
  }
  
  //
  // Algorithm 2 
  //
  
  // the previous algorithm was too complicated => we apply a rule of thumb
  // we fix the column size and see what happens
  
  n_cols = (width + n_sep) / (avg_string_size + n_sep);
  uint col_width = (width - (n_cols - 1) * n_sep) / n_cols;
  
  int index_col = 0;
  string line;
  for(size_t i = 0 ; i < n ; ++i){
    
    index_col = index_col % n_cols;
    
    const string &el = x[i];
    const uint w = all_string_sizes[i];
    
    if(index_col == 0 && i > 0){
        res.push_back(line);
    }
    
    if(w <= col_width){
      // ok it fits
      const string el_padded = right_fill_with_space(el, col_width, ignore_vts);
      if(index_col == 0){
        line = el_padded;
      } else {
        line += sep + el_padded;
      }
      
      ++index_col;
      
    } else {
      // it does not fit a single cell
      
      int n_cells = std::ceil((static_cast<double>(w) + n_sep) / (col_width + n_sep));
      
      if(index_col + n_cells - 1 < static_cast<int>(n_cols)){
        // it fits in the current line
        // ex:
        // width = 100
        // col_width = 20, w = 55, n_cols = 5, index_col = 1
        // n_cells => 3
        // index_col + n_cells - 1 => 3
        
        const string el_padded = right_fill_with_space(el, col_width * n_cells + n_sep * (n_cells - 1), ignore_vts);
        if(index_col == 0){
          line = el_padded;
        } else {
          line += sep + el_padded;
        }
        
        index_col += n_cells;
        
      } else {
        // the content does not fit the cells,
        // we go to the next line
        
        if(index_col > 0){
          res.push_back(line);
        }
        index_col = 0;
        
        if(n_cells < static_cast<int>(n_cols)){
          // other content can fit next to it
          line = right_fill_with_space(el, n_cells * col_width + (n_cells - 1) * n_sep, ignore_vts);
          index_col += n_cells;
          
        } else {
          // it exceeds the total line width
          if(width > 9){
            if(el[0] == 27 && el[1] == '['){
              line = el.substr(0, width - 8) + "\033[0m [trunc]";
            } else {
              line = el.substr(0, width - 8) + " [trunc]";
            }
          } else {
            line = el.substr(0, width);
          }
        }
      }
    }
  }
  
  res.push_back(line);
  
  
  return res;
}


string str_replace(const string &x, const string &from, const string &to){
  
  string res;
  const int n = x.size();
  int i = 0;
  
  const int n_from = from.size();
  
  while(i < n){
    
    if(i < n - n_from + 1 && x[i] == from[0]){
      bool found = true;
      for(int j = 1 ; j < n_from ; ++j){
        if(x[i + j] != from[j]){
          found = false;
          break;
        }
      }
      
      if(found){
        res += to;
        i += n_from;
      } else {
        res += x[i++];
      }
      
    } else {
      res += x[i++];
    }
  
  }
  
  return res;
}

string enumerate(const vector<string> &x, const EnumOpts opts){
  
  string res;
  
  const bool is_quote = !opts.has_quote();
  const string q = opts.get_quote();
  const size_t n = x.size();
  for(size_t i = 0 ; i < n ; ++i){
    if(is_quote){
      res += q + x[i] + q + (i == n - 1 ? "" : ", ");
    }
    
    if(n >= 2 && i == n - 2){
      res += (opts.is_or() ? "or " : "and ");
    }
    
  }
  
  return res;
}

uint64_t hash_string_vector(const vector<string> &x){
  // slow hashing algo for string vectors
  // if hashing becomes critical, => need to update this function
  // In my use case, the strings of x do not contain \n
  // 
  
  if(x.empty()){
    return UNSET::UINT;
  }
  
  string big_str = x[0];
  const uint n = x.size();
  for(uint i = 1 ; i < n ; ++i){
    big_str += big_str + "\n" + x[i];
  }
  
  return std::hash<string>{}(big_str);
}


vector<string> str_split_at_width(const string &x, const int width){
  
  vector<string> res;
  if(x.empty()){
    return res;
  }
  
  int i = 0;
  const int n = x.size();
  
  while(i < n){
    
    string line;
    int w = 0;
    while(w < width && i < n){
      const char &c = x[i];
      if(c == 27){
        // VTS
        line += x[i++];
        while(i < n && !is_ascii_letter(x[i])){
          line += x[i++];
        }
        
        if(i < n){
          line += x[i++];
        }
        
        if(w == 0 && res.size() > 0){
          // special case: string ends with a VTS
          res.back() += line;
          line.clear();
        }
        
      } else if(utf8::is_starting_byte(c)){
        ++w;
        line += x[i++];
        while(i < n && utf8::is_continuation(x[i])){
          line += x[i++];
        }
      } else {
        ++w;
        line += x[i++];
      }
    }
    
    if(!line.empty()){
      res.push_back(line);
    }
    
  }
  
  return res;
}

string shorten(const string &x, const uint nmax, const string ellipsis){
  // side = either right or center
  
  if(x.size() <= nmax){
    return x;
  }
  
  const uint n_wide = size_no_vts(x);
  
  if(n_wide <= nmax){
    return x;
  }
  
  string res;
  const uint n_narrow = x.size();
  const uint n_ellipsis = size_no_vts(ellipsis);
  
  size_t clean_size = 0;
  uint i = 0;
  while(i < n_narrow && clean_size < nmax - n_ellipsis){
    
    if(x[i] == 27){
      ++i;
      while(i < n_narrow && !is_ascii_letter(x[i])){
        ++i;
      }
      ++i;
    } else {
      ++clean_size;
      utf8::move_i_to_start_of_next_utf8_char(x, i);
    }
    
  }
  
  res = x.substr(0, i) + ellipsis;
  
  return res;
}

vector<string> shorten(const vector<string> &all_x, const uint nmax, const string ellipsis){
  const uint n = all_x.size();
  vector<string> res(n);
  for(size_t i = 0 ; i < n ; ++i){
    res[i] = shorten(all_x[i], nmax, ellipsis);
  }
  
  return res;
}

void shorten_inplace(vector<string> &all_x, const uint nmax, const string ellipsis){
  for(auto &x : all_x){
    x = shorten(x, nmax, ellipsis);
  }
}

string_utf8 shorten(const string_utf8 &x, const uint nmax, const string ellipsis){
  return shorten(x.str(), nmax, ellipsis);
}

vector<string_utf8> shorten(const vector<string_utf8> &all_x, const uint nmax, const string ellipsis){
  const uint n = all_x.size();
  vector<string_utf8> res(n);
  for(size_t i = 0 ; i < n ; ++i){
    res[i] = shorten(all_x[i].str(), nmax, ellipsis);
  }
  
  return res;
}

void shorten_inplace(vector<string_utf8> &all_x, const uint nmax, const string ellipsis){
  for(auto &x : all_x){
    x = shorten(x.str(), nmax, ellipsis);
  }
}


} // namespace stringtools


