    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//



#include "console.hpp"
#include <algorithm>


static bool _debug_char = false;

//
// Utilities ------------------------------------------------------------------- 
//

namespace {

const string AUTOMATCH_FULL_PAREN = " +-*/=<>|&~,;#$[]{}";

string clear_underlines(const string &x){
  string res;
  const size_t n = x.size();
  size_t i = 0;
  while(i < n){
    
    if(i + 3 < n && x[i] == 27 && x[i + 1] == '['){
      
      if(x[i + 2] == '4' && x[i + 3] == 'm'){
        i += 4;
      } else if(i + 4 < n && x[i + 2] == '2' && x[i + 3] == '4' && x[i + 4] == 'm'){
        i += 5;
      } else {
        res += x[i++];
      }
      
    } else {
      res += x[i++];
    }
  }
  
  return res;
}

string format_time(std::chrono::microseconds dur_us){
  
  size_t n_us = dur_us.count();
  const size_t n_ms = (n_us / 1000) % 1000;
  const size_t n_sec = (n_us / 1000000) % 60;
  const size_t n_min = n_us / 60000000;
  
  n_us = n_us % 1000;
  
  string res;
  
  bool exists = n_min > 0;
  
  // min
  if(exists){
    if(n_min < 10){
      res = " " + std::to_string(n_min) + "m ";
    } else {
      res = std::to_string(n_min) + "m ";
    }
  } else {
    res = "    ";
  }
  
  // seconds
  if(n_sec > 0){
    if(n_sec < 10){
      res += (exists ? "0" : " ") + std::to_string(n_sec) + "s ";
    } else {
      res += std::to_string(n_sec) + "s ";
    }
    
    exists = true;
  } else {
    res += exists ? "00s " : "    ";
  }
  
  // milliseconds
  if(n_ms > 0){
    if(n_ms < 10){
      res += (exists ? "00" : "  ") + std::to_string(n_ms) + "ms ";
    } else if(n_ms < 100) {
      res += (exists ? "0" : " ") + std::to_string(n_ms) + "ms ";
    } else {
      res += std::to_string(n_ms) + "ms ";
    }
    
    exists = true;
  } else {
    res += exists ? "000ms " : "      ";
  }
  
  // microseconds
  if(n_us > 0){
    if(n_us < 10){
      res += (exists ? "00" : "  ") + std::to_string(n_us) + "us ";
    } else if(n_us < 100) {
      res += (exists ? "0" : " ") + std::to_string(n_us) + "us ";
    } else {
      res += std::to_string(n_us) + "us ";
    }
    
  } else {
    res += exists ? "000us " : "      ";
  }
  
  return res;
}

} // end anonymous namespace

//
// cursor management ----------------------------------------------------------- 
//

inline uint ConsoleCommand::get_line_max_width(uint i){
  return win_width - prompt_size(i == 0);
}

inline uint ConsoleCommand::get_line_height(uint i){
  if(i >= all_lines.size()){
    util::error_msg("Internal error: current line height index is invalid.\n",
                    "i = ", i, "\n",
                    "all_lines.size() = ", all_lines.size());
    return 1;
  }
  
  uint n = all_lines[i].size();
  
  if(n == 0){
    return 1;
  }
  
  const uint &w = get_line_max_width(i);
  return n / w + (n % w > 0);
}

inline uint ConsoleCommand::get_current_line_max_width(){
  return get_line_max_width(cursor_str_y == 0);
}

inline uint ConsoleCommand::get_current_line_height(){
  return get_line_height(cursor_str_y);
}

inline uint ConsoleCommand::get_total_command_height(){
  
  uint total_height = 0;
  for(size_t i = 0 ; i < all_lines.size() ; ++i){
    total_height += get_line_height(i);
  }

  return total_height;
}

inline uint ConsoleCommand::get_max_term_y(){
  return get_total_command_height() - 1;
}

inline uint ConsoleCommand::get_current_line_height_index(){
  
  uint previous_height = 0;
  for(size_t i = 0 ; i < cursor_str_y ; ++i){
    previous_height += get_line_height(i);
  }
  
  return cursor_term_y - previous_height;
}

inline uint ConsoleCommand::get_current_max_term_x(){
  
  const uint h = get_current_line_height();
  const uint psize = current_prompt_size();
  
  if(h == 1){
    return pline->size() + psize;
  }
  
  const uint height_index = get_current_line_height_index();
  const size_t w = get_current_line_max_width();
  
  if(height_index < h - 1){
    return w + psize;
  }
  
  const size_t n = pline->size();
  const size_t max_term_x = n % w + (n > 0 && n % w == 0) * w + current_prompt_size();
  
  return max_term_x;
}

inline bool ConsoleCommand::is_str_x_ambiguous(uint str_x){
  // position at the edge of the window
  
  if(util::is_unset(str_x)){
    str_x = cursor_str_x;
  }
  
  const uint h = get_current_line_height();
  if(h == 1){
    return false;
  }
  
  const uint w = get_current_line_max_width();
  const uint &x = str_x;
  
  return x > 0 && x % w == 0;
}

void ConsoleCommand::update_term_cursors(bool move){
  // update of the term cursors based on the value of the str cursors
  
  if(cursor_str_x > all_lines[cursor_str_y].size()){
    cursor_str_x = all_lines[cursor_str_y].size();
  }
  
  cursor_term_x_old = cursor_term_x;
  cursor_term_y_old = cursor_term_y;
  
  uint y_offset = 0;
  for(size_t i = 0 ; i < cursor_str_y ; ++i){
    y_offset += get_line_height(i);
  }
  
  const uint w = get_current_line_max_width();
  const uint &x = cursor_str_x;
  
  // ambiguous means at the edge
  if(str_x_position_when_ambiguous == STR_X_POSITION::TOP_RIGHT || cursor_str_x == pline->size()){
    cursor_term_x = x % w + (x > 0 && x % w == 0) * w + current_prompt_size();
    cursor_term_y = x / w - (x % w == 0 && x > 0) + y_offset;
    
  } else {
    cursor_term_x = x % w + current_prompt_size();
    cursor_term_y = x / w + y_offset;
    
  }
  
  if(!move){
    return;
  }
  
  //
  // moving
  //
  
  std::cout << VTS::CURSOR_HIDE;
  
  if(cursor_term_y < cursor_term_y_old){
    // we move y up
    std::cout << VTS::cursor_up(cursor_term_y_old - cursor_term_y);
    
  } else if(cursor_term_y > cursor_term_y_old){
    // we move y down
    size_t diff = cursor_term_y - cursor_term_y_old;
    // std::cout << VTS::cursor_down(cursor_term_y - cursor_term_y_old);
    for(size_t i = 0 ; i < diff ; ++i){
      std::cout << "\n";
    }
    
  }
  
  std::cout << VTS::cursor_move_at_x(cursor_term_x);
  
  std::cout << VTS::CURSOR_REVEAL;
  
}

void ConsoleCommand::cursors_increment_str_x(){
  // we update the terminal cursors and we move them
  
  const uint n = pline->size();
  if(cursor_str_x >= n){
    return;
  }
  
  ++cursor_str_x;
  update_term_cursors();
  
}

void ConsoleCommand::cursors_decrement_str_x(){
  
  if(cursor_str_x == 0){
    return;
  }
  
  --cursor_str_x;
  update_term_cursors();
  
}

void ConsoleCommand::cursors_increment_str_y(){
  // we update the terminal cursors and we move them
  
  if(cursor_str_y + 1 == all_lines.size()){
    return;
  }
  
  ++cursor_str_y;
  pline = &all_lines[cursor_str_y];
  if(cursor_str_x > pline->size()){
    cursor_str_x = pline->size();
  }
  
  update_term_cursors();
  
}

void ConsoleCommand::cursors_decrement_str_y(){
  
  if(cursor_str_y == 0){
    return;
  }
  
  --cursor_str_y;
  
  pline = &all_lines[cursor_str_y];
  if(cursor_str_x > pline->size()){
    cursor_str_x = pline->size();
  }
  
  update_term_cursors();
  
}

void ConsoleCommand::update_str_cursors(){
  // term_y has moved: we update
  // str_x str_y term_x
  
  // finding out str_y
  
  uint nlines_previous = 0;
  uint total_lines = 0;
  uint h = get_line_height(0);
  
  size_t str_y = 0;
  for( ; str_y < all_lines.size() ; ++str_y){
    h = get_line_height(0);
    total_lines += h;
    if(total_lines >= cursor_term_y + 1){
      break;
    }
    
    nlines_previous += h;
  }
  
  cursor_str_y = str_y;
  pline = &all_lines[cursor_str_y];
  
  //
  // now cursor_str_x
  //
  
  const uint w = get_current_line_max_width();
  const uint prompt_width = current_prompt_size();
  const uint tx = cursor_term_x - prompt_width;
  const uint ty = cursor_term_y - nlines_previous;
  
  uint str_x = tx % w + w * (ty + (tx == w));
  
  if(str_x > pline->size()){
    // we need to shift term_x to the left
    uint diff = pline->size() - str_x;
    std::cout << VTS::cursor_left(diff);
    cursor_term_x -= diff;
    cursor_str_x = pline->size();
    
  } else {
    cursor_str_x = str_x;
  }
  
}

void ConsoleCommand::cursors_increment_term_x(){
  
  if(cursor_str_x == pline->size()){
    return;
  }
  
  if(get_current_line_height() > 1){
    const uint w = get_current_line_max_width();
    const uint prompt_width = current_prompt_size();
    const uint tx = cursor_term_x - prompt_width;
    
    if(tx == w){
      // special case
      // > hello bon|      > hello bon
      //   jour              |jour 
      // it corresponds to the same str_x but different term_x/term_y
      
      cursor_term_x = prompt_width;
      std::cout << VTS::CURSOR_DOWN << VTS::cursor_move_at_x(cursor_term_x);
      return;
    }
    
  }
  
  // we stay on the same line
  ++cursor_term_x;
  ++cursor_str_x;
  std::cout << VTS::CURSOR_RIGHT;
  
}

void ConsoleCommand::cursors_decrement_term_x(){
  
  if(cursor_str_x == 0){
    return;
  }
  
  if(get_current_line_height() > 1){
    const uint w = get_current_line_max_width();
    const uint prompt_width = current_prompt_size();
    const uint tx = cursor_term_x - prompt_width;
    
    if(tx == 0){
      // special case
      // > hello bon       > hello bon|
      //   |jour             jour 
      // it corresponds to the same str_x but different term_x/term_y
      
      cursor_term_x = prompt_width + w;
      std::cout << VTS::CURSOR_UP << VTS::cursor_move_at_x(cursor_term_x);
      return;
    }
    
  }
  
  // we stay on the same line
  --cursor_str_x;
  --cursor_term_x;
  std::cout << VTS::CURSOR_LEFT;
  
}

void ConsoleCommand::cursors_increment_term_y(){
  
  if(cursors_is_term_y_bottom()){
    return;
  }
  
  ++cursor_term_y;
  std::cout << VTS::CURSOR_DOWN;
  update_str_cursors();
  
}

void ConsoleCommand::cursors_decrement_term_y(){
  
  if(cursor_term_y == 0){
    return;
  }
  
  --cursor_term_y;
  std::cout << VTS::CURSOR_UP;
  update_str_cursors();
  
}

void ConsoleCommand::cursors_set_str_x(uint pos){
  
  if(pos > pline->size()){
    util::error_msg("Internal error in cursors_set_str_x():\n",
                    "new pos = ", pos, "\n",
                    "pline->size() = ", pline->size());
    return;
  }
  
  cursor_str_x = pos;
  update_term_cursors();
  
}

void ConsoleCommand::cursors_set_str_y(uint pos){
  
  if(pos >= all_lines.size()){
    util::error_msg("Internal error in cursors_set_str_y():\n",
                    "new pos = ", pos, "\n",
                    "all_lines.size() = ", all_lines.size());
    return;
  }
  
  cursor_str_y = pos;
  
  pline = &all_lines[pos];
  cursor_str_x = cursor_term_x;
  if(cursor_str_x > pline->size()){
    cursor_str_x = pline->size();
  }
  
  update_term_cursors();
  
}

uint ConsoleCommand::get_str_x_when_term_x_leftmost(){
  // we move on the current line, so that's easy
  
  uint res = cursor_str_x;
  
  const uint psize = prompt_size(cursor_str_y == 0);
  
  if(cursor_term_x <= psize){
    return res;
  }
  
  uint diff = cursor_term_x - psize;
  if(diff > 0){
    res -= diff;
  }
  
  return res;
}


