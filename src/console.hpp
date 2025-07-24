    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "cache.hpp"
#include "constants.hpp"
#include "VTS.hpp"
#include "stringtools.hpp"
#include "clipboard.hpp"
#include "pathmanip.hpp"
#include "util.hpp"
#include "to_index.hpp"
#include "autocomplete.hpp"
#include "program_options.hpp"
#include "shellrun.hpp"
#include "specialfunctions.hpp"
#include "history.hpp"
#include "console_util.hpp"

#include <windows.h>
#ifdef TRUE
  #undef TRUE
#endif
#ifdef FALSE
  #undef FALSE
#endif
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <thread>
#include <mutex>

using std::vector;
using std::cout;
using std::endl;
using std::string;

namespace str = stringtools;

using uint = unsigned int;

// forward declarations
class ConsoleAutocomplete;
class ConsoleCommand;
class LanguageServer;

enum class CON_ACTIONS {
  CREATE,
  INSERT,
  INSERT_PAIRED_PAREN,
  MOVE_X,
  MOVE_X_LEFTMOST,
  MOVE_X_RIGHTMOST,
  MOVE_Y,
  DEL,
  TAB,
  ENTER,
  SELECT_ALL,
  NAVIGATE_HIST,
  CLEAR,
};

inline uint min(uint a, uint b){
  return a > b ? b : a;
}

//
// debug -----------------------------------------------------------------------
//

inline void print_key(KEY_EVENT_RECORD key){
  cout <<"code = " << key.wVirtualKeyCode;
  cout << ", ascii = '" << str::ascii_printable(key.uChar.AsciiChar);
  cout << "', unicode = '" << key.uChar.UnicodeChar;
  cout << "', ctrl = " << key.dwControlKeyState << "\n";
}


//
// ReadOptions ----------------------------------------------------------------- 
//

class ReadOptions {
  bool ascii_ = false;
  bool to_lower_ = false;
  bool to_upper_ = false;
  bool no_block_ = false;
  bool print_ = true;
  bool wait_for_enter_ = false;
  uint n_char_ = UNSET::UINT;
  vector<string> choices_;
  bool choices_clear_wrong_ = false;
  string prompt_;
  
  bool raw_key_ = false;
  wchar_t *wchar_key = nullptr;
  
public:
  // we use a default constructor
  
  // setter functions
  ReadOptions& ascii(){ ascii_ = true; return *this; }
  ReadOptions& to_lower(){ to_lower_ = true; return *this; }
  ReadOptions& to_upper(){ to_upper_ = true; return *this; }
  ReadOptions& no_block(){ no_block_ = true; return *this; }
  ReadOptions& no_print(){ print_ = false; return *this; }
  ReadOptions& wait_for_enter(){ wait_for_enter_ = true; return *this; }
  ReadOptions& n_char(uint n_char){ n_char_ = n_char; return *this; }
  ReadOptions& choices(const vector<string> &choices){ choices_ = choices; return *this; }
  ReadOptions& choices_clear_wrong(){ choices_clear_wrong_ = true; return *this; }
  ReadOptions& prompt(const string &prompt){ prompt_ = prompt; return *this; }
  
  ReadOptions& raw_key(wchar_t *x){ 
    raw_key_ = true; 
    wchar_key = x;
    return *this; 
  }
  
  // getter functions
  bool is_ascii() const { return ascii_; }
  bool is_raw_key() const { return raw_key_; }
  bool is_to_lower() const { return to_lower_; }
  bool is_to_upper() const { return to_upper_; }
  bool is_no_block() const { return no_block_; }
  bool is_print() const { return print_; }
  bool is_wait_for_enter() const { return wait_for_enter_; }
  uint get_n_char() const { return n_char_; }
  vector<string> &get_choices() { return choices_; }
  const vector<string> &get_choices() const { return choices_; }
  bool is_choices_clear_wrong() const { return choices_clear_wrong_; }
  string &get_prompt() { return prompt_; }
  const string &get_prompt() const { return prompt_; }
  wchar_t *get_raw_key_pointer() const { return wchar_key; }
};

