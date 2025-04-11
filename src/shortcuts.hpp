
#pragma once

#include "util.hpp"
#include "stringtools.hpp"
#include "autocomplete.hpp"

#include <deque>

namespace str = stringtools;

extern const std::map<string, vector<string>> shortcut_commands;
extern const std::map<string, string> shortcut_values_with_freeform;
extern const AutocompChoices shortcut_cmd_sugg_closed;
extern const AutocompChoices shortcut_cmd_sugg_open;

class ConsoleCommand;
class CommandToEvaluate;

AutocompChoices gen_shortcut_condition_suggestions(bool already_closed, bool is_first, bool opening_if);

//
// ShortcutAction -------------------------------------------------------------- 
//

class ShortcutAction {
  
  enum class TYPE {
    COMMAND,
    CONDITION,
    UNSET,
  };
  
  TYPE type = TYPE::UNSET;
  
  string cmd_name;
  string cmd_value;
  string cmd_freeform;
  
public:
  
  ShortcutAction() = default;
  ShortcutAction(const string &cmd, const string &value, const string &freeform = "");
  
  bool is_if() const { return str::starts_with(cmd_name, "if"); }
  bool is_endif() const { return cmd_name == "endif"; }
  bool is_condition() const { return type == TYPE::CONDITION; }
  bool is_and_or() const { return type == TYPE::CONDITION && str::starts_with(cmd_name, {"and", "or"}); }
  bool is_and() const { return type == TYPE::CONDITION && str::starts_with(cmd_name, "and"); }
  bool is_or() const { return type == TYPE::CONDITION && str::starts_with(cmd_name, "or"); }
  bool is_else() const { return str::starts_with(cmd_name, "else"); }
  bool is_else_no_if() const { return cmd_name == "else"; }
  bool is_else_if() const { return str::starts_with(cmd_name, "else if"); }
  bool is_else_endif() const { return is_else() || is_endif(); }
  
  string get_raw_command() const;
  
  std::shared_ptr<CommandToEvaluate> run_command(ConsoleCommand *pconcom) const;
  bool is_condition_verified(ConsoleCommand *pconcom) const;
  
};


//
// ParsedShortcut -------------------------------------------------------------- 
//

class ParsedShortcut {
public:
  enum class SUGGEST_TYPE {
    DEFAULT,
    COMMAND,
    VALUE,
    CONDITION_FIRST,
    CONDITION,
    FREEFORM_STRING,
    FREEFORM_INT,
  };
  
private:
  bool _is_valid = false;
  bool _is_valid_context = false;
  string error;
  std::deque<ShortcutAction> all_commands;
  
  // the information below is used in the autocomplete
  string cmd;
  string value;
  string freeform;
  size_t i_start_command = 0;
  size_t i_context = 0;
  bool is_first_condition = false;
  SUGGEST_TYPE sugg_type = SUGGEST_TYPE::DEFAULT;
  
public:
  
  ParsedShortcut() = default;
  ParsedShortcut(const string &x);
  
  bool is_valid() const { return _is_valid; }
  bool is_valid_context() const { return _is_valid_context; }
  string get_error() const { return error; }
  
  std::deque<ShortcutAction> get_all_commands() const { return all_commands; };
  
  bool empty() const { return all_commands.empty(); }
  
  string get_cmd() const { return cmd; }
  string get_value() const { return value; }
  string get_freeform() const { return freeform; }
  size_t get_i_context() const { return i_context; }
  bool get_is_first_condition() const { return is_first_condition; }
  SUGGEST_TYPE get_sugg_type() const { return sugg_type; }
  string get_sugg_type_verbose() const {
    switch(sugg_type){
      case SUGGEST_TYPE::DEFAULT: return "default";
      case SUGGEST_TYPE::COMMAND: return "command";
      case SUGGEST_TYPE::VALUE: return "value";
      case SUGGEST_TYPE::CONDITION_FIRST: return "condition_first";
      case SUGGEST_TYPE::CONDITION: return "condition";
      case SUGGEST_TYPE::FREEFORM_STRING: return "freeform_string";
      case SUGGEST_TYPE::FREEFORM_INT: return "freeform_int";
    }
    return "error";
  }
  
  string err_shorcut(const string &x, int i) const;
};


void print(std::deque<ShortcutAction> all_cmd);