uint ConsoleCommand::get_str_x_when_term_x_rightmost(){
  
  uint res = cursor_str_x;
  
  if(cursor_term_x == win_width || cursor_str_x == pline->size()){
    return res;
  }
  
  const uint h = get_current_line_height();
  
  if(h == 1){
    res = pline->size();
    return res;
  }
  
  uint line_index = get_current_line_height_index();
  
  uint diff;
  if(line_index < h - 1){
    // we're not at the endline
    // => we go at the win_width
    diff = win_width - cursor_term_x;
    
  } else {
    //ending line => we go at the end of the line
    diff = pline->size() - cursor_str_x;
    
  }
  
  if(diff > 0){
    res += diff;
  }
  
  return res;
}

void ConsoleCommand::cursors_set_term_y_top(){
  
  if(cursor_term_y == 0){
    return;
  }
  
  std::cout << VTS::cursor_up(cursor_term_y);
  cursor_term_y = 0;
  cursor_str_y = 0;
  pline = &all_lines[cursor_str_y];
  
  // now x:
  const uint psize = prompt_size(cursor_str_y  == 0);
  if(pline->size() + psize < cursor_term_x){
    cursor_str_x = pline->size();
    cursor_term_x = pline->size() + psize;
    std::cout << VTS::cursor_move_at_x(cursor_term_x);
  } else {
    cursor_str_x = cursor_term_x - psize;
  }
  
}

void ConsoleCommand::cursors_set_term_y_bottom(){
  
  const uint n_lines = all_lines.size();
  uint total_term_lines = 0;
  for(size_t i = 0 ; i < n_lines ; ++i){
    total_term_lines += get_line_height(i);
  }
  
  if(cursor_term_y + 1 == total_term_lines){
    return;
  }
  
  // moving y
  std::cout << VTS::cursor_down(total_term_lines - cursor_term_y - 1);
  cursor_str_y = n_lines - 1;
  cursor_term_y = total_term_lines - 1;
  
  update_str_cursors();
  
}

bool ConsoleCommand::cursors_is_term_y_bottom(){
  
  if(cursor_str_y + 1 < all_lines.size()){
    return false;
  }
  
  const int h = get_current_line_height();
  if(h == 1){
    return true;
  }
  
  return cursor_term_y == get_max_term_y();
}

void ConsoleCommand::cursors_reset(){
  cursor_str_x = 0;
  cursor_str_y = 0;
  
  update_term_cursors();
  
}

void ConsoleCommand::cursors_save_and_set(uint new_x, uint new_y){
  cursor_str_x_bak = cursor_str_x;
  cursor_str_y_bak = cursor_str_y;
  
  cursor_str_x = new_x;
  cursor_str_y = new_y;
  
  update_term_cursors();
}

void ConsoleCommand::cursors_restore(){
  cursor_str_x = cursor_str_x_bak;
  cursor_str_y = cursor_str_y_bak;
  
  update_term_cursors();
}

void ConsoleCommand::n_up_total_height_when_win_resizing(uint &n_up, uint &total_height, uint new_width){
  
  const uint &old_width = win_width;
  
  //
  // step 1: getting the total number of lines to delete (ie new total command height after resizing) 
  //
  
  
  uint total_height_old = 0;
  uint previous_height = 0;
  uint total_height_resize = 0;
  for(uint str_y = 0 ; str_y < all_lines.size() ; ++str_y){
    
    const uint n = all_lines[str_y].size();
    uint old_height = get_line_height(str_y);
    const uint &w_old = get_line_max_width(str_y);
    uint rest = n % w_old;
    if(rest == 0 && n > 0){
      rest = w_old;
    }
    rest += prompt_size(str_y == 0);
    
    uint height_after_resize = old_height + (old_width > new_width) * (old_height - 1) + (rest > new_width);
    
    total_height_old += old_height;
    total_height_resize += height_after_resize;
    
    if(str_y + 1 == cursor_str_y){
      previous_height = total_height_resize;
    }
    
  }
  
  total_height = total_height_resize;
  
  //
  // step 2: getting the number of moves up to make 
  //
  
  uint index = get_current_line_height_index();
  n_up = previous_height + index + index * (old_width > new_width) + (cursor_term_x > new_width);
  
}


//
// ConsoleCommand --------------------------------------------------------------
//


// static values
bool ConsoleCommand::is_initialized = false;
HANDLE ConsoleCommand::handle_in = nullptr;
HANDLE ConsoleCommand::handle_out = nullptr;
uint ConsoleCommand::old_console_CP = 0;
uint ConsoleCommand::old_console_CP_output = 0;
DWORD ConsoleCommand::old_consModeIn = 0;
DWORD ConsoleCommand::old_consModeOut = 0;


ConsoleCommand::ConsoleCommand(string program_name, const opts options){
  initialize(program_name, options);
}


void ConsoleCommand::initialize(string program_name, const opts options){
  
  if(is_initialized){
    throw util::bad_type("ConsoleCommand should not be constructed more than once.");
  }
  
  //
  // main objects 
  //
  
  prog_name = program_name;
  
  // language server
  plgsrv = pdefault_lang.get();
  
  // console autocomplete
  pautocomp->set_console(this);
  
  // history
  std::shared_ptr<ConsoleHistory> phist_main = std::make_shared<ConsoleHistory>(this, program_name);
  phist_main->set_as_main_history();
  
  // we intiialize the default histories
  std::shared_ptr<ConsoleHistory> phist_rest = std::make_shared<ConsoleHistory>(this);
  
  hist_list["main"] = phist_main;
  hist_list["rest"] = phist_rest;
  
  // we set the path of the cache
  CachedData::root_path = get_path_to_program_cache(program_name);
  
  //
  // the console 
  //
  
  
  // Windows
  handle_in = GetStdHandle(STD_INPUT_HANDLE);
  if(handle_in == INVALID_HANDLE_VALUE){
    console_error("cannot access the console");
  }
  
  // Save the current input mode, to be restored on exit.
  if(! GetConsoleMode(handle_in, &old_consModeIn) ){
    console_error("GetConsoleMode failed");
  }

  // Enable the window and mouse input events.
  // https://learn.microsoft.com/en-us/windows/console/setconsolemode
  // ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_VIRTUAL_TERMINAL_INPUT
  DWORD consMode = ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
  if(! SetConsoleMode(handle_in, consMode) ){
    console_error("SetConsoleMode failed");
  }
  
  // output
  handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
  if(handle_out == INVALID_HANDLE_VALUE){
    console_error("cannot access the console in output mode");
  }
  
  // Save the current output mode, to be restored on exit.
  if(! GetConsoleMode(handle_out, &old_consModeOut) ){
    console_error("GetConsoleMode failed for output");
  }
  
  // 2025/01/10: setting this mode leads to display problems, 
  //   try system2("typst", "compile ./autocomplete.R") 
  //    => it will be a mess because every new line starts at the end of the former
  //   => I need to find out the right options 
  // DWORD consModeOut = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_LVB_GRID_WORLDWIDE;
  // if(! SetConsoleMode(handle_out, consModeOut) ){
  //   console_error("SetConsoleMode failed for output");
  // }
  
  // NOTA: reset the console CP on exit
  old_console_CP = GetConsoleCP();
  old_console_CP_output = GetConsoleOutputCP();
  
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  
  CursorInfo curs_info(handle_out);
  win_width = curs_info.win_width;
  win_height = curs_info.win_height;
  
  //
  // special functions
  //
  
  all_special_functions = get_main_special_functions(this);
  
  //
  // options
  //
  
  program_opts.set_program(prog_name);
  program_opts.add_options_fmt(console_options_format);
  
  if(options.is_options_fmt()){
    program_opts.add_options_fmt(options.get_options_format());
  }
  
  program_opts.read_options();
  
  // setting the prompts from the options
  prompt_color = program_opts.get_option("prompt.color").get_color();
  prompt_cont = program_opts.get_option("prompt.continue").get_string();
  prompt_main = program_opts.get_option("prompt.main").get_string();
  
  //
  // pline 
  //
  
  // pline is a pointer!!!
  pline = &all_lines[0];
  
  
  // end
  is_initialized = true;
}

ConsoleCommand::~ConsoleCommand(){
  
  SetConsoleMode(handle_in, old_consModeIn);
  SetConsoleOutputCP(old_console_CP);
  SetConsoleCP(old_console_CP_output);
  
}

void ConsoleCommand::setup_lgsrv(LanguageServer* x){
  plgsrv = x;
}

void ConsoleCommand::console_error(const std::string msg){
  // print the msg in the stderr
  std::cerr << "Error: " << msg;
  // Restore input mode on exit
  SetConsoleMode(handle_in, old_consModeIn);
  ExitProcess(0);
}

void ConsoleCommand::join_lines(int side){
  // side == UP means we join the bottom line with the one above
  // side == DOWN means we join the top line with the line below
  //
  
  quit_autocomp();
  
  if(side == SIDE::UP){
    // we're on the bottom line, we go up
    cursors_decrement_str_y();
  }
  
  // cursor_str_y points to the line above
  
  cursors_set_str_x(0);
  
  uint n_lines_to_del = 0;
  for(size_t i = cursor_str_y ; i < all_lines.size() ; ++i){
    n_lines_to_del += get_line_height(i);
  }
  
  std::cout << VTS::delete_lines(n_lines_to_del);
  
  str::string_utf8 line_above = all_lines[cursor_str_y];
  const str::string_utf8 line_below = all_lines[cursor_str_y + 1];
  
  // we delete the existing data on the line below
  all_lines.erase(all_lines.begin() + cursor_str_y + 1);
  all_lines_fmt.erase(all_lines_fmt.begin() + cursor_str_y + 1);
  all_ending_quotes.erase(all_ending_quotes.begin() + cursor_str_y + 1);
  
  // we place the cursor at the right x-position
  cursors_set_str_x(line_above.size());
  line_above.push_back(line_below.str());
  all_lines[cursor_str_y] = line_above;
  pline = &all_lines[cursor_str_y];
  
  
  colorize();
  print_command(true);
}

void ConsoleCommand::print_command_and_delete_trailing_lines_if_needed(){
  
  if(get_current_line_height() < line_height_origin){
    // we need to delete the trailing lines before printing the command
    uint n_lines_to_del = line_height_origin;
    for(size_t i = cursor_str_y + 1 ; i < all_lines.size() ; ++i){
      n_lines_to_del += get_line_height(i);
    }
    
    uint str_x = cursor_str_x;
    cursors_set_str_x(0);
    std::cout << VTS::delete_lines(n_lines_to_del);
    cursors_set_str_x(str_x); 
    
    print_command(false, true, all_lines.size() - 1);
    
  } else {
    print_command();
    
  }
}

void ConsoleCommand::del(int side, bool is_ctrl){
  
  if(in_autocomp){
    if(side == SIDE::RIGHT){
      quit_autocomp();
    } else if(cursor_str_x == 0){
      quit_autocomp();
    }
  }
  
  char del_letter = '0';
  
  line_height_origin = get_current_line_height();
  
  if(selection.is_selection()){
    // deleteing means going left, hence the choice below
    str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
    
    // we delete the selection
    cursor_str_x = selection.range_start();
    pline->erase(cursor_str_x, selection.range_size());
    selection.remove_selection();
    print_command_and_delete_trailing_lines_if_needed();
    
  } else if(is_ctrl){
    // CTRL - DEL or CTRL - BS
    uint pos;
    if(is_special_command()){
      pos = pline->word_delete_default(cursor_str_x, side);
    } else {
      pos = pline->word_delete(cursor_str_x, side);
    }
    
    if(side == SIDE::LEFT){
      str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
      if(pos < cursor_str_x){
        pline->erase(pos, cursor_str_x - pos);
        cursor_str_x = pos;
        print_command_and_delete_trailing_lines_if_needed();
      }
    } else if(side == SIDE::RIGHT){
      // when we delete on the right, the cursor does not move
      // we respect the existing value of:
      // - str_x_position_when_ambiguous
      
      if(pos > cursor_str_x){
        pline->erase(cursor_str_x, pos - cursor_str_x);
        print_command_and_delete_trailing_lines_if_needed();
      }
    }
    
  } else {
    // no CTRL
    
    if(side == SIDE::LEFT){
      str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
      if(cursor_str_x == 0 && cursor_str_y > 0){
        // we delete the line and join it to the one above
        
        join_lines(SIDE::UP);
        
      } else if(cursor_str_x > 0){
        del_letter = (*pline)[cursor_str_x - 1];
        if(last_action == CON_ACTIONS::INSERT_PAIRED_PAREN){
          // adding a paren was an error, we remove the paired paren
          pline->erase(cursor_str_x - 1, 2);
        } else {
          pline->erase(cursor_str_x - 1, 1);
        }
        --cursor_str_x;
        print_command_and_delete_trailing_lines_if_needed();
        
      }
    } else if(side == SIDE::RIGHT){
      // when we delete on the right, the cursor does not move
      // we respect the existing value of:
      // - str_x_position_when_ambiguous
      
      if(cursor_str_x == pline->size() && cursor_str_y + 1 < all_lines.size()){
        
        join_lines(SIDE::DOWN);
        
      } else if(cursor_str_x < pline->size()){
        pline->erase(cursor_str_x, 1);
        print_command_and_delete_trailing_lines_if_needed();
      }
    }
  }
  
  last_action = CON_ACTIONS::DEL;
  
  if(in_autocomp){
    // we may rerun autocomplete if:
    // 1) the user uses simple backspaces that don't remove a control character
    // 2) the user uses ctrl+backspace + we're in a path
    if(side == SIDE::LEFT){
      if(!pautocomp->is_in_path()){
        if(!is_ctrl){
          if(del_letter != ' ' && !str::is_control_char(del_letter)){
            run_autocomp();
            return;
          }
        }
      } else {
        // in path
        run_autocomp();
        return;
      }
    }
    
    quit_autocomp();
  }
  
}

