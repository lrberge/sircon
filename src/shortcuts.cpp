    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//



#include "shortcuts.hpp"
#include "console.hpp"

//
// constants ------------------------------------------------------------------- 
//


static const vector<string> if_conditions = {
  "empty", "line_empty", "line_matches", "one_liner", 
  "is_letter_left", "is_letter_right", "is_punct_left", "is_punct_right",
  "any_selection", "y_top", "y_bottom", 
  "x_leftmost", "x_rightmost"
};

const vector<string> condition_commands = {
  "if", "if not", "and if", "and if not", "or if", "or if not", "else if",
  "else if not", "else", "endif"
};

const std::map<string, vector<string>> shortcut_commands = {
  {"select", {"all", "context"}},
  {"move_x", {"left", "right", "leftmost", "rightmost", "word_left", "word_right"}},
  {"move_y", {"down", "up", "top", "bottom"}},
  {"delete", {"left", "right", "line", "word_left", "word_right", "all_left", "all_right"}},
  {"selection", {"stash", "pop"}},
  {"command", {"stash", "pop", "clear"}},
  {"copy", {}},
  {"paste", {}},
  {"cut", {}},
  {"undo", {}},
  {"redo", {}},
  {"newline", {}},
  {"insert", {}},
  {"enter", {}},
  {"run", {}},
  {"run_no_echo", {}},
  {"clear_screen", {}},
  {"debug", {}},
  // if related
  {"if", if_conditions},
  {"else if", if_conditions},
  {"else", {}},
  {"endif", {}},
};

const std::map<string, string> shortcut_values_with_freeform = {
  {"left", "int"},
  {"right", "int"},
  {"word_left", "int"},
  {"word_right", "int"},
  {"up", "int"},
  {"down", "int"},
  {"insert", "string"},
  {"run", "string"},
  {"run_no_echo", "string"},
  {"line_matches", "string"},
};

static AutocompChoices gen_shortcut_command_suggestions(bool already_closed){
  
  AutocompChoices choices;
  
  vector<string> all_cmd = util::map_names(shortcut_commands);
            
  const size_t n = all_cmd.size();
  vector<string> cursor_shift(n);
  vector<string> append_right(n);
  vector<string> do_continue(n);
  
  int i = -1;
  for(auto &cmd : all_cmd){
    ++i;
    if(util::map_contains(shortcut_values_with_freeform, cmd)){
      
      const string &type = shortcut_values_with_freeform.at(cmd);
      if(type == "string"){
        cmd += ": \"\"";
        cursor_shift[i] = already_closed ? "-1" : "-2";
      } else {
        cmd += ": ";
      }
      
      if(!already_closed){
        append_right[i] = ">";
      }
      
    } else if(shortcut_commands.at(cmd).empty()){
      if(!already_closed){
        append_right[i] = ">";
      }
      
    } else {
      cmd += ": ";
      do_continue[i] = "true";
    }
  }
  
  choices = all_cmd;
  choices.set_meta("cursor_shift", cursor_shift);
  choices.set_meta("append_right", append_right);
  choices.set_meta("continue", do_continue);
  
  return choices;
}

const AutocompChoices shortcut_cmd_sugg_closed = gen_shortcut_command_suggestions(true);
const AutocompChoices shortcut_cmd_sugg_open = gen_shortcut_command_suggestions(false);

AutocompChoices gen_shortcut_condition_suggestions(bool already_closed, bool is_first, bool opening_if){
  
  AutocompChoices choices;
  
  vector<string> all_conditions = if_conditions;
  
  if(!is_first){
    all_conditions.push_back("and");
    all_conditions.push_back("or");
  }
  
  std::sort(all_conditions.begin(), all_conditions.end());
  
  if(already_closed){
    opening_if = false;
  }
  
  const size_t n = all_conditions.size();
  vector<string> cursor_shift(n);
  vector<string> append_right(n);
  
  int i = -1;
  for(auto &cond : all_conditions){
    ++i;
    
    if(util::map_contains(shortcut_values_with_freeform, cond)){
      
      cond += ": \"\"";
      
      if(opening_if){
        append_right[i] = ">  <endif>";
        cursor_shift[i] = "-11";
        
      } else if(!already_closed){
        append_right[i] = ">";
        cursor_shift[i] = "-2";
      } else {
        cursor_shift[i] = "-1";
      }
      
    } else {
      if(opening_if){
        append_right[i] = ">  <endif>";
        cursor_shift[i] = "-8";
        
      } else if(!already_closed){
        append_right[i] = ">";
      }
    }
  }
  
  choices = all_conditions;
  choices.set_meta("cursor_shift", cursor_shift);
  choices.set_meta("append_right", append_right);
  choices.set_meta("append_left", " ");
  
  return choices;
}

//
// ShortcutAction 
//


ShortcutAction::ShortcutAction(const string &cmd, const string &value, const string &freeform):
  cmd_name(cmd), cmd_value(value), cmd_freeform(freeform){
  
  if(util::vector_contains(condition_commands, cmd)){
    type = TYPE::CONDITION;
  } else {
    type = TYPE::COMMAND;
  }
}