//
// KeySequence -----------------------------------------------------------------
//

class StringKeySequence {
  // implementation:
  // - it represents a sequence of keys
  // - the sequence of keys is directly inserted into an UTF-8 string
  // - each string cannot contain the ENTER key
  // - ENTER is represented by "the space" between two element
  // - the empty string is valid
  // ex: sequence = {"bonjour", "les gens", "", "ca va?"}
  //     is equivalent to "bonjour\nles gens\n\nca va?"
  // 
  
  std::deque<string> sequence;
  int n_keys = 0;
  
public:
  
  // modifiers
  void push_back(const string &x){
    sequence.push_back(x);
  }
  
  void push_back(const KEY_EVENT_RECORD &x){
    const char &ascii = x.uChar.AsciiChar;
    const char &unicode = x.uChar.UnicodeChar;
    
    if(unicode < 32 && !stringtools::is_enter(ascii)){
      // we don't accept control sequences unless ENTER
      return;
    }
    
    ++n_keys;
    
    if(sequence.empty()){
      sequence.push_back("");
    }
    
    string &tmp = sequence.back();
    if(unicode > 126){
      bool is_error;
      tmp += str::utf16_to_utf8(unicode, is_error);
    } else if(stringtools::is_enter(ascii)){
      sequence.push_back("");
    } else {
      tmp += ascii;
    }
  }
  
  void push_back(const StringKeySequence &x){
    if(x.sequence.empty()){
      return;
    }
    
    if(sequence.empty()){
      sequence = x.sequence;
      n_keys = x.n_keys;
      return;
    }
    
    string &last = sequence.back();
    last += x.sequence[0];
    sequence.insert(sequence.end(), x.sequence.begin() + 1, x.sequence.end());
    
    n_keys += x.n_keys;
  }
  
  string pop_front(){
    string tmp = sequence.front();
    sequence.pop_front();
    return tmp;
  }
  
  void clear(){
    n_keys = 0;
    sequence.clear();
  }
  
  // non modifiers
  bool empty() const { return sequence.empty(); }
  bool is_enter() const { return !sequence.empty(); }
  
  string &front() { return sequence.front(); }
  const string &front() const { return sequence.front(); }
  
  int get_num_insertions(){ return n_keys; }
  
  uint size() const { return sequence.size(); }
  
  string at(uint i){
    if(i < size()){
      return sequence[i];
    }
    
    return "StringKeySequence: OUT OF BOUNDS";
  }

};

//
// ConsoleCommand --------------------------------------------------------------
//

class ConsoleCommand {
  
public:
  
  enum class IO_TYPE {
    INPUT,
    OUTPUT,
  };
  
  using time_t = std::chrono::time_point<std::chrono::system_clock>;
  
private:
  using clock = std::chrono::system_clock;
  
  // program it is attached to
  string prog_name;
  
  // static values
  static bool is_initialized;
  
  // prompt
  string prompt_main = "> ";
  string prompt_cont = "+ ";
  string prompt_color = VTS::FG_MAGENTA;
  
  string current_prompt_main = "> ";
  string current_prompt_color = prompt_color;
  void print_prompt(const bool is_main = true){
    std::cout << current_prompt_color << (is_main ? current_prompt_main : prompt_cont) << VTS::FG_DEFAULT;
  }
  size_t prompt_size(const bool is_main = true){
    return is_main ? str::utf8::count_wide_chars(current_prompt_main) : str::utf8::count_wide_chars(prompt_cont);
  }
  void print_prompt_raw(const bool is_main = true){
    std::cout << (is_main ? current_prompt_main : prompt_cont);
  }
  size_t current_prompt_size(){
    return prompt_size(cursor_str_y == 0);
  }
  
  // access to other classes
  std::map<string, std::shared_ptr<ConsoleHistory>> hist_list;
  std::shared_ptr<ConsoleHistory> phist = nullptr;
  
  std::unique_ptr<CommandState> pcmdstate = std::make_unique<CommandState>(this);
  