void ConsoleCommand::move_x(int side, bool is_ctrl, bool is_shift, bool is_alt){
  // any move on the x-axis
  
  if(in_autocomp){
    quit_autocomp();
    // if simple move => we just quit the autocomp
    if(!is_ctrl && side != SIDE::LEFTMOST && side != SIDE::RIGHTMOST){
      return;
    }
  }
  
  if(!is_shift && selection.is_selection()){
    
    if(side == SIDE::LEFT){
      cursor_str_x = selection.range_start();
    } else if(side == SIDE::RIGHT){
      cursor_str_x = selection.range_end();
    }
    
    clear_selection();
    
    if(side == SIDE::LEFT || side == SIDE::RIGHT){
      print_command();
      return;
    }
  }
  
  uint new_pos = cursor_str_x;
  
  if(side == SIDE::LEFTMOST){
    
    uint previous_pos = cursor_str_x_old;
    cursor_str_x_old = cursor_str_x;
    
    if(last_action == CON_ACTIONS::MOVE_X_LEFTMOST && cursor_str_x != previous_pos){
      new_pos = previous_pos;
      
      if(is_str_x_ambiguous(new_pos) && new_pos > cursor_str_x){
        str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
      }
      
    } else {
      new_pos = get_str_x_when_term_x_leftmost();
      str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
    }
    
    last_action = CON_ACTIONS::MOVE_X_LEFTMOST;
    
  } else if(side == SIDE::RIGHTMOST){
    
    uint previous_pos = cursor_str_x_old;
    cursor_str_x_old = cursor_str_x;
    
    if(last_action == CON_ACTIONS::MOVE_X_RIGHTMOST && cursor_str_x != previous_pos){
      new_pos = previous_pos;
      
      if(is_str_x_ambiguous(new_pos) && new_pos < cursor_str_x){
        str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
      }
      
    } else {
      new_pos = get_str_x_when_term_x_rightmost();
      str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
    }
    
    last_action = CON_ACTIONS::MOVE_X_RIGHTMOST;
    
  } else {
    last_action = CON_ACTIONS::MOVE_X;
    
    if(is_ctrl){
      // CTRL left/right
      
      if(is_alt){
        // jump to paren
        new_pos = str::go_to_next_inner_paren(pline->str(), cursor_str_x, side == SIDE::LEFT);
        
      } else {
        
        if(is_special_command()){
          new_pos = pline->word_jump_default(cursor_str_x, side);
        } else {
          new_pos = pline->word_jump(cursor_str_x, side);
        }
        
      }
      
    } else {
      // no CTRL
      if(side == SIDE::LEFT){
        if(cursor_str_x > 0){
          
          if(is_str_x_ambiguous() && str_x_position_when_ambiguous == STR_X_POSITION::BOTTOM_LEFT){
            str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
            
          } else {
            --new_pos;
            str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
          }
          
        }
      } else if(side == SIDE::RIGHT){
        if(cursor_str_x < pline->size()){
          
          if(is_str_x_ambiguous() && str_x_position_when_ambiguous == STR_X_POSITION::TOP_RIGHT){
            str_x_position_when_ambiguous = STR_X_POSITION::BOTTOM_LEFT;
            
          } else {
            ++new_pos;
            str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
          }
          
        }
      }
    }
  }
  
  if(new_pos != cursor_str_x){
    if(is_shift){
      selection.select_range(cursor_str_x, new_pos);
    }
    
    cursors_set_str_x(new_pos);
  }
  
  if(is_shift || is_str_x_ambiguous()){
    print_command();
    
  } else {
    pcmdstate->add_state();
    
  }
  
}

void ConsoleCommand::move_y(int side, bool is_shift){
  // NOTE: wrt selection, I need to define what is the expected behavior with y-axis movements
  // so far I'm not entirely for multi line selections
  
  // NOTA: ctrl + up/down is not captured by the win32 api
  
  clear_selection();
  
  //
  // branch 1: top/bottom navigation in autocomp
  //
  
  if(in_autocomp){
    if(is_shift){
      side = side == SIDE::UP ? SIDE::TOP : SIDE::BOTTOM;
    }
    pautocomp->move(side);
    return;
  }
  
  //
  // branch 2: history navigation 
  //  
  
  const bool was_navigation = last_action == CON_ACTIONS::NAVIGATE_HIST;
  
  bool navigate_hist = false;
  if(side == SIDE::UP && cursor_term_y == 0){
    navigate_hist = true;
  }
  
  if(side == SIDE::DOWN && (cursors_is_term_y_bottom() || was_navigation)){
    navigate_hist = true;
  }
  
  if(navigate_hist && is_shift){
    
    if(side == SIDE::UP && cursor_term_y == 0){
      // nothing
      return;
    }
    
    if(side == SIDE::DOWN && cursors_is_term_y_bottom()){
      // nothing
      return;
    }
    
    navigate_hist = false;
  }
  
  if(navigate_hist){
    
    bool any_update = false;
    uint n_lines_old = get_total_command_height();
    uint cursor_term_y_old = cursor_term_y;
    phist->navigate(side, any_update, !was_navigation);
    
    if(any_update){
      // we need to go back to the top line before deleting
      if(cursor_term_y_old > 0){
        cout << VTS::cursor_up(cursor_term_y_old);
      }
      cout << VTS::CURSOR_HIDE << VTS::delete_lines(n_lines_old);
      
      print_command(true);
    }
    
    last_action = CON_ACTIONS::NAVIGATE_HIST;
    return;
  }
  
  //
  // branch 3: regular up/down 
  //
  
  // Note that we end up here **only if** we are not topmost/bottommost
  // otherwise => hist navigation
  
  // we save the current line
  // NOTA: 2025-03-27, now we don't need to bc we manipulate direclty
  // pline which is a pointer to it
  // all_lines[cursor_str_y] = line; // <= this line would be invalid now, I keep it for info
  
  // hide cursor
  cout << VTS::CURSOR_HIDE;
  
  if(last_action != CON_ACTIONS::MOVE_Y){
    // we save the cursor position
    cursor_term_x_request = cursor_term_x;
  }
  
  if(side == SIDE::UP){
    if(is_shift){
      // move at the top
      cursors_set_term_y_top();
    } else {
      // move one line up
      cursors_decrement_term_y();
    }
    
  } else {
    if(is_shift){
      // we move at the bottom
      cursors_set_term_y_bottom();
    } else {
      // move one line down
      cursors_increment_term_y();
    }
    
  }
  
  pline = &all_lines[cursor_str_y];
  
  // we reposition the cursor
  const uint &n = pline->size();
  if(cursor_term_x != cursor_term_x_request){
    const size_t max_term_x = get_current_max_term_x();
    
    cursor_term_x = cursor_term_x_request > max_term_x ? max_term_x : cursor_term_x_request;
    std::cout << VTS::CURSOR_HIDE << VTS::cursor_move_at_x(cursor_term_x);
    update_str_cursors();
    
  } else if(cursor_str_x > n){
    cursor_str_x = n;
    update_term_cursors();
  }
  
  // reveal cursor
  cout << VTS::CURSOR_REVEAL;
  
  last_action = CON_ACTIONS::MOVE_Y;
}


void ConsoleCommand::add_char(const string &val, bool is_sequence){
  
  if(val.empty()){
    return;
  }
  
  // adding new characters moves the cursor to the right, hence the choice below
  str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
  
  last_action = CON_ACTIONS::INSERT;
  
  const char &c = val[0];
  bool is_single_char = val.size() == 1 && !is_sequence;
  
  if(in_autocomp){
    if(!is_single_char){
      quit_autocomp();
    } else {
      if(c != ' ' && !(pline->size() > 0 && cursor_str_x >= 1 && (*pline)[cursor_str_x - 1] == '.' && c == '/') && str::is_control_char(c)){
        // we exit autocomp
        quit_autocomp();
      } else {
        // we continue AC
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        print_command();
        run_autocomp();
        return;
      }
    }
  }
  
  if(selection.is_selection()){
    if(is_single_char && str::is_quote_paren_open(c)){
      // we "surround"
      pline->insert(selection.range_start(), val);
      // the cursor is moved to the end of the selection, right before the closing paren
      cursor_str_x = selection.range_end() + 1;
      pline->insert(cursor_str_x, str::pair_quote_paren(c));
      
    } else {
      // we delete the selection and insert the character
      cursor_str_x = selection.range_start();
      pline->erase(cursor_str_x, selection.range_size());
      pline->insert(cursor_str_x, val);
      cursor_str_x += str::utf8::count_wide_chars(val);
    }
    
    selection.remove_selection();
    
  } else if(is_single_char){
    uint narrow_cursor_str_x = pline->wide_to_narrow_index(cursor_str_x);
    
    if(c == '}' && cursor_str_y > 0 && str::no_nonspace_char(pline->str())){
      *pline = str::string_utf8{"}"};
      cursor_str_x = 1;
      
    } else if(str::is_closing_paren(c) && pline->size() > cursor_str_x && 
       (*pline)[cursor_str_x] == c && str::is_open_paren_left(pline->str(), narrow_cursor_str_x, c)){
      // val    = ')'
      // line   = hello(|)
      // cursor =       |
      //
      // outcome: only cursor moves, no character insertion
      
      ++cursor_str_x;
      
    } else if(str::is_quote_paren_open(c)){
      if(pline->size() > cursor_str_x && (*pline)[cursor_str_x] == c){
        // we don't insert anything, but move cursor as if
        ++cursor_str_x;
        
      } else if( str::is_quote(c) && ( str::is_escaped(pline->str(), narrow_cursor_str_x) || str::is_same_quote_open_before(pline->str(), narrow_cursor_str_x, c) || str::is_letter_adjacent(pline->str(), narrow_cursor_str_x) ) ){
        // no automatch => regular insertion
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        
      } else if(pline->size() > cursor_str_x && (str::is_ascii_letter((*pline)[cursor_str_x]) || (*pline)[cursor_str_x] == '.')){
        // we don't automatch when there's a letter right after
        // ex: fun|arg = 5
        // =>  fun(|arg = 5
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        
      } else if(str::is_paren(c) && pline->size() > cursor_str_x && c == str::pair_quote_paren_char((*pline)[cursor_str_x]) && !str::any_open_paren_before(pline->str(), c, cursor_str_x - 1)){
        // we don't automatch when there's the closing paren and it's appropriate not to automatch
        // bon(jour|) => bon(jour(|))
        // jour|) => jour(|)
        
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        
      } else if(str::is_paren(c) && pline->size() > cursor_str_x && (str::is_quote((*pline)[cursor_str_x]) || (*pline)[cursor_str_x] == '[')){
        // bon|"jour"
        // bon|[[jour]]
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        
      } else {
        pline->insert(cursor_str_x, val);
        ++cursor_str_x;
        pline->insert(cursor_str_x, str::pair_quote_paren(c));
        last_action = CON_ACTIONS::INSERT_PAIRED_PAREN;
      }
    } else {
      pline->insert(cursor_str_x, val);
      ++cursor_str_x;
    }
    
  } else {
    pline->insert(cursor_str_x, val);
    cursor_str_x += str::utf8::count_wide_chars(val);
  }
  
  print_command();
}

void ConsoleCommand::clear_display_all_lines(){
  // go at the top line
  // clear all lines below
  // we reposition the cursor
  // 
  
  std::cout << VTS::CURSOR_HIDE;
  
  if(cursor_str_y > 0){
    std::cout << VTS::cursor_up(cursor_str_y);
  }
  
  std::cout << VTS::delete_lines(all_lines.size());
  
  if(cursor_str_y > 0){
    std::cout << VTS::cursor_down(cursor_str_y);
  }
  
  std::cout << VTS::CURSOR_REVEAL;
  
}