string ShortcutAction::get_raw_command() const { 
  string res = "<" + cmd_name;
  if(!cmd_value.empty()){
    res += ": " + cmd_value;
    if(!cmd_freeform.empty()){
      res += ": " + cmd_freeform;
    }
  }
  res += ">";
  
  return res;
}


std::shared_ptr<CommandToEvaluate> ShortcutAction::run_command(ConsoleCommand *pconcom) const{
  
  std::shared_ptr<CommandToEvaluate> pres = std::make_shared<CommandToEvaluate>();
  
  if(type == TYPE::UNSET){
    util::error_msg("Internal error: ShortcutAction: trying to run a shortcut that has not been set.");
    return pres;
  }
  
  if(type != TYPE::COMMAND){
    util::error_msg("Internal error: ShortcutAction: run_command() can only be used for commands.\n",
                    "For conditions, use check_condition().");
    return pres;
  }
  
  if(cmd_name == "select"){
    pconcom->select_all(cmd_value != "context");
    
  } else if(cmd_name == "move_x"){
    
    size_t n = 0;
    if(!cmd_freeform.empty()){
      n = std::stoi(cmd_freeform);
    }
    
    if(cmd_value == "left" || cmd_value == "word_left"){
      
      bool is_ctrl = cmd_value == "word_left";
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->move_x(SIDE::LEFT, is_ctrl, false);
      }
      
    } else if(cmd_value == "right" || cmd_value == "word_right"){
      
      bool is_ctrl = cmd_value == "word_right";
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->move_x(SIDE::RIGHT, is_ctrl, false);
      }
      
    } else if(cmd_value == "leftmost"){
      pconcom->move_x(SIDE::LEFTMOST, false, false);
      
    } else if(cmd_value == "rightmost"){
      pconcom->move_x(SIDE::RIGHTMOST, false, false);
      
    } else {
      util::error_msg("Internal error: ShortcutAction: for command ", str::dquote(cmd_name), 
                      ", unknonw value ", cmd_value);
      return pres;
    }
    
  } else if(cmd_name == "move_y"){
    
    size_t n = 0;
    if(!cmd_freeform.empty()){
      n = std::stoi(cmd_freeform);
    }
    
    if(cmd_value == "up"){
      
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->move_y(SIDE::UP, false);
      }
      
    } else if(cmd_value == "top"){
      pconcom->move_y(SIDE::UP, true);
      
    } else if(cmd_value == "down"){
      
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->move_y(SIDE::DOWN, false);
      }
      
    } else if(cmd_value == "bottom"){
      pconcom->move_y(SIDE::DOWN, true);
      
    } else {
      util::error_msg("Internal error: ShortcutAction: for command ", str::dquote(cmd_name), 
                      ", unknonw value ", cmd_value);
      return pres;
    }
    
  } else if(cmd_name == "delete"){
    
    size_t n = 0;
    if(!cmd_freeform.empty()){
      n = std::stoi(cmd_freeform);
    }
    
    if(cmd_value == "left"){
      
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->del(SIDE::LEFT, false);
      }
      
    } else if(cmd_value == "right"){
      
      for(size_t i = 0 ; i < n ; ++i){
        pconcom->del(SIDE::LEFT, false);
      }
      
    } else if(cmd_value == "word_left"){
      pconcom->del(SIDE::LEFT, true);
      
    } else if(cmd_value == "word_right"){
      pconcom->del(SIDE::RIGHT, true);
      
    } else if(cmd_value == "line"){
      pconcom->delete_current_line();
      
    } else if(cmd_value == "all_left"){
      pconcom->delete_all_left();
      
    } else if(cmd_value == "all_right"){
      pconcom->delete_all_right();
      
    } else {
      util::error_msg("Internal error: ShortcutAction: for command ", str::dquote(cmd_name), 
                      ", unknonw value ", cmd_value);
      return pres;
    }
    
  } else if(cmd_name == "selection"){
    
    if(cmd_value == "stash"){
      pconcom->selection_stash();
      
    } else if(cmd_value == "pop"){
      pconcom->selection_pop();
      
    } else {
      util::error_msg("Internal error: ShortcutAction: for command ", str::dquote(cmd_name), 
                      ", unknonw value ", cmd_value);
      return pres;
    }
    
  } else if(cmd_name == "command"){
    
    if(cmd_value == "stash"){
      pconcom->command_stash();
      
    } else if(cmd_value == "pop"){
      pconcom->command_pop();
      
    } else if(cmd_value == "clear"){
      pconcom->reset_all_lines();
      pconcom->print_command();
      
    } else {
      util::error_msg("Internal error: ShortcutAction: for command ", str::dquote(cmd_name), 
                      ", unknonw value ", cmd_value);
      return pres;
    }
    
  } else if(cmd_name == "copy"){
    pconcom->copy_selection();
    
  } else if(cmd_name == "paste"){
    pconcom->paste();
    
  } else if(cmd_name == "cut"){
    pconcom->cut_selection();
    
  } else if(cmd_name == "undo"){
    pconcom->undo();
    
  } else if(cmd_name == "redo"){
    pconcom->redo();
    
  } else if(cmd_name == "newline"){
    pconcom->insert_newline();
    
  } else if(cmd_name == "insert"){
    pconcom->add_char(cmd_value, true);
    
  } else if(cmd_name == "enter"){
    *pres = pconcom->enter();
    
  } else if(cmd_name == "run"){
    // we stash the current command
    pconcom->command_stash();
    *pres = CommandToEvaluate(cmd_value, true, false);
    pconcom->add_char(cmd_value, true);
    pconcom->print_command(false, false);
    pconcom->flush_cmd(false, false);
    
  } else if(cmd_name == "run_no_echo"){
    // we stash the current command
    pconcom->command_stash();
    std::cout << VTS::cursor_move_at_x(0) << VTS::CLEAR_LINE;
    *pres = CommandToEvaluate(cmd_value, true, false);
    
  } else if(cmd_name == "clear_screen"){
    pconcom->clear_screen();
    
  } else if(cmd_name == "debug"){
    util::next_debug_type();
    
  }
  
  return pres;
}