  std::unique_ptr<LanguageServer> pdefault_lang = std::make_unique<LanguageServer>();
  LanguageServer *plgsrv = nullptr;
  
  std::shared_ptr<ServerAutocomplete> pserver_ac = std::make_shared<ServerAutocomplete>();
  
  std::unique_ptr<ConsoleAutocomplete> pautocomp = std::make_unique<ConsoleAutocomplete>();
  
  friend class ConsoleHistory;
  friend class ConsoleAutocomplete;
  friend class ConsoleCommandSummary;
  friend class CommandState;
  
  // console handles
  static HANDLE handle_in;
  static HANDLE handle_out;
  static uint old_console_CP;
  static uint old_console_CP_output;
  static DWORD old_consModeIn;
  static DWORD old_consModeOut;
  
  // hooks
  DWORD Run_While_Reading_interval_ms = INFINITE;      // milliseconds
  void (*Run_While_Reading_fun)() = nullptr;           // function to run at some interval
  
  // lines composing the command
  vector<stringtools::string_utf8> all_lines{stringtools::string_utf8()};
  stringtools::string_utf8 *pline;
  vector<string> all_lines_fmt{UNSET::STRING};
  vector<char> all_ending_quotes{NOT_A_QUOTE};
  string inline_comment;
  
  vector<string> io_backup;
  vector<IO_TYPE> io_backup_type;
  
  CON_ACTIONS last_action = CON_ACTIONS::CREATE;
  CursorSelection selection = false;
  vector<uint> cursor_before_selection = {0, 0};
  
  string stashed_selection;
  ConsoleCommandSummary stashed_cmd;
  std::deque<ShortcutAction> stashed_shortcut_commands;
  
  // two types of cursor: within the string, and on the terminal
  uint cursor_str_x = 0;
  uint cursor_str_x_old = 0;
  uint cursor_str_y = 0;
  
  uint cursor_str_x_bak = 0;
  uint cursor_str_y_bak = 0;
  
  uint cursor_term_x = 0;
  uint cursor_term_y = 0;
  uint cursor_term_x_old = 0;
  uint cursor_term_y_old = 0;
  
  uint cursor_term_x_request = 0;
  
  uint line_height = 1;
  
  // cursor management
  void cursors_increment_str_x();
  void cursors_decrement_str_x();
  
  void cursors_increment_str_y();
  void cursors_decrement_str_y();
  
  void cursors_increment_term_x();
  void cursors_decrement_term_x();
  
  void cursors_increment_term_y();
  void cursors_decrement_term_y();
  
  void cursors_set_str_x(uint i);
  void cursors_set_str_y(uint i);
  
  void cursors_set_term_y_top();
  void cursors_set_term_y_bottom();
  
  bool cursors_is_term_y_bottom();
  
  void cursors_reset();
  void cursors_save_and_set(uint new_x, uint new_y);
  void cursors_restore();
  
  inline uint get_line_height(uint i);
  inline uint get_line_max_width(uint i);
  inline uint get_current_line_max_width();
  inline uint get_current_line_height();
  inline uint get_total_command_height();
  inline uint get_max_term_y();
  inline uint get_current_line_height_index();
  inline uint get_current_max_term_x();
  inline uint get_str_x_when_term_x_leftmost();
  inline uint get_str_x_when_term_x_rightmost();
  inline bool is_str_x_ambiguous(uint str_x = UNSET::UINT);
  void n_up_total_height_when_win_resizing(uint &n_up, uint &total_height, uint new_width);
  enum class STR_X_POSITION {TOP_RIGHT, BOTTOM_LEFT};
  STR_X_POSITION str_x_position_when_ambiguous = STR_X_POSITION::TOP_RIGHT;
  
  void update_term_cursors(bool move = true);
  void update_str_cursors();
  
  uint line_height_origin = 0;
  bool any_long_line = false;
  
  // status
  bool in_autocomp = false;
  bool in_hist_autocomp = false;
  bool in_command = false;
  bool past_command_from_sequence = false;
  bool in_sequence = false;
  