void ConsoleCommand::clear_display_below(){
  std::cout << VTS::delete_lines(all_lines.size() - cursor_str_y);
}

void ConsoleCommand::delete_all_left() {
  quit_autocomp();
  last_action = CON_ACTIONS::DEL;
  selection.remove_selection();
  
  if(cursor_str_x > 0){
    pline->erase(0, cursor_str_x);
    print_command();
  }
  
}

void ConsoleCommand::delete_all_right() {
  quit_autocomp();
  last_action = CON_ACTIONS::DEL;
  selection.remove_selection();
  
  if(cursor_str_x < pline->size()){
    pline->erase(cursor_str_x, pline->size() - cursor_str_x);
    print_command();
  }
  
}

void ConsoleCommand::delete_current_line(){
  
  quit_autocomp();
  last_action = CON_ACTIONS::DEL;
  selection.remove_selection();
  
  if(all_lines.size() == 1){
    reset_all_lines();
    print_command();
    return;
  }
  
  *pline = str::string_utf8();
  
  if(cursor_str_y == 0){
    join_lines(SIDE::DOWN);
  } else {
    join_lines(SIDE::UP);
  }
  
}

void ConsoleCommand::add_line(){
  
  quit_autocomp();
  clear_selection();
  last_action = CON_ACTIONS::INSERT;
  
  if(is_paren_adjacent(*pline, cursor_str_x)){
    // we recolorize without paren highlight
    print_command(false, false);
  }
  
  if(pline->size() == 0){
    colorize();
  }
  
  str::string_utf8 empty_line = str::string_utf8{};
  
  const int tab_size = in_sequence ? 0 : program_opts.get_option("tab_size").get_int();
  const string tab = string(tab_size, ' ');
  if(tab_size > 0){
    empty_line.insert(0, tab);
  }
  
  all_lines.push_back(empty_line);
  pline = &all_lines.back();
  
  all_lines_fmt.push_back(tab);
  all_ending_quotes.push_back(NOT_A_QUOTE);
  
  // we move y down
  
  cursors_increment_str_y();
  cursor_str_x = tab_size;
  
  print_command();
  
}

void ConsoleCommand::insert_newline(){
  // this is different from add_line => here we insert the newline
  // at the current cursor position
  
  quit_autocomp();
  clear_selection();
  last_action = CON_ACTIONS::INSERT;
  
  //
  // step 1: we break the line
  //
  
  vector<str::string_utf8> line_split = pline->split(cursor_str_x);
  
  //
  // step 3: we bookkeep 
  //
  
  // we end up in the line below
  str::string_utf8 &line_top = line_split[0];
  str::string_utf8 &line_below = line_split[1];
  
  const int tab_size = program_opts.get_option("tab_size").get_int();
  if(tab_size > 0){
    line_below.insert(0, string(tab_size, ' '));
  }
  
  all_lines[cursor_str_y] = line_top;
  all_lines.insert(all_lines.begin() + cursor_str_y + 1, line_below);
  
  all_lines_fmt.insert(all_lines_fmt.begin() + cursor_str_y + 1, UNSET::STRING);
  all_ending_quotes.insert(all_ending_quotes.begin() + cursor_str_y + 1, NOT_A_QUOTE);
  
  cursor_str_x = tab_size;
  pline = &all_lines.back();
  
  // update the format of the top line...
  colorize();
  
  //
  // step 3: we print
  //
  
  // we need to reset the cursor position
  print_command(true);
  
  // and move the cursor down
  cursors_increment_str_y();
  
}


void ConsoleCommand::reset_all_lines(){
  // we reset everything
  cursor_str_x = 0;
  cursor_str_y = 0;
  cursor_term_x = 0;
  cursor_term_y = 0;
  
  all_lines = vector<str::string_utf8>{str::string_utf8()};
  pline = &all_lines[0];
  all_lines_fmt = vector<string>{UNSET::STRING};
  all_ending_quotes = vector<char>{NOT_A_QUOTE};
  
  any_long_line = false;
}

void ConsoleCommand::flush_cmd(bool save, bool is_tmp){
  // we go to the last line, then type enter, then a new prompt
  
  quit_autocomp();
  
  if(all_lines_fmt.size() > 1 && util::is_unset(all_lines_fmt.back())){
    all_lines_fmt.back() = "";
  }
  
  // we save to the history
  if(save && !phist->is_ignored_cmd(all_lines)){
    phist->add_command(is_tmp);
    if(!is_tmp){
      phist->append_history_line();
    }
  }
  
  clear_selection();
  last_action = CON_ACTIONS::CREATE;
  
  // RM OLD
  // cout << VTS::cursor_down(all_lines.size() - cursor_str_y - 1) << endl;
  
  // we go after the full command
  cursors_set_term_y_bottom();
  std::cout << endl;
  
  pcmdstate->clear();
  
  reset_all_lines();
  
}

void ConsoleCommand::clear_cmd(){
  
  quit_autocomp();
  
  clear_selection();
  
  last_action = CON_ACTIONS::CLEAR;
  
  std::cout << VTS::cursor_up(cursor_str_y) << VTS::delete_lines(all_lines.size());
  print_prompt();
  
  // we reset everything
  reset_all_lines();
}

void ConsoleCommand::select_all(bool strong){
  quit_autocomp();
  
  if(strong){
    // we just select the full line
    selection.select_range(0, pline->size(), true);
    last_action = CON_ACTIONS::SELECT_ALL;
    print_command();
    return;
  }
  
  // cursor pos
  uint cleft = selection.is_selection() ? selection.range_start() : cursor_str_x;
  uint cright = selection.is_selection() ? selection.range_end() : cursor_str_x;
  
  if(last_action != CON_ACTIONS::SELECT_ALL){
    // we save the state
    cursor_before_selection = vector<uint>({cleft, cright});
  }
  
  vector<uint> new_range = contextual_selection(*pline, cleft, cright);
  uint cleft_new = new_range[0];
  uint cright_new = new_range[1];
  
  // if no change in cursor, we cycle back
  if(cleft == cleft_new && cright == cright_new){
    cleft_new = cursor_before_selection[0];
    cright_new = cursor_before_selection[1];
  }
  
  selection.select_range(cleft_new, cright_new, true);
  cursor_str_x = cright_new;
  
  last_action = CON_ACTIONS::SELECT_ALL;
  print_command();
}

void ConsoleCommand::cut_selection(){
  quit_autocomp();
  if(selection.is_selection()){
    copy_selection();
    del(SIDE::RIGHT, false);
  }
}

void ConsoleCommand::copy_selection(){
  quit_autocomp();
  if(selection.is_selection()){
    str::string_utf8 sel_txt = pline->substr(selection.range_start(), selection.range_size());
    simpleclipboard::set_clipboard(sel_txt.str());
  }
}

void ConsoleCommand::paste(StringKeySequence *psequence){
  // NOTE:
  // We paste in two possible containers:
  // - if psequence == nullptr, we paste in `sequence`
  //   which is a variable defined at the class level
  // - else we paste in the StringKeySequence pointed to by
  //   psequence, which is local to a function (and hence
  //   gets destroyed once out of the function)
  //   
  
  quit_autocomp();
  // we start by deleting if there is a selection
  if(selection.is_selection()){
    del(SIDE::RIGHT, false);
  }
  
  // then we insert the text
  string str = simpleclipboard::get_clipboard();
  if(!str.empty()){
    // we implement paste as an ascii sequence
    // NOTES:
    // - we interpret \r\n as a single regular newline
    //
    
    const bool is_local_sequence = psequence != nullptr;
    StringKeySequence &my_sequence = is_local_sequence ? *psequence : sequence;
    
    my_sequence.clear();
    const uint n = str.size();
    string tmp;
    uint i = 0;
    while(i < n){
      const char &c = str[i++];
      if(c == '\r'){
        if(i < n && str[i] == '\n'){
          // we skip the next newline
          ++i;
        }
        my_sequence.push_back(tmp);
        tmp.clear();
      } else if(c == '\n'){
        my_sequence.push_back(tmp);
        tmp.clear();
      } else {
        tmp.push_back(c);
      }
    }
    
    my_sequence.push_back(tmp);
  }
}

void ConsoleCommand::undo(){
  pcmdstate->undo();
}

void ConsoleCommand::redo(){
  pcmdstate->redo();
}

void ConsoleCommand::run_autocomp(){
  
  //
  // branch 0-A: history navigation 
  //
  
  if(all_lines.size() == 1 && pline->size() > 0 && (*pline)[0] == '@'){
    string context;
    if(pline->size() > 0){
      context = pline->str().substr(1);
    }
    
    vector<string> past_commands = phist->get_past_commands();
    
    const bool empty_context = str::no_nonspace_char(context);
    
    StringMatch hist_match;
    if(past_commands.size() == 0){
      hist_match.set_cause_no_match("No history available.");

    } else if(empty_context){
      hist_match = StringMatch(context, past_commands);

    } else {
      // NOTA: we reverse the order to that the most 
      // relevant is at the top
      
      reverse(past_commands.begin(), past_commands.end());
      
      // we need to keep track of the ID the entry refers to
      const int n = past_commands.size();
      vector<string> hist_id(n);
      for(int i = 0 ; i < n ; ++i){
        hist_id[i] = std::to_string((n - 1) - i);
      }
      
      str::MetaStringVec past_commands_msv(past_commands);
      past_commands_msv.set_meta("hist_id", hist_id);
      
      hist_match = stringtools::string_match(context, past_commands_msv);
      
    }
    
    in_hist_autocomp = true;
    pautocomp->set_matches(hist_match, false);
    pautocomp->display(true);
    if(empty_context){
      pautocomp->move(SIDE::BOTTOM);
    }
    in_autocomp = true;
    return;
  }
  
  
  //
  // branch 0-B: special functions and options
  //
  
  if(all_lines.size() == 1 && pline->size() > 0 && (*pline)[0] == '%' && cursor_str_x > 0){
    
    StringMatch special_matches = ac_suggestion_special_command();
    
    pautocomp->set_matches(special_matches, false);
    pautocomp->display(true);
    if(special_matches.has_meta("autocomp-bottom")){
      pautocomp->move(SIDE::BOTTOM);
    }
    in_autocomp = true;
    return;
  }
  
  
  //
  // step 1: mini parse of the context
  //
  
  
  //
  // step 1 -- a:  we find out if we're in a string
  //
  
  
  char quote = cursor_str_y == 0 ? NOT_A_QUOTE : all_ending_quotes[cursor_str_y - 1];
  bool is_in_quote = quote != NOT_A_QUOTE;
  
  const string &str = pline->str();
  uint i = 0;
  int index_quote_start = -1;
  
  const uint cursor_str_x_narrow = pline->wide_to_narrow_index(cursor_str_x);
  while(i < cursor_str_x_narrow){
    const char &c = str[i];
    if(is_in_quote || str::is_quote(c)){
    
      if(!is_in_quote){
        index_quote_start = i;
        quote = c;
        ++i;
      }
      
      while( i < cursor_str_x_narrow && !( str[i] == quote && !str::is_escaped(str, i) ) ){
        i++;
      }
      
      if(i < cursor_str_x_narrow){
        ++i;
        is_in_quote = false;
      } else {
        is_in_quote = true;
      }
    }
    
    ++i;
  }
  
  // we don't account for multiline paths.... does not make sense
  is_in_quote = is_in_quote && index_quote_start >= 0;
  
  // special case for path
  string path_context;
  if(is_in_quote){
    // path autocomplete
    ++index_quote_start; // => we go past the first quote
    path_context = str.substr(index_quote_start, cursor_str_x_narrow - index_quote_start);
  }
  
  
  //
  // step 1 -- b:  getting the full context
  //
  
  
  // creating the context before and after the cursor
  string before_cursor;
  for(uint y = 0 ; y < cursor_str_y ; ++y){
    before_cursor += all_lines[y].str();
  }
  
  for(uint i = 0 ; i < cursor_str_x_narrow ; ++i){
    before_cursor += str[i];
  }
  
  string after_cursor;
  for(uint i = cursor_str_x_narrow ; i < str.size() ; ++i){
    after_cursor += str[i];
  }
  
  AutocompleteContext context;
  context.before_cursor = before_cursor;
  context.after_cursor = after_cursor;
  context.is_in_path = is_in_quote;
  context.path_query = path_context;
  
  //
  // step 2: the auto complete 
  //
  
  
  StringMatch matches = pserver_ac->make_suggestions(context);
  if(matches.empty() && matches.get_cause_no_match().empty()){
    quit_autocomp();
    
  } else {
    pautocomp->set_matches(matches, false);
    pautocomp->display(true);
    in_autocomp = true;
  }
  
}