bool ShortcutAction::is_condition_verified(ConsoleCommand *pconcom) const {
  
  if(type == TYPE::UNSET){
    util::error_msg("Internal error: ShortcutAction: trying to run a shortcut that has not been set.");
    return false;
  }
  
  if(type != TYPE::CONDITION){
    util::error_msg("Internal error: ShortcutAction: is_condition_verified() can only be used for conditions.\n",
                    "For commands, use run_command().");
    return false;
  }
  
  if(util::vector_contains({"else", "endif"}, cmd_name)){
    util::error_msg("Internal error: ShortcutAction: is_condition_verified() can only be used for conditions.\n",
                    "It should not be run for `", cmd_name, "`.");
    return false;
  }
  
  bool res = false;
  
  if(cmd_value == "empty"){
    res = (pconcom->all_lines.size() == 1) && (pconcom->pline->empty());
    
  } else if(cmd_value == "line_empty"){
    res = pconcom->pline->empty();
    
  } else if(cmd_value == "line_matches"){
    const string &pattern = cmd_freeform;
    const string line = pconcom->pline->str();
    const unsigned int cx = pconcom->cursor_str_x;
    
    if(str::str_contains(pattern, "_cursor_")){
      const string pat_left = str::delete_after(pattern, "_cursor_");
      const string pat_right = str::delete_until(pattern, "_cursor_");
      
      res = true;
      if(!pat_left.empty()){
        if(cx > 0){
          string line_left = pconcom->pline->substr(0, cx);
          res = str::ends_with(line_left, pat_left);
        } else {
          res = false;
        }
      }
      
      if(res && !pat_right.empty()){
        const size_t n = pconcom->pline->size();
        if(cx < n){
          string line_right = pconcom->pline->substr(cx, n - cx);
          res = str::starts_with(line_right, pat_right);
        } else {
          res =false;
        }
        
      }
      
    } else {
      
      res = str::str_contains(line, pattern);
      
    }
    
  } else if(cmd_value == "one_liner"){
    res = pconcom->all_lines.size() == 1;
    
  } else if(cmd_value == "is_letter_left" || cmd_value == "is_punct_left"){
    const string &line = pconcom->pline->str();
    const size_t pos = pconcom->cursor_str_x;
    
    int i = pos - 1;
    str::move_i_to_non_WS_if_i_WS(line, i, SIDE::LEFT);
    
    bool is_punct = false;
    if(i >= 0){
      is_punct = str::is_control_char(line[i]);
    } else {
      return false;
    }
    
    return cmd_value == "is_punct_left" ? is_punct : !is_punct;
    
  } else if(cmd_value == "is_letter_right" || cmd_value == "is_punct_right"){
    const string &line = pconcom->pline->str();
    const size_t pos = pconcom->cursor_str_x;
    
    int i = pos;
    str::move_i_to_non_WS_if_i_WS(line, i, SIDE::RIGHT);
    
    bool is_punct = false;
    if(i < static_cast<int>(line.size())){
      is_punct = str::is_control_char(line[i]);
    } else {
      return false;
    }
    
    return cmd_value == "is_punct_right" ? is_punct : !is_punct;
    
  } else if(cmd_value == "any_selection"){
    res = pconcom->selection.is_selection();
    
  } else if(cmd_value == "y_top"){
    res = pconcom->cursor_str_y == 0;
    
  } else if(cmd_value == "y_bottom"){
    res = pconcom->cursor_str_y == pconcom->all_lines.size();
    
  } else if(cmd_value == "x_leftmost"){
    res = pconcom->cursor_str_x == 0;
    
  } else if(cmd_value == "x_rightmost"){
    res = pconcom->cursor_str_x == pconcom->pline->size();
    
  }
  
  if(str::str_contains(cmd_name, "not")){
    res = !res;
  }
  
  return res;
}