  StringKeySequence sequence;
  StringKeySequence sequence_bak;
  
  bool custom_win_width = false;
  uint win_width = 0;
  uint win_height = 0;
  
  bool command_ok = false;
  CommandToEvaluate command_result;
  
  // infobar
  uint infobar_y = 0;
  bool is_infobar = false;
  
  //
  // main functions 
  //
  
  void console_error(string);
  
  // operations on the commands
  void del(int, bool);
  void move_x(int, bool, bool, bool is_alt = false);
  void move_y(int, bool);
  void add_char(const string&, bool is_sequence = false);
  void add_line();
  void delete_all_left();
  void delete_all_right();
  void delete_current_line();
  void join_lines(int side);
  void flush_cmd(bool save = true, bool is_tmp = false);
  void clear_cmd();
  void insert_newline();
  void select_all(bool strong = false);
  void cut_selection();
  void copy_selection();
  void paste(StringKeySequence *psequence = nullptr);
  void tab();
  void escape();
  CommandToEvaluate enter();
  void undo();
  void redo();
  void selection_stash();
  void selection_pop();
  void command_stash();
  void command_pop();
  
  // autocomplete
  void run_autocomp();
  void update_autocomp(char);
  void accept_autocomp(bool is_enter = false);
  void quit_autocomp();
  str::StringMatch ac_suggestion_special_command();
  str::StringMatch ac_suggestion_special_function(const string &);
  str::StringMatch ac_suggestion_options(const string &);
  
  // print / cursor
  size_t colorize(bool paren_highlight = true);
  inline bool is_inline_comment(const string &x, int i, int n);
  void reset_all_lines();
  void print_command(bool full = false, bool paren_highlight = true, uint str_y_end_custom = UNSET::UINT);
  void print_command_grey();
  void print_command_and_delete_trailing_lines_if_needed();
  void clear_selection();
  void clear_screen();
  void clear_infobar();
  void clear_display_all_lines();
  void clear_display_below();
  
  // access
  string current_line();
  string collect();
  string collect_fmt();
  string collect_special();
  uint64_t hash();
  
  // modify
  void copy_cmd(const ConsoleCommandSummary&);
  
  //
  // special functions 
  //
  
  // timing (if asked by the user)
  bool time_all;
  time_t time_sent;
  
  // function handling the special commands
  void special_command(const string &);
  bool is_special_command() const;
  
  const string &get_last_output() const;
  const vector<string> get_all_inputs() const;
  const vector<string> get_all_outputs() const;
  
  friend class ShortcutAction;
  bool in_shortcut = false;
  std::shared_ptr<CommandToEvaluate> apply_shortuts(const ParsedShortcut &);
  std::shared_ptr<CommandToEvaluate> apply_shortuts(std::deque<ShortcutAction> &all_shortcuts);
  void apply_shortut_if(std::deque<ShortcutAction> &all_shortcuts);
  
  friend void sf_time_all(ConsoleCommand *);
  friend void sf_time_none(ConsoleCommand *);
  friend void sf_path_history(ConsoleCommand *);
  friend void sf_clear_history(ConsoleCommand *);
  friend void sf_copy_last_output(ConsoleCommand *pconcom);
  friend void sf_step_into_last_output(ConsoleCommand *pconcom);
  friend void sf_width(ConsoleCommand *, const vector<ParsedArg> &);
  friend void sf_reprex_last(ConsoleCommand *, const vector<ParsedArg> &);
  
  friend std::shared_ptr<AutocompChoices> suggest_reprex_last(ConsoleCommand *pconcom);
  
  std::map<string, SpecialFunctionInfo> all_special_functions;
  
  string command_to_send;
  
  //
  // options 
  //
  
  ProgramOptions program_opts;
  