void ConsoleCommand::update_autocomp(char code){
  
  StringMatch matches = pserver_ac->update_suggestions(code);
  
  if(matches.get_cause_no_match() == "unavailable"){
    return;
  }
  
  if(matches.get_cause_no_match() == "identical"){
    return;
  }
  
  pautocomp->set_matches(matches, false);
  pautocomp->display(true);
  
}

void ConsoleCommand::quit_autocomp(){
  if(in_autocomp){
    pautocomp->clear();
    pserver_ac->quit_autocomp();
    in_hist_autocomp = false;
    in_autocomp = false;
  }
}

void ConsoleCommand::accept_autocomp(bool is_enter){
  
  if(pautocomp->no_suggests()){
    quit_autocomp();
    return;
  }
  
  AutocompleteResult suggest = pautocomp->get_selection();
  
  if(in_hist_autocomp){
    // special case: we replace the full entry
    uint hist_id = pautocomp->get_selection_id();
    
    if(suggest.has_key("hist_id")){
      hist_id = suggest.get_meta<uint>("hist_id");
    }

    ConsoleCommandSummary new_cmd = phist->get_command_at(hist_id);
    copy_cmd(new_cmd);
    print_command();
    quit_autocomp();
    return;
  }
  
  
  string x = pautocomp->get_target();
  uint nx = str::utf8::count_wide_chars(x);
  uint n_del = suggest.n_delete_left();
  
  if(nx > 0 || n_del > 0){
    pline->erase(cursor_str_x - nx - n_del, nx + n_del);
  }
  
  pline->insert(cursor_str_x - nx - n_del, suggest.get_string());
  cursor_str_x += str::utf8::count_wide_chars(suggest.get_string()) - nx - n_del + suggest.cursor_shift();
  print_command();
  
  if(suggest.continue_autocomp() && !is_enter){
    // on enter: we don't continue to suggest
    // ie we only continue to suggest on TAB
    // 
    
    run_autocomp();
    if(pautocomp->no_suggests() || pautocomp->is_identity_suggest()){
      // we close
      pautocomp->clear();
      in_autocomp = false;
    }
  } else {
    pautocomp->clear();
    in_autocomp = false;
    
    if(suggest.cursor_shift() == -1 && suggest.get_string().back() == ')'){
      // there was an addition of parentheses
      last_action = CON_ACTIONS::INSERT_PAIRED_PAREN;
    }
  }
  
}

void ConsoleCommand::tab(){
  
  if(in_autocomp){
    accept_autocomp();
  } else {
    run_autocomp();
  }
  
}

void ConsoleCommand::escape(){
  clear_selection();
  if(in_autocomp){
    quit_autocomp();
  } else {
    // we exit the current code
    // we don't save it in the history
    if(!str::no_nonspace_char(pline->str())){
      // we print in grey commands that contain at least one character
      print_command_grey();
    } else if(all_lines.size() > 1){
      // multilines => also in grey
      print_command_grey();
    }
    
    // we go bottom
    flush_cmd(true, true);
    print_command();
  }
  
}

CommandToEvaluate ConsoleCommand::enter(){
  
  if(in_autocomp){
    accept_autocomp(true);
    return CommandToEvaluate();
  }
  
  string full_cmd = collect();
  
  if(!in_command){
    // NOTA: we accept empty lines
    flush_cmd();
    command_result = CommandToEvaluate(full_cmd, true, false);
    return command_result;
  }
  
  // on enter
  if(!in_shortcut){
    const ParsedShortcut &shortcut = program_opts.get_option("shortcut.enter").get_shortcut();
    if(!shortcut.empty()){
      // behavior:
      // - if the first condition is valid: we apply the shortcut and then leave this function
      // - else: we continue in this function
      
      std::deque<ShortcutAction> all_commands = shortcut.get_all_commands();
      while(!all_commands.empty() && all_commands.at(0).is_if()){
        apply_shortut_if(all_commands);
      }
      
      if(!all_commands.empty()){
        CommandToEvaluate res = *apply_shortuts(shortcut);
        return res;
      }
      
    }
  }
  
  // %special commands
  if(is_special_command()){
    
    command_to_send.clear();
    
    full_cmd = collect_special();
    
    if(util::ends_with_backslash(all_lines.back().str())){
      // we add a new line
      add_line();
      return CommandToEvaluate();
    }
    
    special_command(full_cmd);
    
    if(!command_to_send.empty()){
      return CommandToEvaluate(command_to_send, true, false);
      
    } else {
      print_prompt();
      
    }
    
    return CommandToEvaluate();
  }
  
  CmdParsed cmd_parsed = plgsrv->parse_command(full_cmd);
  
  if(full_cmd.empty()){
    // we don't need to bother the language server
    
    if(!program_opts.get_option("ignore_empty_lines").get_logical()){
      // we save the command
      io_backup.push_back("");
      io_backup_type.push_back(IO_TYPE::INPUT);
    }
    
    print_command(false, false);
    flush_cmd(false);
    print_prompt();
    return CommandToEvaluate();
    
  } else if(plgsrv->is_line_comment(full_cmd)){
    // we refuse comments
    // => can it lead to bugs in multiline commands starting with a comment?
    // I don't think so unless the newlines where entered with ctrl+N
    // In that case, that's life!
    // 
    
    if(program_opts.get_option("ignore_comment").get_logical()){
      // we clear
      clear_cmd();
    } else {
      // we save the command and carry on
      io_backup.push_back(collect_fmt());
      io_backup_type.push_back(IO_TYPE::INPUT);
      
      flush_cmd(false);
      print_prompt();
    }
    
    return CommandToEvaluate();
    
  } if(cmd_parsed.is_continuation){
    add_line();
    return CommandToEvaluate();

  } else if(cmd_parsed.is_error){
    // we should do something nice, like c++'s compiler errors
    // Ideally, we should catch the parsing failure
    flush_cmd();
    command_result = CommandToEvaluate(full_cmd, true, true);
    return command_result;
    
  } else {
    
    // we save the command
    io_backup.push_back(collect_fmt());
    io_backup_type.push_back(IO_TYPE::INPUT);
    
    // then a new prompt
    print_command(false, false);
    flush_cmd();
    
    // this is fine => we send
    command_result = CommandToEvaluate(full_cmd, true, false);
    
    if(time_all){
      time_sent = clock::now();
    }
    
    return command_result;
  }
  
  return CommandToEvaluate();
}

void ConsoleCommand::insert_newline_if_needed_to_be_leftmost() const {
  
  CursorInfo cursor(handle_out);
  
  if(!cursor.x_abs == 0){
    std::cout << "\n";
  }
  
}

inline bool ConsoleCommand::is_inline_comment(const string &x, int i, int n){
  // ad hoc function, i is right after c
  
  if(inline_comment.empty()){
    return false;
  }
  
  if(x[i] == inline_comment[0]){
    if(inline_comment.size() == 1){
      return true;
    }
    
    return i + 1 < n && x[i + 1] == inline_comment[1 + 1];
  }
  
  return false;
}

size_t ConsoleCommand::colorize(bool paren_highlight){
  
  if(!in_command){
    // when we're not in a command => we don't colorize
    // non commands can only be one-liners (typically: cin)
    all_lines_fmt[0] = pline->str();
    return 0;
  }
  
  // information on selection
  const bool is_sel = selection.is_selection();
  const uint sel_start = is_sel ? pline->wide_to_narrow_index(selection.range_start()) : 0;
  const uint sel_end = is_sel ? pline->wide_to_narrow_index(selection.range_end()) : 0;
  
  //
  // branch: special commands
  //
  
  
  if(is_special_command()){
    // simple colorization: 1) command, 2) arguments
    string line_fmt;
    
    line_fmt += opt_color("special_command");
    
    string current_line = pline->str();
    
    // we add the selection if needed
    if(is_sel){
      current_line.insert(sel_end, VTS::BG_DEFAULT);
      current_line.insert(sel_start, opt_color("selection_bg"));
    }
    
    uint i = 0;
    uint n = current_line.size();
    while(i < n && current_line[i] != ' '){
      line_fmt += current_line[i++];
    }
    
    if(i < n){
      line_fmt += opt_color("special_argument");
      
      while(i < n){
        line_fmt += current_line[i++];
      }
    }
    
    line_fmt += VTS::RESET_FG_BG;
    
    all_lines_fmt[0] = line_fmt;
    return 0;
  }
  
  //
  // branch: regular commands 
  //
  
  
  const uint line_start = util::is_unset(all_lines_fmt[0]) ? 0 : cursor_str_y;
  size_t n_lines = all_lines.size();
  
  char quote = line_start == 0 ? NOT_A_QUOTE : all_ending_quotes[line_start - 1];
  bool is_in_quote = quote != NOT_A_QUOTE;
  
  char new_ending_quote = quote;
  
  #define SELECTION_FORMAT                    \
    if(look_for_selection){                   \
      if(i == sel_start){                     \
        current += opt_color("selection_bg"); \
      } else if(i == sel_end){                \
        current += VTS::BG_DEFAULT;           \
        look_for_selection = false;           \
      }                                       \
    }
  
  
  uint idx = line_start;
  for(; idx<n_lines ; ++idx){
    
    bool look_for_selection = is_sel && idx == line_start;
    const string &current_line = all_lines[idx].str();
    uint n = current_line.size();
    uint i = 0;
    string line_fmt;
    string current;
    
    while(i < n){
      
      if(look_for_selection && i == sel_start){
        line_fmt += opt_color("selection_bg");
      }
      
      const char c = current_line[i++];
      
      if(is_in_quote && c == quote){
        // special case: first character in a newline is the ending quote
        
        line_fmt += opt_color("string") + c + VTS::FG_DEFAULT;
        is_in_quote = false;
        new_ending_quote = NOT_A_QUOTE;
        
      } else if(is_in_quote || str::is_quote(c)){
        
        if(!is_in_quote){
          // the previous quote is carried
          quote = c;
        }
        
        int n_interpol_open = 0;
        current += c;
        while( i < n && !( current_line[i] == quote && !str::is_escaped(current_line, i) ) ){
          SELECTION_FORMAT
          
          if(current_line[i] == '{' && !str::is_escaped(current_line, i)){
            if(n_interpol_open == 0){
              current += opt_color("interpolation");
            }
            ++n_interpol_open;
            
          } else if(current_line[i] == '}' && !str::is_escaped(current_line, i)){
            if(n_interpol_open == 1){
              n_interpol_open = 0;
              current += current_line[i++] + opt_color("string");
              continue;
            }
            --n_interpol_open;
            
          }
          
          current += current_line[i++];
        }
        
        if(i < n){
          // we're out of the quote
          if(i == sel_end){
            current += VTS::BG_DEFAULT;
            look_for_selection = false;
          }
          new_ending_quote = NOT_A_QUOTE;
          ++i;
          current += quote;
          is_in_quote = false;
        } else {
          // we're still inside a quote and we reached the end of the line
          new_ending_quote = quote;
          is_in_quote = true;
        }
        
        line_fmt += opt_color("string") + current + VTS::FG_DEFAULT;
        
      } else if(str::is_digit(c) || (i < n && c == '.' && str::is_digit(current_line[i]))){
        
        current += c;
        while(i < n && !str::is_control_char(current_line[i])){
          SELECTION_FORMAT
          current += current_line[i++];
        }
        
        line_fmt += opt_color("num") + current + VTS::FG_DEFAULT;
        
      } else if(str::is_starting_word_char(c)){
        string raw_txt;
        raw_txt += c;
        current += c;
        while(i < n && str::is_word_char(current_line[i])){
          SELECTION_FORMAT
          raw_txt += current_line[i];
          current += current_line[i++];
        }
        
        if(util::vector_contains(lang_keywords, current)) {
          line_fmt += opt_color("keyword") + current + VTS::FG_DEFAULT;
          
        } else if(util::vector_contains(lang_controls, current)){
          line_fmt += opt_color("control") + current + VTS::FG_DEFAULT;
          
        } else {
          // finding out if it's a function
          int j = i;
          str::move_i_to_non_WS_if_i_WS(current_line, j, SIDE::RIGHT);
          bool is_fun = static_cast<uint>(j) < n && current_line[j] == '(';
          
          if(is_fun){
            line_fmt += opt_color("fun") + current + VTS::FG_DEFAULT;
          } else {
            line_fmt += opt_color("var") + current + VTS::FG_DEFAULT;
          }
        }
        
      } else if(is_inline_comment(current_line, i - 1, n)){
        
        current += c;
        while(i < n){
          SELECTION_FORMAT
          current += current_line[i++];
        }
        line_fmt += opt_color("comment") + current + VTS::FG_DEFAULT;
        
      } else {
        current += c;
        while(i < n && str::is_nonquote_control(current_line[i]) && !is_inline_comment(current_line, i, n)){
          SELECTION_FORMAT
          current += current_line[i++];
        }
        
        line_fmt += current;
      }
      
      if(look_for_selection && i == sel_end){
        line_fmt += VTS::BG_DEFAULT;
        look_for_selection = false;
      }
      
      current = "";
    }
    
    if(look_for_selection){
      line_fmt += VTS::BG_DEFAULT;
    }
    
    all_lines_fmt[idx] = line_fmt;
    
    if(all_ending_quotes[idx] == new_ending_quote){
      // means the status of the next lines hasn't changed => no need to process them
      
      // we just check that the next line has already been processed at least once
      if(idx + 1 < n_lines && !util::is_unset(all_lines_fmt[idx + 1])){
        break;
      }
    }
    
    all_ending_quotes[idx] = new_ending_quote;
    
    if(idx + 1 == n_lines){
      break;
    }
    
  }
  
  //
  // paren highlighting on the current line 
  //
  
  if(paren_highlight && !is_sel && sequence.empty()){
    str::ParenMatcher info_paren(pline->str(), cursor_str_x);
    if(info_paren.pair_found){
      
      // paren is found, we need to convert into VTS
      string &current_line = all_lines_fmt[cursor_str_y];
      const uint &n = current_line.size();
      uint i_start = info_paren.i_start;
      uint i_end = info_paren.i_end;
      
      uint i = 0;
      uint i_noVTS = 0;
      uint i_start_VTS = UNSET::UINT;
      uint i_end_VTS = UNSET::UINT;
      while(i < n){
        const char c = current_line[i];
        if(c == 27){
          ++i;
          while(i < n && !str::is_ascii_letter(current_line[i])){
            ++i;
          }
          ++i;
        }
        
        if(i_start == i_noVTS){
          i_start_VTS = i;
          break;
        }
        
        ++i_noVTS;
        ++i;
      }
      
      while(i < n){
        const char c = current_line[i];
        if(c == 27){
          ++i;
          while(i < n && !str::is_ascii_letter(current_line[i])){
            ++i;
          }
          ++i;
        }
        
        if(i_end == i_noVTS){
          i_end_VTS = i;
          break;
        }
        
        ++i_noVTS;
        ++i;
      }
      
      if(i_end_VTS < n){
        current_line.insert(i_end_VTS + 1, VTS::UNDERLINE_NOT + VTS::BOLD_NOT);
        current_line.insert(i_end_VTS, VTS::UNDERLINE + VTS::BOLD);
        current_line.insert(i_start_VTS + 1, VTS::UNDERLINE_NOT + VTS::BOLD_NOT);
        current_line.insert(i_start_VTS, VTS::UNDERLINE + VTS::BOLD);
      }
    }
  }
  
  return idx;
}

