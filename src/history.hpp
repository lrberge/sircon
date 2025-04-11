    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "util.hpp"
#include "stringtools.hpp"
#include "constants.hpp"
#include "VTS.hpp"
#include "console_util.hpp"

using std::vector;
using std::string;

namespace str = stringtools;

using uint = unsigned int;

class ConsoleCommand;


//
// ConsoleCommandSummary ------------------------------------------------------- 
//

// NOTA: you need a default constructor, otherwise classes that use ConsoleCommandSummary
// cannot be initialized via the default constructor
class ConsoleCommandSummary {
  enum class TYPE {
    SET,
    UNSET,
  };
  
  TYPE type = TYPE::UNSET;
  
public:
  // the elements that vary across commands in the history list
  vector<stringtools::string_utf8> all_lines{stringtools::string_utf8()};
  vector<string> all_lines_fmt{UNSET::STRING};
  vector<char> all_ending_quotes{NOT_A_QUOTE};
  string command_short;
  
  CursorSelection selection;
  
  uint cursor_str_x = 0;
  uint cursor_str_y = 0;
  
  uint64_t hash = 0;
  
  ConsoleCommandSummary() = default;
  ConsoleCommandSummary(ConsoleCommand *pconcom);
  ConsoleCommandSummary(const vector<string> &x);
  
  string current_line() const {
    return all_lines.at(cursor_str_y).str();
  }
  
  bool is_unset() const { return type == TYPE::UNSET; }
};

//
// HistoryLookup ---------------------------------------------------------------
//

class HistoryLookup {
public:
  bool is_tmp = true;
  uint idx = 0;
  HistoryLookup() = default;
  HistoryLookup(bool b, uint i): is_tmp(b), idx(i) {};
};


//
// ConsoleHistory --------------------------------------------------------------
//

class ConsoleHistory {
  vector<ConsoleCommandSummary> past_commands;
  vector<ConsoleCommandSummary> tmp_commands;
  std::map<uint64_t, uint> cmd_index;
  
  // the variable index refers to a position in the *lookup* table
  // => not in the past_commands
  uint index = 0;
  
  uint index_hist_max = UNSET::UINT;
  
  ConsoleCommandSummary current_cmd;
  vector<HistoryLookup> lookup{HistoryLookup()};
  
  bool is_fresh = true;
  
  void write_history();
  
  static fs::path hist_path;
  
  bool is_main_hist = false;
  
  vector<string> ignored_cmd;
  
  ConsoleCommand *pconcom = nullptr;
  
public:
  
  ConsoleHistory() = delete;
  ConsoleHistory(ConsoleCommand *pcon, const string program_name = UNSET::STRING);
  
  void navigate(int, bool&, bool);
  
  void add_command(bool is_tmp = false);
  
  void append_history_line();
  
  vector<string> get_past_commands() const {
    vector<string> res;
    for(auto &cmd: past_commands){
      res.push_back(cmd.command_short);
    }
    return res; 
  }
  
  ConsoleCommandSummary get_command_at(uint i) const {
    if(util::is_out_of_bounds(past_commands, i, "ConsoleCommandSummary.get_command_at")){
      return ConsoleCommandSummary();
    }
    
    return past_commands[i];
  }
  
  fs::path get_history_path() const {
    return hist_path;
  }
  
  void set_as_main_history(){
    is_main_hist = true;
  }
  
  bool is_main() const {
    return is_main_hist;
  }
  
  void set_ignored_cmd(const vector<string> &x){
    ignored_cmd = x;
  }
  
  bool is_ignored_cmd(const vector<str::string_utf8> &all_lines) const {
    return all_lines.size() > 1 ? false : (util::vector_contains(ignored_cmd, all_lines[0].str()));
  }
  
};


//
// CommandState ---------------------------------------------------------------- 
//

class CommandState {
  
  using time_t = std::chrono::time_point<std::chrono::system_clock>;
  
  const ConsoleCommandSummary empty_cmd{vector<string>{""}};
  vector<ConsoleCommandSummary> all_states = {empty_cmd};
  ConsoleCommand *pconcom = nullptr;
  time_t time_last_add = UNSET::TIME;
  int n_undo = 0;
  bool record_state = true;
  
  void set_state(const ConsoleCommandSummary &cmd);
  
public:
  
  CommandState(ConsoleCommand *px): pconcom(px){}
  
  void add_state();
  void undo();
  void redo();
  void clear();
  void clear(const ConsoleCommandSummary &cmd);
      
};