  options_fmt_t console_options_format = {
    // language
    {"color.fun",           argtype::COLOR("pale_golden_rod")},
    {"color.var",           argtype::COLOR("#a0daef")},
    {"color.num",           argtype::COLOR("#b4ceab")},
    {"color.keyword",       argtype::COLOR("royal_blue")},
    {"color.control",       argtype::COLOR("#c676bd")},
    {"color.paren",         argtype::COLOR("term_white")},
    {"color.string",        argtype::COLOR("gold")},
    {"color.interpolation", argtype::COLOR("orange")},
    {"color.comment",       argtype::COLOR("forest_green")},
    {"color.selection_bg",       argtype::COLOR("term_blue").bg()},
    // autocomp
    {"color.autocomp_text_bg",      argtype::COLOR("term_white").bg()},
    {"color.autocomp_text_fg",      argtype::COLOR("term_bright_black")},
    {"color.autocomp_selection_bg", argtype::COLOR("term_blue").bg()},
    {"color.autocomp_selection_fg", argtype::COLOR("term_bright_white")},
    {"color.autocomp_scrollbar",    argtype::COLOR("#808080").bg()},
    // special commands
    {"color.special_command",  argtype::COLOR("medium_turquoise")},
    {"color.special_argument", argtype::COLOR("powder_blue")},
    // output
    {"color.output_highlight", argtype::COLOR("term_bright_red")},
    // reprex
    {"reprex.prompt",       argtype::STRING("#> ")},
    {"reprex.output_color", argtype::COLOR("forest_green")},
    // prompt
    {"prompt.color",    argtype::COLOR("term_default")},
    {"prompt.main",     argtype::STRING("> ")},
    {"prompt.continue", argtype::STRING("+ ")},
    // other options
    {"tab_size", argtype::INT("2")},
    {"ignore_comment", argtype::LOGICAL("true")},
    {"ignore_empty_lines", argtype::LOGICAL("true")},
    // shortcuts
    {"shortcut.alt+enter", argtype::SHORTCUT("")},
    {"shortcut.enter",  argtype::SHORTCUT("")},
    {"shortcut.ctrl+a", argtype::SHORTCUT("<select: context>")},
    {"shortcut.ctrl+b", argtype::SHORTCUT("")},
    {"shortcut.ctrl+c", argtype::SHORTCUT("<copy>")},
    {"shortcut.ctrl+d", argtype::SHORTCUT("<delete: line>")},
    {"shortcut.ctrl+e", argtype::SHORTCUT("")},
    {"shortcut.ctrl+f", argtype::SHORTCUT("")},
    {"shortcut.ctrl+g", argtype::SHORTCUT("")},
    // ctrl+h is the backspace
    {"shortcut.ctrl+i", argtype::SHORTCUT("")},
    {"shortcut.ctrl+j", argtype::SHORTCUT("")},
    {"shortcut.ctrl+k", argtype::SHORTCUT("")},
    {"shortcut.ctrl+l", argtype::SHORTCUT("<clear_screen>")},
    {"shortcut.ctrl+m", argtype::SHORTCUT("")},
    {"shortcut.ctrl+n", argtype::SHORTCUT("<newline>")},
    {"shortcut.ctrl+o", argtype::SHORTCUT("")},
    {"shortcut.ctrl+p", argtype::SHORTCUT("")},
    {"shortcut.ctrl+q", argtype::SHORTCUT("<debug>")},
    {"shortcut.ctrl+r", argtype::SHORTCUT("")},
    {"shortcut.ctrl+s", argtype::SHORTCUT("")},
    {"shortcut.ctrl+t", argtype::SHORTCUT("")},
    {"shortcut.ctrl+u", argtype::SHORTCUT("")},
    {"shortcut.ctrl+v", argtype::SHORTCUT("<paste>")},
    {"shortcut.ctrl+w", argtype::SHORTCUT("")},
    {"shortcut.ctrl+x", argtype::SHORTCUT("<cut>")},
    {"shortcut.ctrl+y", argtype::SHORTCUT("<redo>")},
    {"shortcut.ctrl+z", argtype::SHORTCUT("<undo>")},
    // auto-trimming in sequences
    {"trim_comment", argtype::STRING("")},
  };
  
  const string opt_color(const string &x) const {
    string key = "color." + x;
    if(!program_opts.has_option(key)){
      util::error_msg("The color ", str::dquote(key), " does not exist!");
    }
    return program_opts.get_color(key);
  }
  