void ConsoleCommand::clear_screen(){
  cout << VTS::CLEAR_SCREEN << VTS::cursor_move_at_y(0);
  print_command();
}

void ConsoleCommand::clear_selection(){
  if(selection.is_selection()){
    // we need to refresh the line
    selection.remove_selection();
    print_command();
  }
}

void ConsoleCommand::selection_stash(){
  if(selection.is_selection()){
    str::string_utf8 sel_txt = pline->substr(selection.range_start(), selection.range_size());
    stashed_selection = sel_txt.str();
  } else {
    stashed_selection.clear();
  }
}

void ConsoleCommand::selection_pop(){
  if(!stashed_selection.empty()){
    add_char(stashed_selection, true);
  }
}

void ConsoleCommand::command_stash(){
  stashed_cmd = ConsoleCommandSummary(this);
  reset_all_lines();
  print_command();
}

void ConsoleCommand::command_pop(){
  copy_cmd(stashed_cmd);
  print_command();
  stashed_cmd = ConsoleCommandSummary();
}

void ConsoleCommand::print_command(bool full, bool paren_highlight, uint str_y_end_custom){
  
  const size_t n_lines = all_lines.size();
  
  uint str_y_start = 0;
  uint str_y_end = 0;
  if(full){
    if(util::is_unset(all_lines_fmt[0])){
      colorize();
    }
    str_y_end = n_lines - 1;
    
  } else {
    str_y_start = cursor_str_y;
    str_y_end = colorize(paren_highlight);
    
    if(any_long_line){
      str_y_end = n_lines - 1;
    }
    
  }
  
  if(!util::is_unset(str_y_end_custom) && str_y_end_custom > str_y_end){
    str_y_end = str_y_end_custom;
  }
  
  // we set up the cursor at the top
  cursors_save_and_set(0, str_y_start);
  // => it sets cursor_term_x/y at the appropriate values
  
  // hide cursor
  cout << VTS::CURSOR_HIDE;
  
  size_t total_lines = 0;
  
  for(size_t str_y = str_y_start ; str_y <= str_y_end ; ++str_y){
    std::cout << VTS::CURSOR_LEFTMOST;
    
    const string &line = all_lines_fmt[str_y];
    
    const uint h = get_line_height(str_y);
    total_lines += h;
    if(h == 1){
      // simple case
      
      print_prompt(str_y == 0);
      std::cout << line << VTS::CLEAR_RIGHT;
      
    } else {
      any_long_line = true;
      
      const uint w = get_line_max_width(str_y);
      vector<string> line_split = str::str_split_at_width(line, w);
      
      if(line_split.size() != h){
        util::error_msg("Internal error in print_command:\n",
                        "line_split.size() = ", line_split.size(), "\n",
                        "h = ", h);
        
        while(line_split.size() < h){
          line_split.push_back("");
        }
      }
      
      const uint psize = prompt_size(str_y == 0);
      string prompt_space = "\u2026";
      for(size_t i = 1 ; i < psize ; ++i){
        prompt_space.push_back(' ');
      }
      
      
      for(size_t i = 0 ; i < h ; ++i){
        if(i == 0){
          print_prompt(str_y == 0);
        } else {
          std::cout << prompt_space;
        }
        
        std::cout << line_split[i] << VTS::CLEAR_RIGHT;
        
        if(i + 1 < h){
          std::cout << "\n";
        }
      }
      
    }
    
    if(str_y + 1 <= str_y_end){
      // move one line down
      std::cout << "\n";
    }
  }
  
  // go back to the right location
  std::cout << VTS::CURSOR_LEFTMOST;
  // we add all the (new)lines we've printed
  cursor_term_y += total_lines - 1;
  cursors_restore();
  
  // reveal cursor
  cout << VTS::CURSOR_REVEAL;
  
  pcmdstate->add_state();
  
}

void ConsoleCommand::print_command_grey(){
  // I could integrate it into print_command() with an option
  // but that function would become quite complex
  // so I prefer copy pasting + ad hoc changes
  // 
  
  const size_t nlines = all_lines.size();
  const size_t y_end = nlines - 1;
  
  // we set up the cursor at the top
  cursors_save_and_set(0, 0);
  // => it sets cursor_term_x/y at the appropriate values
  
  // hide cursor
  cout << VTS::CURSOR_HIDE;
  
  size_t total_lines = 0;
  
  for(size_t str_y = 0 ; str_y <= y_end ; ++str_y){
    std::cout << VTS::CURSOR_LEFTMOST << VTS::fg_grey(72);
    
    const string &line = all_lines[str_y].str();
    
    const uint h = get_line_height(str_y);
    total_lines += h;
    if(h == 1){
      // simple case
      
      print_prompt_raw(str_y == 0);
      std::cout << line << VTS::CLEAR_RIGHT;
      
    } else {
      
      const uint w = get_line_max_width(str_y);
      vector<string> line_split = str::str_split_at_width(line, w);
      
      if(line_split.size() != h){
        util::error_msg("Internal error in print_command:\n",
                        "line_split.size() = ", line_split.size(), "\n",
                        "h = ", h);
        
        while(line_split.size() < h){
          line_split.push_back("");
        }
      }
      
      const uint psize = prompt_size(str_y == 0);
      string prompt_space = "\u2026";
      for(size_t i = 1 ; i < psize ; ++i){
        prompt_space.push_back(' ');
      }
      
      for(size_t i = 0 ; i < h ; ++i){
        if(i == 0){
          print_prompt_raw(str_y == 0);
        } else {
          std::cout << prompt_space;
        }
        
        std::cout << line_split[i] << VTS::CLEAR_RIGHT;
        
        if(i + 1 < h){
          std::cout << "\n";
        }
      }
      
    }
    
    if(str_y + 1 <= y_end){
      // move one line down
      std::cout << "\n";
    }
  }
  
  
  /*
  for(size_t idx = 0 ; idx<n_lines ; ++idx){
    cout << VTS::CLEAR_LINE << VTS::fg_grey(72);
    cout << (idx == 0 ? current_prompt_main : prompt_cont);
    cout << all_lines[idx].str();
    cout << VTS::FG_DEFAULT;
    
    if(idx + 1 < n_lines){
      // move one line down
      cout << endl;
    }
  }
  
  // go back up to the right line
  if(n_lines - 1 > cursor_str_y){
    cout << VTS::cursor_up(n_lines - 1 - cursor_str_y);
  }
  */
   
  // go back to the right location
  std::cout << VTS::CURSOR_LEFTMOST;
  // we add all the (new)lines we've printed
  cursor_term_y += total_lines - 1;
  cursors_restore();
  
  // reveal cursor
  cout << VTS::CURSOR_REVEAL;
}


string ConsoleCommand::current_line(){
  return pline->str();
}


string ConsoleCommand::collect(){
  // we save the current line before collection
  
  if(all_lines.size() == 1){
    return pline->str();
  }
  
  string res = all_lines[0].str();
  for(uint i=1 ; i<all_lines.size() ; ++i){
    res += "\n" + all_lines[i].str();
  }
  
  return res;
}

string ConsoleCommand::collect_special(){
  // we transform all the lines into a single line
  
  string res;
  for(uint i = 0 ; i < all_lines.size() ; ++i){
    string line = str::trim_WS(all_lines[i].str());
    if(line.empty()){
      continue;
    }
    
    if(line.back() == '\\'){
      line.pop_back();
    }
    
    res += (i > 0 ? " " : "") + line;
  }
  
  return res;
}

uint64_t ConsoleCommand::hash(){
  
  if(all_lines.size() == 1){
    return str::hash_string(str::trim_WS(collect()));
  }
  
  return str::hash_string(collect());
}

string ConsoleCommand::collect_fmt(){
  
  if(all_lines_fmt.size() == 1){
    return clear_underlines(all_lines_fmt[0]);
  }
  
  string res = cursor_str_y == 0 ? clear_underlines(all_lines_fmt[0]) : all_lines_fmt[0];
  for(uint i = 1 ; i < all_lines_fmt.size() ; ++i){
    res +=  "\n" + (cursor_str_y == i ? clear_underlines(all_lines_fmt[i]) : all_lines_fmt[i]) ;
  }
  
  return res;
}

void ConsoleCommand::copy_cmd(const ConsoleCommandSummary &new_cmd){
  
  all_lines = new_cmd.all_lines;
  all_lines_fmt = new_cmd.all_lines_fmt;
  all_ending_quotes = new_cmd.all_ending_quotes;
  
  cursor_str_x = new_cmd.cursor_str_x;
  cursor_str_y = new_cmd.cursor_str_y;
  
  pline = &all_lines[cursor_str_y];
  
  cursor_term_x_request = 0;
  
  selection = new_cmd.selection;
  
}

void ConsoleCommand::clear_infobar(){
  if(is_infobar){
    cout << VTS::CURSOR_SAVE << VTS::CURSOR_HIDE;
    
    CursorInfo curs_info(handle_out);
    cout << VTS::cursor_move_at_xy(1, curs_info.win_height);
    cout << VTS::CLEAR_LINE;
    
    cout << VTS::CURSOR_RESTORE << VTS::CURSOR_REVEAL;
    
    is_infobar = false;
  }
}

void ConsoleCommand::infobar(const string &x){
  // write a message in the infobar
  
  is_infobar = true;
  
  CursorInfo curs_info(handle_out);
  
  // we hide the cursor
  cout << VTS::CURSOR_HIDE;
  
  bool at_bottom = curs_info.y_abs + 1 == curs_info.win_height;
  if(at_bottom){
    // we write below the current line
    cout << VTS::CURSOR_DOWN;
  }
  
  // we go at the bottom
  cout << VTS::cursor_move_at_xy(1, curs_info.win_height);
  
  cout << VTS::CLEAR_LINE;
  cout << "sircon " + x;
  
  // we restore the cursor
  cout << VTS::cursor_move_at_xy(curs_info.x_abs, curs_info.y_abs - at_bottom);
  
  cout << VTS::CURSOR_REVEAL;
}