//
// PasredShortcut -------------------------------------------------------------- 
//

/* Shortcut syntax:
* 
* There are two modes: 
* 1) command
* 2) insertion
* 
* Commands are written inside <>, like "<enter>"
* 
* To insert, use text directly. Special values:
* - _all_: the full line
* - _sel_: the current selection
* 
* By default spaces before/after are trimmed. To insert spaces, use quotes:
* - ex: " %in% " vs %in%. In the latter, no spaces will be inserted.
* 
* - insertion are trasnformed into command sequences
* 
* */

string ParsedShortcut::err_shorcut(const string &x, int i) const {
  string res = x;
  res.insert(i, util::FG_ERROR);
  res.insert(i_start_command, VTS::all_html_colors.at("deep_pink"));
  return str::bquote(res);
}

ParsedShortcut::ParsedShortcut(const string &x){
  
  const int n = x.size();
  int i = 0;
  while(i < n){
    
    sugg_type = SUGGEST_TYPE::DEFAULT;
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
    
    if(i >= n){
      break;
    }
    
    i_start_command = i;

    if(x[i] == '<'){
      i_context = i + 1;
      cmd.clear();
      value.clear();
      freeform.clear();
      
      ++i;
      while(i < n && x[i] != '>' && x[i] != ':'){
        cmd += x[i++];
      }
      
      if(i == n){
        error = util::txt(
          "The current shortcut is invalid. Each shortcut command should be contained within \"<>\".\n",
          "Problem: in the shortcut ", err_shorcut(x, i), "\n",
          "=> the command ", str::dquote(cmd), " does not have a closing \">\"");
          sugg_type = SUGGEST_TYPE::COMMAND;
        return;
      }
      
      str::trim_WS_inplace(cmd);
      
      if(cmd.empty()){
        error = util::txt(
          "The current shortcut is invalid. Each shortcut command should be contained within \"<>\".\n",
          "Problem: in the shortcut ", err_shorcut(x, i), "\n",
          "=> the command is empty");
        sugg_type = SUGGEST_TYPE::COMMAND;
        return;
      }
      
      
      //
      // if 
      //
      
      // Accepted syntax:
      // c1 or not c2 and c3
      // => conditions cannot be combined with parentheses
      // keywords: or, and, not
      // 
      
      if(str::is_string_in(cmd, {"if", "else if"})){
        
        if(x[i] != ':'){
          error = util::txt(
            "The current shortcut is invalid. The command 'if' must be followed with a condition.\n",
            "ex: <if: one_liner and not x_rightmost> \n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> there is no condition associated to the if");
          sugg_type = SUGGEST_TYPE::COMMAND;
          return;
        }
        
        // we go past the ':'
        ++i;
        
        i_context = i;
        
        // we parse on the fly
        is_first_condition = true;
        bool first = true;
        bool is_not = false;
        bool is_and = false;
        bool is_or = false;
        while(i < n && x[i] != '>'){
          
          string kw = str::extract_word(x, i);
          
          if(kw == "not"){
            is_not = true;
            i_context = i;
            
          } else if(kw == "and" || kw == "or"){
            if(first){
              error = util::txt(
                "The current shortcut is invalid. In `if`: the operator ", str::bquote(kw), " is used to combine conditions.\n",
                "ex: <if: one_liner ", kw, " not x_rightmost> \n",
                "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                "=> the operator ", kw, " cannot be placed before the first condition");
              sugg_type = SUGGEST_TYPE::CONDITION_FIRST;
              return;
            }
            
            is_and = kw == "and";
            is_or = kw == "or";
            i_context = i;
            
          } else if(str::is_string_in(kw, if_conditions)){
            // OK. We apply a last check though
            
            if(!first && !is_and && !is_or){
              error = util::txt(
                "The current shortcut is invalid. In `if`: to combine different conditions, you need to use `and` or `or`.\n",
                "ex: <if: one_liner and x_rightmost> \n",
                "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                "=> use `and` or `or`");
              sugg_type = SUGGEST_TYPE::CONDITION;
              return;
            }
            
            // does it accept a freeform argument?
            string if_freeform;
            if(util::map_contains(shortcut_values_with_freeform, kw)){
              str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
              if(i < n && x[i] == ':'){
                // we skip
                ++i;
                str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
              }
              
              i_context = i;
              
              // we extract the quotes!
              if(i == n || x[i] != '"'){
                error = util::txt(
                  "The current shortcut is invalid. In `if`: the condition ", str::bquote(kw), " must be followed with a value within double quotes.\n",
                  "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                  "=> no double quotes found following ", str::bquote(kw));
                sugg_type = first ? SUGGEST_TYPE::CONDITION_FIRST : SUGGEST_TYPE::CONDITION;
                return;
              }
              
              ++i;
              if_freeform = str::extract_until_next_unescaped_char(x, i, '"');
              
              if(i == n){
                error = util::txt(
                  "The current shortcut is invalid. Problem in `if`: to condition ", str::bquote(kw), " must be followed with a value within double quotes.\n",
                  "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                  "=> the quotes following ", str::bquote(kw), " are not closed");
                sugg_type = SUGGEST_TYPE::FREEFORM_STRING;
                return;
              }
              
              ++i;
              
            }
            
            i_context = i;
            
            string if_cmd = first ? cmd : (is_and ? "and if" : "or if");
            if(is_not){
              if_cmd += " not";
            }
            
            all_commands.emplace_back(if_cmd, kw, if_freeform);
            
            first  = false;
            is_not = false;
            is_and = false;
            is_or  = false;
            is_first_condition = false;
            
          } else {
            error = util::txt(
              "Problem: in the shortcut ", err_shorcut(x, i), "\n",
              "In `if`: the condition ", str::bquote(kw), " is invalid.\n",
              "PYI, the valid conditions are: ", str::enumerate(if_conditions, str::EnumOpts().bquote()));
            sugg_type = first ? SUGGEST_TYPE::CONDITION_FIRST : SUGGEST_TYPE::CONDITION;
            return;
          }
          
        }
        
        if(i == n){
          error = util::txt(
            "The current shortcut is invalid. Each shortcut command should be contained within \"<>\".\n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> the command ", str::dquote(cmd), " does not have a closing \">\"");
          sugg_type = SUGGEST_TYPE::CONDITION;
          return;
        }
        
        // we skip '>'
        ++i;
        continue;
        
      }
      
      
      //
      // regular commands ------------------------------------------------------
      //
      
      if(!util::map_contains(shortcut_commands, cmd)){
        error = util::txt(
          "The current shortcut is invalid.\n",
          "Problem: in the shortcut ", err_shorcut(x, i), "\n",
          "=> the command ", str::dquote(cmd), " does not exist");
        
        str::StringMatch sugg = str::string_match(cmd, util::map_names(shortcut_commands));
        if(!sugg.empty()){
          error += "\nDid you mean " + str::enumerate(sugg.get_matches(), str::EnumOpts().add_or().bquote()) + "?";
        }
        
        sugg_type = SUGGEST_TYPE::COMMAND;
        return;
      }
      
      const vector<string> all_valid_values = shortcut_commands.at(cmd);
      
      //
      // cmd + freeform option
      //
      
      if(util::map_contains(shortcut_values_with_freeform, cmd)){
        // value is the freeform
        // ex:
        // - <insert: "hello">
        //
        
        const string &type = shortcut_values_with_freeform.at(cmd);
        
        if(x[i] != ':'){
          
          if(type == "int"){
            // default value
            sugg_type = SUGGEST_TYPE::FREEFORM_INT;
            freeform = "1";
            
          } else {
            error = util::txt(
              "The current shortcut is invalid. The command `", cmd, "` must be of the form \n",
              "\"<", cmd, ": ", type == "string" ? str::dquote("string") : "int", ">\".",
              "Problem: in the shortcut ", err_shorcut(x, i), "\n",
              "=> there is no ':' following the command");
            sugg_type = SUGGEST_TYPE::COMMAND;
            return;
          }
          
        } else {
          // freeform + is after ':'
          
          ++i;
          str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
          i_context = i;
          
          if(type == "string"){
            // NOTA: we accept empty strings
            sugg_type = SUGGEST_TYPE::FREEFORM_STRING;
            
            // we extract the quotes!
            if(i == n || x[i] != '"'){
              error = util::txt(
                "The current shortcut is invalid. The command `", cmd, "` must be of the form \n",
              "\"<", cmd, ": ", str::dquote("string"), ">\".",
              "Problem: in the shortcut ", err_shorcut(x, i), "\n",
              "=> there is no starting quote");
              return;
            }
            
            ++i;
            value = str::extract_until_next_unescaped_char(x, i, '"');
            
            if(i == n){
              error = util::txt(
                "The current shortcut is invalid. The command `", cmd, "` must be of the form \n",
                "\"<", cmd, ": ", str::dquote("string"), ">\".",
                "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                "=> the quotes following ", str::bquote(cmd), " are not closed");
              return;
            }
            
            ++i;
            str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
            
            // last check: no newline => can lead to severe bugs
            if(str::str_contains(value, '\n')){
              error = util::txt(
                "The current shortcut is invalid. The command `", cmd, "` shall not contain newlines.",
                "Problem: in the shortcut ", err_shorcut(str::ascii_printable(x), i), "\n",
                "=> the value for ", str::bquote(cmd), " (", str::ascii_printable(value), "), contains a newline");
              return;
            }
            
          } else if(type == "int"){
            sugg_type = SUGGEST_TYPE::FREEFORM_INT;
            
            value = str::extract_until_next_unescaped_char(x, i, '>');
            value = str::trim_WS(value);
            
            if(!str::is_int_inside(value)){
              error = util::txt(
                "The current shortcut is invalid. The command `", cmd, "` must be of the form \n",
                "\"<", cmd, ": int>\".",
                "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                "=> the integer in `", value, "` could not be parsed");
              return;
            }
            
          }
        }
        
      } else if(all_valid_values.empty()){
        //
        // cmd + no option
        //
        
        sugg_type = SUGGEST_TYPE::COMMAND;
        if(x[i] == ':'){
          error = util::txt(
            "The current shortcut is invalid. The shortcut ", cmd, " does not acceept options.\n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> option found for ", cmd, " (", value, "). Simply write <", cmd, "> instead.");
          return;
        }
        
      } else {
        
        //
        // cmd + option
        //
        
        
        // the command is valid and it accepts options
        // we parse the option
        
        if(x[i] != ':'){
          error = util::txt(
            "The current shortcut is invalid. \n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> the command ", cmd, " has no option.\n",
            "FYI, the valid options are: ", str::enumerate(all_valid_values, str::EnumOpts().bquote()));
          sugg_type = SUGGEST_TYPE::COMMAND;
          return;
        }
        
        ++i;
        i_context = i;
        
        value = str::extract_word(x, i);
        
        sugg_type = SUGGEST_TYPE::VALUE;
        if(!util::vector_contains(all_valid_values, value)){
          error = util::txt(
            "The current shortcut is invalid. \n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> the command ", cmd, " has no option", (value.empty() ? "" : str::bquote(value)), ".\n",
            "FYI, the valid options are: ", str::enumerate(all_valid_values, str::EnumOpts().bquote()));
          return;
        }
        
        // it this a freeform value?
        str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
        
        if(util::map_contains(shortcut_values_with_freeform, value)){
          
          //
          // cmd + option + freeform
          //
          
          const string &type = shortcut_values_with_freeform.at(value);
          
          string cmd_value = cmd + ": " + value;
          
          if(x[i] != ':'){
            
            if(type == "int"){
              // default value
              sugg_type = SUGGEST_TYPE::FREEFORM_INT;
              freeform = "1";
              
            } else {
              error = util::txt(
                "The current shortcut is invalid. The command `", cmd_value, "` must be of the form \n",
                "\"<", cmd_value, ": ", type == "string" ? str::dquote("string") : "int", ">\".",
                "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                "=> there is no ':' following the command");
              sugg_type = SUGGEST_TYPE::VALUE;
              return;
            }
            
            
          } else {
            
            ++i;
            str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
            i_context = i;
            
            if(type == "string"){
              sugg_type = SUGGEST_TYPE::FREEFORM_STRING;
              // NOTA: we accept empty strings
              
              // we extract the quotes!
              if(i == n || x[i] != '"'){
                error = util::txt(
                  "The current shortcut is invalid. The command `", cmd_value, "` must be of the form \n",
                  "\"<", cmd_value, ": ", str::dquote("string"), ">\".",
                  "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                  "=> there is no starting quote");
                return;
              }
              
              ++i;
              freeform = str::extract_until_next_unescaped_char(x, i, '"');
              
              if(i == n){
                error = util::txt(
                  "The current shortcut is invalid. The command `", cmd_value, "` must be of the form \n",
                  "\"<", cmd_value, ": ", str::dquote("string"), ">\".",
                  "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                  "=> the quotes following ", str::bquote(cmd_value), " are not closed");
                return;
              }
              
              ++i;
              str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
              
              // last check: no newline => can lead to severe bugs
              if(str::str_contains(value, '\n')){
                error = util::txt(
                  "The current shortcut is invalid. The command `", cmd_value, "` shall not contain newlines.",
                  "Problem: in the shortcut ", err_shorcut(str::ascii_printable(x), i), "\n",
                  "=> the value for ", str::bquote(cmd_value), " (", str::ascii_printable(freeform), "), contains a newline");
                return;
              }
              
            } else if(type == "int"){
              sugg_type = SUGGEST_TYPE::FREEFORM_INT;
              
              freeform = str::extract_until_next_unescaped_char(x, i, '>');
              str::trim_WS_inplace(freeform);
              
              if(!str::is_int_inside(freeform)){
                error = util::txt(
                  "The current shortcut is invalid. The command `", cmd_value, "` must be of the form \n",
                  "\"<", cmd_value, ": int>\".",
                  "Problem: in the shortcut ", err_shorcut(x, i), "\n",
                  "=> the integer in `", freeform, "` could not be parsed");
                return;
              }
            }
          }
          
        }
        
      }
      
      // out of a regular command, i must point to '>'
      if(i == n || x[i] != '>'){
        error = util::txt(
          "The current shortcut is invalid. Each shortcut command should be contained within \"<>\".\n",
          "Problem: in the shortcut ", err_shorcut(x, i), "\n",
          "=> the command ", str::dquote(cmd), " does not have a closing \">\"");
        return;
      }
      
      // all good!
      all_commands.emplace_back(cmd, value, freeform);
      ++i;
      
    } else {
      // insertion
      
      i_context = i;
      
      string insert_value;
      if(x[i] == '"'){
        // quoted insertion
        ++i;
        while(i < n && !(x[i] == '"' && !str::is_escaped(x, i))){
          if(x[i] == '"'){
            insert_value.back() = '"';
            ++i;
          } else {
            insert_value += x[i++];
          }
        }
        
        if(i == n){
          error = util::txt(
            "The current shortcut is invalid. When using quotes to insert text, each open quote must be closed.\n",
            "Problem: in the shortcut ", err_shorcut(x, i), "\n",
            "=> there is no closing quote");
          return;
        }
        
        // we go to the next <
        while(i < n && x[i] != '<'){
          ++i;
        }
        
      } else {
        
        // we go to the next <
        while(i < n && x[i] != '<'){
          insert_value += x[i++];
        }
        
        insert_value = str::trim_WS(insert_value);
        
      }
      
      // we parse the value to be inserted and tranform it into a command sequence
      string insert_left;
      string insert_right;
      
      if(str::str_contains(insert_value, "_sel_")){
        insert_left = str::delete_after(insert_value, "_sel_");
        insert_right = str::delete_until(insert_value, "_sel_");
        
        all_commands.emplace_back("selection", "stash");
        all_commands.emplace_back("insert", insert_left);
        all_commands.emplace_back("selection", "pop");
        all_commands.emplace_back("insert", insert_right);
        
      } else if(str::str_contains(insert_value, "_all_")){
        insert_left = str::delete_after(insert_value, "_all_");
        insert_right = str::delete_until(insert_value, "_all_");
        
        all_commands.emplace_back("move_x", "leftmost");
        all_commands.emplace_back("insert", insert_left);
        all_commands.emplace_back("move_x", "rightmost");
        all_commands.emplace_back("insert", insert_right);
        
      } else {
        all_commands.emplace_back("insert", insert_value);
      }
      
    }
    
  }
  
  // for the autocomplete context, we don't need this check
  _is_valid_context = true;
  
  // we check conditions are closed
  int n_if_open = 0;
  for(size_t i = 0 ; i < all_commands.size() ; ++i){
    const ShortcutAction &cmd = all_commands[i];
    if(cmd.is_if()){
      ++n_if_open;
      
    } else if(cmd.is_endif()){
      --n_if_open;
      
    } else if(cmd.is_else()){
      // they must follow an if
      if(n_if_open <= 0){
        error = util::txt(
          "The shortcut is invalid. `else` statements must be placed after an 'if'.\n"
          "Problem: in the shortcut ", str::dquote(x), "\n",
          "=> the statement ", cmd.get_raw_command(), " does not follow an 'if"
        );
        return;
      }
    }
    
  }
  
  if(n_if_open > 0){
    error = util::txt(
      "The shortcut is invalid. All 'if' statements must be closed with an <endif> .\n"
      "Problem: in the shortcut ", str::dquote(x), "\n",
      "=> <endif> is missing"
    );
    return;
  }
  
  if(n_if_open < 0){
    error = util::txt(
      "The shortcut is invalid. The <endif> statement can only be used to close an 'if' statement.\n"
      "Problem: in the shortcut ", str::dquote(x), "\n",
      "=> too many <endif>"
    );
    return;
  }
  
  _is_valid = true;
}


void print(std::deque<ShortcutAction> all_cmd){
  
  util::info_msg("# commands: ", all_cmd.size());
  int i = 1;
  for(const auto &sh : all_cmd){
    util::info_msg(i++, ") ", sh.get_raw_command());
  }
  
}

//
// applying the shortcuts ------------------------------------------------------
//


std::shared_ptr<CommandToEvaluate> ConsoleCommand::apply_shortuts(const ParsedShortcut &x){
  
  stashed_shortcut_commands = x.get_all_commands();
  
  return apply_shortuts(stashed_shortcut_commands);
}

std::shared_ptr<CommandToEvaluate> ConsoleCommand::apply_shortuts(std::deque<ShortcutAction> &all_shortcuts){
  
  in_shortcut = true;
  std::shared_ptr<CommandToEvaluate> pres = std::make_shared<CommandToEvaluate>();
  
  while(!all_shortcuts.empty()){
    const ShortcutAction cmd = all_shortcuts.front();
    if(cmd.is_if()){
      apply_shortut_if(all_shortcuts);
      
    } else {
      pres = cmd.run_command(this);
      all_shortcuts.pop_front();
      
      if(pres->is_complete){
        in_shortcut = false;
        return pres;
      }
      
    }
    
  }
  
  in_shortcut = false;
  return pres;
}

void ConsoleCommand::apply_shortut_if(std::deque<ShortcutAction> &all_shortcuts){
  // this function recreates the shortcuts vector with the commands
  // from the appropriate branch of the if-condition
  
  
  if(all_shortcuts.empty()){
    util::error_msg("Internal error: in apply_shortut_if, the shortcuts cannot be empty");
    return;
  }
  
  if(!all_shortcuts[0].is_if()){
    util::error_msg("Internal error: in apply_shortut_if, the first shortcut must be an if");
    all_shortcuts.clear();
    return;
  }
  
  std::deque<ShortcutAction> branch_commands;
  
  int i = 0;
  const int n = all_shortcuts.size();
  bool is_valid_branch = false;
  
  while(i < n && !is_valid_branch){
    
    const ShortcutAction &cmd = all_shortcuts.at(i);
    if(cmd.is_if() || cmd.is_else_if()){
      is_valid_branch = cmd.is_condition_verified(this);
      
      ++i;
      while(i < n && all_shortcuts.at(i).is_and_or()){
        const ShortcutAction &cmd_and_or = all_shortcuts.at(i);
        if(cmd_and_or.is_and()){
          is_valid_branch = is_valid_branch && cmd_and_or.is_condition_verified(this);
        } else {
          is_valid_branch = is_valid_branch || cmd_and_or.is_condition_verified(this);
        }
        
        ++i;
      }
      
      if(!is_valid_branch){
        // we go to the next if-else, or endif
        
        int n_if_open = 0;
        while(i < n){
          const ShortcutAction &cmd_branch = all_shortcuts.at(i);
          
          if(cmd_branch.is_if()){
            ++n_if_open;
            
          } else if(cmd_branch.is_else_endif()){
            
            if(n_if_open > 0){
              // we need to go to the endif directly, this is a nested condition
              if(cmd_branch.is_endif()){
                --n_if_open;
              }
            } else {
              break;
            }
            
          }
          
          ++i;
        }
        
        // out of this loop either:
        // - i => is_else_if, we continue and check
        // - i => is_else_no_if, we continue and don't check (that's the branch)
        // - i => endif => we're out
        // - i == n => ill formed statement
        
        if(i == n){
          util::error_msg("The shortcut condition is invalid. A condition must be ",
                          "a <if: connditon> terminated with an <endif>\n",
                          "Problem: end of the shortcut reached before <endif>");
          all_shortcuts.clear();
          return;
        }
        
        if(all_shortcuts.at(i).is_endif()){
          // we skip it
          ++i;
          break;
        }
        
      }
      
    } else if(cmd.is_else_no_if()){
      
      is_valid_branch = true;
      ++i;
      
    } else {
      util::error_msg("Internal error: apply_shortcut_if: expecting either if/else if/else, ",
                      "received: ", str::dquote(cmd.get_raw_command()), ". Please fix.");
      all_shortcuts.clear();
      return;
    }
  }
  
  // here: either we're in a valid branch, i pointing at the first valid command, 
  // either we're out of the ifs, just after the <endif>
  // 
  
  if(is_valid_branch){
    // this is the right branch: extract the branch then go to the endif
    int n_if_open = 0;
    
    while(i < n){
      
      const ShortcutAction &cmd_branch = all_shortcuts.at(i);
      if(cmd_branch.is_if()){
        ++n_if_open;
        
      } else if(cmd_branch.is_else_endif()){
        
        if(n_if_open > 0){
          // we need to go to the endif directly, this is a nested condition
          if(cmd_branch.is_endif()){
            --n_if_open;
          }
        } else {
          // we only look at the branch, so we're done
          break;
        }
        
      }
      
      branch_commands.push_back(cmd_branch);
      ++i;
    }
    
    if(i == n){
      util::error_msg("The shortcut condition is invalid. A condition must be ",
                      "a <if: connditon> terminated with an <endif>\n",
                      "Problem: end of the shortcut reached before <endif>");
      all_shortcuts.clear();
      return;
    }
    
    // now we go at the endif
    if(all_shortcuts.at(i).is_endif()){
      // OK 
      ++i;
      
    } else {
      // i points to an else
      // we fetch the endif
      
      ++i;
      n_if_open = 0;
      while(i < n){
      
        const ShortcutAction &cmd_branch = all_shortcuts.at(i);
        if(cmd_branch.is_if()){
          ++n_if_open;
          
        } else if(cmd_branch.is_endif()){
          
          if(n_if_open > 0){
            --n_if_open;
          } else {
            break;
          }
          
        }
        
        ++i;
      }
      
      if(i == n){
        util::error_msg("The shortcut condition is invalid. A condition must be ",
                        "a <if: connditon> terminated with an <endif>\n",
                        "Problem: end of the shortcut reached before <endif>");
        all_shortcuts.clear();
        return;
      }
      
      // here we point at an endif, we skip it
      ++i;
    }
    
  }
  
  // we first wipe out the commands related to the condition
  all_shortcuts.erase(all_shortcuts.begin(), all_shortcuts.begin() + i);
  
  // then we add the commands of the branch, if OK
  if(is_valid_branch && !branch_commands.empty()){
    all_shortcuts.insert(all_shortcuts.begin(), branch_commands.begin(), branch_commands.end());
  }
  
}