  vector<string> lang_keywords;
  vector<string> lang_controls;
  
  friend class ResetInCommandOnLeave;
  class ResetInCommandOnLeave {
    bool in_cmd = false;
    ConsoleCommand *pconcom;
  public:
    ResetInCommandOnLeave(ConsoleCommand *pconsole): pconcom(pconsole){
      in_cmd = pconcom->in_command;
    }
    ~ResetInCommandOnLeave(){
      pconcom->in_command = in_cmd;
    }
  };

public:

  class opts {
    options_fmt_t new_options_fmt;
  public:
    opts() = default;
  
    //
    // options_fmt
    //
    
    opts& set_options_format(const options_fmt_t &x){
      new_options_fmt = x;
      return *this;
    }
    
    options_fmt_t get_options_format() const { return new_options_fmt; }
    bool is_options_fmt() const { return !new_options_fmt.empty(); }
    
  };
  
  ConsoleCommand() = default;
  ConsoleCommand(string program_name, const opts options = opts());
  ~ConsoleCommand();
  
  // 
  // setup
  //
  
  void initialize(string program_name, const opts options = opts());
  
  void setup_lgsrv(LanguageServer* x);
  
  void setup_srvautocomp(std::shared_ptr<ServerAutocomplete> ptr){ pserver_ac = ptr; }
  
  void setup_Run_While_Reading(void (*fun)(), DWORD interval_ms = 35){
    Run_While_Reading_fun = fun;
    Run_While_Reading_interval_ms = interval_ms;
  }
  
  void setup_language_keywords(const vector<string> &x){
    lang_keywords = x;
  }
  
  void setup_language_controls(const vector<string> &x){
    lang_controls = x;
  }
  
  void setup_inline_comment(const string &x){
    inline_comment = x;
  }
  
  void setup_special_functions(const std::map<string, SpecialFunctionInfo> &funs){
    util::map_add_entries(all_special_functions, funs);
  }
  
  void setup_ignored_hist_cmd(const vector<string> &x){
    for(auto &phist : hist_list){
      phist.second->set_ignored_cmd(x);
    }
  }
  
  //
  // options 
  //
  
  using op_write_t = ProgramOptions::TYPE;
  const ParsedArg& set_program_option(const string &key, const string &value, 
                                         const op_write_t type = op_write_t::TEMP);
  
  const ParsedArg& set_program_option(const string &key, bool value, const op_write_t type = op_write_t::TEMP){
    return set_program_option(key, str::bool_to_string(value), type);
  }
  
  const ParsedArg& set_program_option(const string &key, int value, const op_write_t type = op_write_t::TEMP){
    return set_program_option(key, std::to_string(value), type);
  }
  
  const ParsedArg& set_program_option(const string &key, size_t value, const op_write_t type = op_write_t::TEMP){
    return set_program_option(key, std::to_string(value), type);
  }
  
  const ParsedArg& set_program_option(const string &key, fs::path value, const op_write_t type = op_write_t::TEMP){
    return set_program_option(key, value.string(), type);
  }
  
  const ParsedArg& get_program_option(const string &key, 
                                         const util::DoCheck options = util::DoCheck(false)) const;
  
  const string get_path_options() const {
    return program_opts.get_global_path();
  }
  
  void set_command_to_send(const string &cmd){
    command_to_send = cmd;
  }
  
  //
  // real public functions
  //
  
  CommandToEvaluate read_command(bool is_command = true, 
                                 string hist_name = "main", 
                                 string prompt = UNSET::STRING,
                                 bool was_error = false);
  
  string read_line(const ReadOptions &);
  string read_line();
  
  void write_output(const string &x, bool highlight = false);
  
  uint window_width(){ return win_width; }
  
  void init_command(){ command_ok = false; }
  CommandToEvaluate get_command(){ return command_result; };
  void insert_newline_if_needed_to_be_leftmost() const;
  
  void infobar(const string&);
  
  std::mutex mut_input;
  std::mutex mut_output;
};