//
// ... options
//

const ParsedArg& ConsoleCommand::set_program_option(const string &key, const string &value,
                                                       const op_write_t type){
  program_opts.set_option(key, value, type);
  return program_opts.get_option(key);
}

const ParsedArg& ConsoleCommand::get_program_option(const string &key,
                                                       const util::DoCheck options) const {
  return program_opts.get_option(key, options);
}

bool ConsoleCommand::is_special_command() const {
  const string &first_line = all_lines[0].str();
  return !first_line.empty() && first_line.front() == '%';
}


//
// read_command ----------------------------------------------------------------
//

inline bool is_key_letter(KEY_EVENT_RECORD key){
  return key.uChar.UnicodeChar >= 32 || key.uChar.UnicodeChar == KEYS::ENTER;
}

inline string to_utf8(const char &ascii, const wchar_t &unicode){
  
  if(unicode > 126){
    bool is_error;
    return str::utf16_to_utf8(unicode, is_error);
  }
  
  string tmp;
  tmp += ascii;
  return tmp;
}

string ConsoleCommand::read_line(){
  ReadOptions opts;
  return read_line(opts);
}

string ConsoleCommand::read_line(const ReadOptions &opts){
  // NOTA: the default return is the empty string
  // this function only considers key-events and no other events 
  // 
  
  // we might use read_line within a command (like in special functions)
  // => we save the state and restore it when leaving
  ResetInCommandOnLeave in_cmd(this);
  
  in_command = false;
  reset_all_lines();
  
  DWORD n_events_read = 0;
  vector<INPUT_RECORD> inputs_read;
  
  uint n_char_max = opts.get_n_char();
  
  vector<string> choices = opts.get_choices();
  const bool is_choices = !choices.empty();
  bool tab_for_choices = is_choices;
  const bool is_choices_clear_wrong = opts.is_choices_clear_wrong() && is_choices;
  
  const bool is_print = opts.is_print() && !opts.is_no_block();
  
  if(is_print){
    current_prompt_main = opts.get_prompt();
    current_prompt_color = VTS::FG_DEFAULT;
    print_prompt();
  }
  
  _KEY_EVENT_RECORD key_in = {0, 1, 0, 0, 0, 0};
  
  // local sequences
  StringKeySequence key_sequence;
  
  while(true){
    
    //
    // STEP 1: reading
    //
    
    key_sequence.clear();
    
    if(tab_for_choices){
      // we automatically proc TAB
      tab_for_choices = false;
      key_in.uChar.UnicodeChar = KEYS::TAB;
      
    } else if(pline->size() >= n_char_max){
      // we automatically ENTER
      key_in.uChar.UnicodeChar = KEYS::ENTER;
      
    } else {
      // we read the key
      //
      
      // cout << "CAPTURING INPUT\n";
      while(true){
        // we read until we have a key down. we ignore key up events
        DWORD n_events;
        GetNumberOfConsoleInputEvents(handle_in, &n_events);
        inputs_read.resize(n_events);
        
        if(! ReadConsoleInputW(handle_in, &inputs_read[0], n_events, &n_events_read) ){
          console_error("ReadConsoleInput failed");
        }
        
        if(n_events_read > 0 && inputs_read[0].EventType == KEY_EVENT){
          key_in = inputs_read[0].Event.KeyEvent;
          if(key_in.bKeyDown == 1){
            break;
          }
        }
        
        // this is the moment when there is no more key in the buffer
        // we return and don't wait for more
        if(opts.is_no_block()){
          // like pressing enter and sending
          // note that n_events_read == 0 leads to skipping
          // the next loop and the next if
          // 
          
          if(in_autocomp){
            accept_autocomp(true);
          }
          key_in.uChar.UnicodeChar = KEYS::ENTER;
          break;
        }
      }
      
      for(size_t i = 0 ; i < n_events_read ; ++i){
        // we only show key down events
        // we olny keep the last event
        if(inputs_read[i].EventType == KEY_EVENT){
          
          const KEY_EVENT_RECORD &key_i = inputs_read[i].Event.KeyEvent;
          if(key_i.bKeyDown == 0){
            continue;
          }
          
          if(_debug_char){
            cout << "i = " << i << ", ";
            print_key(key_i);
          }
          
          key_in = key_i;
          key_sequence.push_back(key_i);
          
        }
      }
      
      if(key_sequence.get_num_insertions() >= 2){
        // this is a sequence
        // we never add return in the string
        // we flush everything beyond the first new line
        if(is_print){
          add_char(key_sequence.front());
        } else {
          pline->push_back(key_sequence.front());
        }
        key_sequence.clear();
        // we flush
        FlushConsoleInputBuffer(handle_in);
        continue;
      }
    }
    
    DWORD &control_state = key_in.dwControlKeyState;
    WCHAR &unicode = key_in.uChar.UnicodeChar;
    
    if(opts.is_raw_key()){
      if(unicode != KEYS::ENTER){
        *(opts.get_raw_key_pointer()) = unicode;
      }
      
      return "";
    } 
    
    bool is_ctrl = (control_state & CSTATE::CTRL) == CSTATE::CTRL;
    bool is_shift = (control_state & CSTATE::SHIFT) == CSTATE::SHIFT;
    
    if(!is_print){
      // we accept only printable characters and ENTER
      // we ignore the rest
      // 
      bool is_printable = unicode == KEYS::ENTER || unicode > 32;
      if(is_printable){
        // almost a printable character => a few more checks
        if(control_state == 2 &&  key_in.uChar.AsciiChar == 'd'){
          is_printable = false;
          
        } else if(unicode == 127){
          is_printable = false;
          
        }
      }
      
      if(!is_printable){
        continue;
      }
    }
    
    //
    // ------------------------------------------------------------------------- 
    //
    
    if(unicode == 0){
      WORD &virtual_key = key_in.wVirtualKeyCode;
      
      if(virtual_key == KEYS::LEFT){
        move_x(SIDE::LEFT, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::RIGHT){
        move_x(SIDE::RIGHT, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::UP){
        // not used
        
      } else if(virtual_key == KEYS::DOWN){
        // not used
        
      } else if(virtual_key == KEYS::HOME){
        move_x(SIDE::LEFTMOST, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::END){
        move_x(SIDE::RIGHTMOST, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::DEL){
        del(SIDE::RIGHT, is_ctrl);
        
      }
      
    } else {
      const char &ascii = key_in.uChar.AsciiChar;
      
      if(unicode == KEYS::BS){
        del(SIDE::LEFT, is_ctrl);
        
      } else if((is_ctrl && unicode == 23) || unicode == 127){
        // CTRL + BS
        del(SIDE::LEFT, true);
        
      } else if(control_state == 2 && ascii == 'd'){
        // CTRL + DEL
        del(SIDE::RIGHT, true);
        
      } else if(unicode == UNICODE_KEY::CTRL_A){
        select_all();
        
      } else if(unicode == UNICODE_KEY::CTRL_C){
        copy_selection();
        
      } else if(unicode == UNICODE_KEY::CTRL_L){
        clear_screen();
        
      } else if(unicode == UNICODE_KEY::CTRL_N){
        // not used
        
      } else if(unicode == UNICODE_KEY::CTRL_V){
        paste(&key_sequence);
        
        if(!key_sequence.empty()){
          // NOTA: we never add newlines in read_line
          // we wait for the user to enter a new line
          if(is_print){
            add_char(key_sequence.front());
          } else {
            pline->push_back(key_sequence.front());
          }
          key_sequence.clear();
        }
        
      } else if(unicode == UNICODE_KEY::CTRL_X){
        cut_selection();
        
      } else if(unicode >= 32){
        // REGULAR CHARACTER
        
        string val;
        if(unicode <= 126){
          val += ascii;
        } else if(!opts.is_ascii()){
          
          bool is_error = false;
          val = str::utf16_to_utf8(unicode, is_error);
          if(is_error){
            continue;
          }
          
          // CursorInfo old(handle_out);
          // cout << val;
          // CursorInfo new_pos(handle_out);
          // cout << "\nold position = " << old.x_abs << ", new position = " << new_pos.x_abs << "\n";
          
        }
        
        if(is_print){
          add_char(val);
        } else {
          pline->push_back(val);
        }
        
      } else if(unicode == KEYS::ESC){
        // => cancellation
        clear_selection();
        if(in_autocomp){
          quit_autocomp();
          
        } else if(is_choices_clear_wrong){
          pline->clear();
          if(is_print){
            print_command();
          }
          
        } else {
          if(is_print){
            flush_cmd(false);
          } else {
            pline->clear();
          }
          
          return "";
        }
        
      } else if(unicode == KEYS::ENTER){
        // we ALWAYS send, unless we're:
        //  - in auto complettion
        //  - is_choices_clear_wrong == true
        //  
        
        if(in_autocomp){
          accept_autocomp(true);
          continue;
        }
        
        string res = pline->str();
        
        if(opts.is_to_lower()){
          res = str::to_lower(res);
        } else if(opts.is_to_upper()){
          res = str::to_upper(res);
        }
        
        if(is_choices_clear_wrong){
          if(!str::is_string_in(res, choices)){
            // this is not a valid choice => we clear and continue
            pline->clear();
            if(is_print){
              print_command();
              tab_for_choices = true;
            }
            
            continue;
          }
        }
        
        if(is_print){
          flush_cmd(false);
        } else {
          pline->clear();
        }
        
        return res;
        
      } else if(unicode == KEYS::TAB){
        if(!is_print){
          // TAB: only if is_print (otherwise there's no point)
          continue;
        }
        
        // we proc the auto complete only if choices were provided
        string context = pline->str();
        StringMatch my_matches = stringtools::string_match(context, choices);
        if(choices.empty()){
          my_matches.set_cause_no_match("No suggestion available (free text expected)");
        }
        pautocomp->set_matches(my_matches, false);
        pautocomp->display(true);
        in_autocomp = true;
        
      }
      
    }
    
  }
  
  return pline->str();
}


void ConsoleCommand::write_output(const string &x, bool highlight){
  
  if(!io_backup.empty() && io_backup_type.back() == IO_TYPE::OUTPUT){
    string &last_output = io_backup[io_backup.size() - 1];
    last_output += x;
  } else {
    io_backup.push_back(x);
    io_backup_type.push_back(IO_TYPE::OUTPUT);
  }
  
  if(highlight){
    std::cout << opt_color("output_highlight") << x << VTS::FG_DEFAULT;
  } else {
    std::cout << x;
  }
  
}

CommandToEvaluate ConsoleCommand::read_command(bool is_command, string hist_name, 
                                               string prompt, bool was_error){
    
  bool command_front = true;
  in_command = is_command;
  const time_t starting_time = clock::now();
  
  if(time_all){
    std::chrono::microseconds elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(starting_time - time_sent);
    std::cout << VTS::FG_BRIGHT_BLUE << "#> Elapsed: " << format_time(elapsed_us) << VTS::RESET_FG_BG << "\n";
  }
  
  // fetching the right history
  if(hist_name == "main"){
    phist = hist_list["main"];
    
  } else {
    if(hist_name.size() == 0){
      hist_name = "rest";
    }
    
    if(auto search = hist_list.find(hist_name); search != hist_list.end()){
      phist = hist_list[hist_name];
    } else {
      std::shared_ptr<ConsoleHistory> phist_new = std::make_shared<ConsoleHistory>(this);
      hist_list[hist_name] = phist_new;
      phist = phist_new;
    }
  }
  
  // the prompt
  if(util::is_unset(prompt)){
    prompt = prompt_main;
  }
  
  current_prompt_main = prompt;
  if(is_command){
    current_prompt_color = prompt_color;
  } else {
    current_prompt_color = VTS::FG_DEFAULT;
  }
  
  // we add a newline if a previous print leads us in a wrong place (like in cat). EX:
  // > cat("bonjour")
  // bonjour
  // > |
  //
  CursorInfo curs_info(handle_out);
  if(curs_info.x_abs != 0){
    cout << endl;
  }
  
  // window size
  if(custom_win_width){
    if(curs_info.win_width < win_width){
      // we always resize when smaller
      custom_win_width = false;
      win_width = curs_info.win_width;
      plgsrv->resize_window_width(win_width);
    }
  } else {
    if(curs_info.win_width != win_width){
      win_width = curs_info.win_width;
      plgsrv->resize_window_width(win_width);
    }
  }
  
  if(curs_info.win_height != win_height){
    win_height = curs_info.win_height;
  }
  
  // shortcuts
  if(!stashed_cmd.is_unset()){
    // we always pop the stashed command
    command_pop();
  }
  
  if(stashed_shortcut_commands.size() > 0){
    if(was_error){
      stashed_shortcut_commands.clear();
    } else {
      apply_shortuts(stashed_shortcut_commands);
    }
  }
  
  // to display the prompt
  print_command();
  
  DWORD n_events_read = 0;
  vector<INPUT_RECORD> inputs_read;
  
  // we restore the command sequences, if needed
  if(in_command && !sequence_bak.empty()){
    sequence = sequence_bak;
    sequence_bak.clear();
  }
  
  if(in_command && past_command_from_sequence && was_error && !sequence.empty()){
    // we stop the command sequence and flush the console
    FlushConsoleInputBuffer(handle_in);
    sequence.clear();
  }
  
  if(in_command && sequence.empty()){
    past_command_from_sequence = false;
  }
  
  // wether we capture KB input
  if(!in_command && !sequence.empty()){
    // case: you're in the middle of a sequence, and there's a cin called
    // => you need to handle the cin, then resume the sequence
    sequence_bak = sequence;
    sequence.clear();
  }
  
  StringKeySequence tmp_sequence;
  
  int n = 0;
  while(n++ < 10000){
    
    // Read the event from the user
    tmp_sequence.clear();
    int n_tmp = 0;
    _KEY_EVENT_RECORD key_in;
    if(sequence.empty()){
      bool sequence_found = false;
      
      // this loop is to flush a sequence, if a sequence was provided
      // ex: sequences are buffered at about 1500 keyboard inputs
      //     => we want the complete sequence at once
      while(true){
        bool wait_more = true;
        // we may need to wait to fully read the buffer
        
        // this loop is to block until having a keyboard input
        while(true){
          
          // we read until we have a key down. we ignore key up events
          
          WaitForSingleObject(handle_in, Run_While_Reading_interval_ms);
          
          DWORD n_events;
          GetNumberOfConsoleInputEvents(handle_in, &n_events);
          inputs_read.resize(n_events);
          if(! ReadConsoleInputW(handle_in, &inputs_read[0], n_events, &n_events_read) ){
            console_error("ReadConsoleInput failed");
          }
          
          if(n_events_read == 0){
            
            if(sequence_found){
              // in a sequence:
              // - either we wait more to flush the sequence at once
              // - either we just carry on to process the sequence
              // 
              
              if(wait_more){
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                wait_more = false;
                continue;
                
              } else {
                break;
              }
              
            } else {
              // we're not in a sequence
              // + we run the "hook" function if provided
              // + then, we try again to get an input
              // 
              
              if(Run_While_Reading_fun){
                Run_While_Reading_fun();
              }
              
              continue;
            }
            
          } else if(inputs_read[0].EventType == KEY_EVENT){
            // good: we move on
            break;
            
          } else if(inputs_read[0].EventType == WINDOW_BUFFER_SIZE_EVENT){
            
            // this is absolutely needed, to flush all window movements
            // finding this took me ages, don't remove it
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            CursorInfo curs_info(handle_out);
            
            if(curs_info.win_width < win_width){
              // only if window gets smaller
              custom_win_width = false;
            }
            
            bool update_width = !custom_win_width && curs_info.win_width != win_width;
            
            if(update_width){
              
              const uint &new_width = curs_info.win_width;
              
              uint n_up = 0;
              uint total_delete = 0;
              n_up_total_height_when_win_resizing(n_up, total_delete, new_width);
              
              win_width = curs_info.win_width;
              plgsrv->resize_window_width(curs_info.win_width);
              win_height = curs_info.win_height;
              
              update_term_cursors(false);
              
              // we need to reprint the current command
              // but the cursors have been messed up, that's tricky
              
              std::cout << VTS::cursor_up(n_up) << VTS::cursor_move_at_x(0) << VTS::delete_lines(total_delete);
              
              cursor_term_y = 0;
              
              print_command(true);
              
            }
            
            
          } else {
            // nothing: maybe later catch other events
            // string type = inputs_read[0].EventType == MOUSE_EVENT ? "mouse" : "other";
            // cout << type << " event\n";
          }
        }
        
        if(n_events_read == 0){
          // used when sequence_found == true
          break;
        }

        if(_debug_char) cout << "n read = " << n_events_read << endl;
        
        bool ok = false;
        for(size_t i = 0; i < n_events_read; i++){
          // we only keep key down events
          if(inputs_read[i].EventType == KEY_EVENT){
            
            const KEY_EVENT_RECORD &key_i = inputs_read[i].Event.KeyEvent;
            if(_debug_char && key_i.bKeyDown){
              cout << "i = " << i << ", ";
              print_key(key_i);
            }
            
            if(key_i.bKeyDown == 0){
              continue;
            }
            
            ok = true;
            key_in = key_i;
            tmp_sequence.push_back(key_in);
            
            if(key_in.uChar.AsciiChar == 'q'){
              // used for debugging
            }
            
          } 
        }
        
        if(!ok){
          // only key up events => we read a new input
          // note that we cannot possibly be within a sequence here because sequences
          // are always made of at least one key down event
          // 
          // we can have only key up events in special cases 
          // (read_cmd starts when user was already pressing a key)
          continue;
        }
        
        if(tmp_sequence.get_num_insertions() > 1){
          // we stay in this loop to flush the sequence completely
          n_tmp++;
          sequence_found = true;
          // we need to wait a bit to receive the commands
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
          // we're not at a keyboard sequence, we quit this loop and continue
          // we will use key_in
          break;
        }
      }
    }
    
    if(tmp_sequence.get_num_insertions() > 1){
      // cout << "seq.size() = " << tmp_sequence.size() << "\n";
      sequence = tmp_sequence;
    }
    
    //
    // branch 1: right click paste // commands from VSCode
    //
    
    if(!in_command){
      // we handle the sequences differently => we ignore returns
      // we never continue the sequences => we always flush them
      
      if(command_front && key_in.uChar.UnicodeChar == KEYS::ENTER){
        // sometimes we get an unwanted ENTER right after cin
        
        const auto now = clock::now(); 
        auto n_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - starting_time);
        
        command_front = false;
        
        if(n_ms.count() < 5){
          continue;
        }
        
      }
      
      command_front = false;
      
      if(!sequence.empty()){
        // we stop at the first return and ignore the rest
        add_char(sequence.front(), true);
        sequence.clear();
        continue;
      }
      
    } else if(!sequence.empty()){
      
      // cout << "In sequence, command_front = " << command_front << "\n";
      // for(uint i=0 ; i<sequence.size() ; ++i){
      //   cout << "i = " << i << ", '" << str::ascii_printable(sequence.at(i)) << "'\n";
      // }
      
      // we discard the empty lines
      if(command_front){
        while(!sequence.empty()){
          const string &str = sequence.front();
          if(str::no_nonspace_char(str) && 
             program_opts.get_option("ignore_empty_lines").get_logical()){
            
            sequence.pop_front();
            
          } else if(plgsrv->is_line_comment(str) && 
                    program_opts.get_option("ignore_comment").get_logical()){
            
            sequence.pop_front();
            
          } else {
            // we stop at non empty, non comment
            break;
          }
        }
      }
      
      command_front = false;
      
      // we get the first command
      in_sequence = true;
      while(!sequence.empty()){
        string str = sequence.pop_front();
        add_char(str, true);
        if(sequence.is_enter()){
          // cout << "ENTER -- size = " << sequence.size();
          CommandToEvaluate res = enter();
            
          if(res.is_complete){
            past_command_from_sequence = true;
            return res;
          }
        }
      }
      in_sequence = false;
      
      // I'm not sure I should keep the lines below: too many false positives
      // 
      
      // if(past_command_from_sequence && pline->size() > 0 && sequence.empty()){
      //   // we add a return if the current command is non empty
      //   cout << "ENTER -- past_command_from_sequence\n";
      //   CommandToEvaluate res = enter();
      //   if(res.is_complete){
      //     return res;
      //   }
      // }
      
      continue;
    } else {
      command_front = false;
    }
    
    
    //
    // branch 2: regular key press
    //
    
    
    DWORD &control_state = key_in.dwControlKeyState;
    WCHAR &unicode = key_in.uChar.UnicodeChar;
    
    bool is_ctrl = (control_state & CSTATE::CTRL) == CSTATE::CTRL;
    bool is_shift = (control_state & CSTATE::SHIFT) == CSTATE::SHIFT;
    bool is_alt = (control_state & CSTATE::ALT) == CSTATE::ALT;
    
    //
    // ------------------------------------------------------------------------- 
    //
    
    #define COMMAND_EVALUATION 0
    
    if(unicode == 0){
      WORD &virtual_key = key_in.wVirtualKeyCode;
      
      if(virtual_key == KEYS::LEFT){
        move_x(SIDE::LEFT, is_ctrl, is_shift, is_alt);
        
      } else if(virtual_key == KEYS::RIGHT){
        move_x(SIDE::RIGHT, is_ctrl, is_shift, is_alt);
        
      } else if(virtual_key == KEYS::UP){
        // NOTA: is_ctrl is useless because ctrl + up/down is captured upstream and is not passed to the console API
        move_y(SIDE::UP, is_shift);
        
      } else if(virtual_key == KEYS::DOWN){
        move_y(SIDE::DOWN, is_shift);
        
      } else if(virtual_key == KEYS::HOME){
        move_x(SIDE::LEFTMOST, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::END){
        move_x(SIDE::RIGHTMOST, is_ctrl, is_shift);
        
      } else if(virtual_key == KEYS::DEL){
        del(SIDE::RIGHT, is_ctrl);
        
      }
      
    } else {
      char &ascii = key_in.uChar.AsciiChar;
      
      if(unicode == KEYS::BS){
        del(SIDE::LEFT, is_ctrl);
      } else if((is_ctrl && unicode == 23) || unicode == 127){
        // CTRL + BS
        del(SIDE::LEFT, true);
        
      } else if(control_state == 2 && ascii == 'd'){
        // CTRL + DEL
        del(SIDE::RIGHT, true);
        
      }  else if(unicode == KEYS::ESC){
        escape();
        
      } else if(unicode == KEYS::ENTER && is_alt){
        // we treat it as a shortcut
        
        const ParsedShortcut &shortcut = program_opts.get_option("shortcut.alt+enter").get_shortcut();
        if(!shortcut.empty()){
          CommandToEvaluate res = *apply_shortuts(shortcut);
          if(res.is_complete){
            return res;
          }
        }
        
      } else if(unicode == KEYS::ENTER){
        // save the line
        
        CommandToEvaluate res = enter();
        
        if(res.cmd == "_debug_char"){
          _debug_char = !_debug_char;
          flush_cmd(false);
          print_prompt();
          
        } else if(res.cmd == "_debug"){
          util::next_debug_type();
          flush_cmd(false);
          util::info_msg("Debug now set to: ", util::_debug == util::DEBUG_TYPE::MSG ? "message" : "pipe");
          print_prompt();
          
        } else if(res.cmd == "exit"){
          console_error("Exit requested");
          
        } else if(res.is_complete){
          if(in_command){
            past_command_from_sequence = false;
          }
          return res;
        }
        
      } else if(unicode == KEYS::TAB){
        tab();
        
      } else if(in_autocomp && unicode <= UNICODE_KEY::CTRL_Z){
        // Autocomplete intercepts the CTRL- shortcuts
        
        // we send the key and not the ctrl key
        char code = static_cast<char>(unicode + 64);
        update_autocomp(code);
        
      } else if(unicode <= UNICODE_KEY::CTRL_Z){
        // ctrl + a = 1
        // a = 97
        // 
        
        char letter = unicode + 96;
        string key = "shortcut.ctrl+" + string{letter};
        
        if(program_opts.key_has_option(key)){
          const ParsedShortcut &shortcut = program_opts.get_option(key).get_shortcut();
          
          stashed_shortcut_commands = shortcut.get_all_commands();
          CommandToEvaluate cmd = *apply_shortuts(stashed_shortcut_commands);
          if(cmd.is_complete){
            if(in_command){
              past_command_from_sequence = false;
            }
            return cmd; 
          }
        }
        
      } else if(unicode >= 32){
        // REGULAR CHARACTER
        
        string val;
        if(unicode <= 126){
          val += ascii;
        } else {
          
          bool is_error = false;
          val = str::utf16_to_utf8(unicode, is_error);
          if(is_error){
            continue;
          }
          
          // CursorInfo old(handle_out);
          // cout << val;
          // CursorInfo new_pos(handle_out);
          // cout << "\nold position = " << old.x_abs << ", new position = " << new_pos.x_abs << "\n";
          
          
        }
        
        add_char(val);
        
      }
    }
    
  }
  
  return CommandToEvaluate("ERROR: read_console", true, false);
}


