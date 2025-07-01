    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#include "specialfunctions.hpp"
#include "autocomplete.hpp"
#include "console.hpp"


//
// ConsoleCommand methods 
//

namespace {

inline string fun_has_only_xx_arguments(const string &fun, const vector<ArgumentFormat> &all_args){
  int n = all_args.size();
  string txt = "The function " + fun + " has only "+ std::to_string(n) + " argument" + (n == 1 ? "" : "s");
  txt += "\n" + arg_signature(all_args);
  return txt;
}

} // end anonymous namespace


str::StringMatch ConsoleCommand::ac_suggestion_special_command(){
  // A special command starts with %
  // it's either a special functions, either an option
  
  // NOTE: cursor_str_x is always >= 1
  string before_cursor = pline->str().substr(1, cursor_str_x - 1);
  
  if(str::starts_with(before_cursor, "options.")){
    return ac_suggestion_options(before_cursor);
  }
  
  return ac_suggestion_special_function(before_cursor);
}

str::StringMatch ConsoleCommand::ac_suggestion_special_function(const string &before_cursor){
  //
  // special function suggestion
  //
  
  string context = before_cursor;
  AutocompChoices choices;
  
  vector<string> fun_names = util::map_names(all_special_functions);
  fun_names.push_back("options.");
  
  // are we in an argument of a valid fun?
  string fun_context = str::delete_after(context, " ");
  
  if(util::vector_contains(fun_names, fun_context)){
    // we're in a valid function
    // ex:
    // %width |
    // file_list ../|
    
    const vector<ArgumentFormat> &all_args_fmt = all_special_functions[fun_context].get_arg_fmt();
    
    if(all_args_fmt.empty()){
      choices.set_cause_empty("This special function has no argument");
      
    } else {
      // the autocomplete is produced only for paths
      // in that case, the result of the AC is quoted
      
      string args_context = str::delete_until(context, " +");
      
      if(fun_context.size() == context.size()){
        
        choices = vector<string>{fun_context + " "};
        choices.set_continue();
        
      } else {
        
        ParsedCommandArguments all_args_input(args_context);
        
        // we go to the last non valid argument
        
        int n_fmt = all_args_fmt.size();
        int j = 0;
        
        int n_input = all_args_input.size();
        // 5 ./hello|    A
        // 5 ./hello |   B
        // => A in arg: ./hello
        // => B not in arg: ""
        // 
        
        bool any_invalid = false;
        while(j < n_input && j < n_fmt){
          
          ArgumentFormat fmt = all_args_fmt[j];
          string arg = all_args_input.at(j).string();
          ParsedArg opt(arg, fmt);
          if(!opt.is_unset()){
            // this is ok
            ++j;
          } else {
            any_invalid = true;
            break;
          }
          
        }
        
        // EXAMPLES (values out of the previous while):
        // 
        //   fmt: int logical        path
        // input:   5   false ./hello wor|
        // 
        // any_invalid: depends on whether ./hello exists
        // 
        // case "./hello" exists:
        // any_invalid = false
        // j = 3
        // 
        // j == n_fmt:
        //  - as fmt.back() == path => we stack "./hello" and "wor"
        //  - if fmt.back() != path:
        //     i) j_input < n_input || !in_arg => msg fun has only 3 args + signature
        //     ii) else: info on the current expected type
        // 
        // -----------------------------------------------------------------
        // 
        // case "./hello" DOES NOT exist:
        // any_invalid = true
        // j = 2
        // 
        // j < n_fmt:
        //  - as fmt.at(j) == path => we stack "./hello" and "wor"
        //  - if fmt.at(j) != path:
        //     i) j_input != n_input - 1 || !in_arg => msg fun has only 3 args + signature
        //     ii) else: info on the current expected type
        //  
        //  ================================================================
        //  
        // 
        // 
        // 
        
        bool AC_ok = false;
        ArgumentFormat fmt;
        if(!any_invalid){
          
          if(j == 0){
            // when n_input == 0
            fmt = all_args_fmt.at(0);
          } else {
            fmt = all_args_fmt.at(j - 1);
          }
          
          if(n_input == 0){
            AC_ok = true;
            context.clear();
            
          } else if(j == n_fmt && j < n_input){
            // more inputs than requested arguments
            // 
            
            if(fmt.is_path()){
              // it can be a multiword completion
              AC_ok = true;
              context.clear();
              for(int k = j - 1 ; k >= 0 && k < n_input ; ++k){
                const CommandArg &a = all_args_input.at(k);
                if(a.is_quoted()){
                  AC_ok = false;
                  break;
                }
                context += a.string() + a.trailing_space();
              }
              
            }
            
            if(!AC_ok){
              choices.set_cause_empty(fun_has_only_xx_arguments(fun_context, all_args_fmt));
            }
            
          } else if(j == n_fmt && j == n_input){
            // as many input as expeted arg + all are valid
            AC_ok = true;
            context = all_args_input.at(j - 1).full_string();
            
          } else if(j < n_fmt && j == n_input){
            // more expected args than input
            
            AC_ok = true;
            context = all_args_input.at(j - 1).full_string();
            
          } else {
            throw util::bad_type("Internal error in AC of special function.\n",
                                  "We should never end on this branch.");
          }
          
        } else {
          // the current j's input is invalid
          
          fmt = all_args_fmt.at(j);
          
          if(fmt.is_path()){
            AC_ok = true;
            
            context.clear();
            for(int k = j ; k >= 0 && k < n_input ; ++k){
              const CommandArg &a = all_args_input.at(k);
              if(a.is_quoted()){
                AC_ok = false;
                break;
              }
              context += a.string() + a.trailing_space();
            }
            
          } else if(j == n_input - 1){
            AC_ok = true;
            context = all_args_input.at(j).full_string();
            
          } 
          
          if(!AC_ok){
            string msg = util::txt("The current arguments are invalid, ",
                                    "the function signature is:\n",
                                    arg_signature(all_args_fmt));
            
            choices.set_cause_empty(msg);
          }
          
        }
        
        if(AC_ok){
          
          if(fmt.has_suggestion()){
            choices = *fmt.get_suggestion(this);
            
          } else if(fmt.is_path()){
            const bool is_quoted = (n_input > 0 && all_args_input.back().is_quoted());
            choices = suggest_path(context, !is_quoted);
            
          } else if(fmt.is_logical()){
            choices = vector<string>{"true", "false"};
            
          } else {
            string msg = util::txt(fmt.type_verbose(), 
                                    "\nFun. signature: ", arg_signature(all_args_fmt));
            choices.set_cause_empty(msg);
          }
          
        }
      }
    }
    
    
  } else {
    // we're still trying to find a valid function name
    
    std::sort(fun_names.begin(), fun_names.end());
    
    // labels
    const size_t n = fun_names.size();
    vector<string> labels(n, "");
    for(size_t i = 0 ; i < n ; ++i){
      const string &fun = fun_names[i];
      if(fun != "options."){
        const vector<ArgumentFormat> &all_args = all_special_functions.at(fun).get_arg_fmt();
        if(!all_args.empty()){
          fun_names[i] += " ";
          labels[i] = "  <" + arg_signature(all_args) + ">";
        }
      }
    }
    
    choices = fun_names;
    
    // we set continue ovnly for "options."
    vector<string> cont(n, "false");
    size_t pos = util::which<string>(fun_names, "options.");
    if(pos < cont.size()){
      cont[pos] = "true";
    }
    
    choices.set_meta("continue", cont);
    choices.set_meta("labels", labels);
    
  }
  
  return str::string_match(context, choices);
}

str::StringMatch ConsoleCommand::ac_suggestion_options(const string &before_cursor){
  //
  // options suggestion
  //
  
  string context = str::delete_until(before_cursor, "options.");
  AutocompChoices choices;
  
  string key = str::delete_after(context, {".", " "});
  
  // either we suggest the key, either we suggest the argument
  bool is_key_valid = program_opts.is_key_settable(key);
  if(key.size() == context.size()){
    is_key_valid = false;
  }
  
  if(!is_key_valid){
    // later: add labels with the type => MSV program_opts.labeled_names()
    // is this a nested key? (a key containing a dot, eg, color.var)
    
    const vector<string> all_key_roots = program_opts.get_key_roots();
    
    if(util::vector_contains(all_key_roots, key)){
      
      string subkey = context.substr(key.size());
      if(!subkey.empty() && subkey[0] == '.'){
        subkey.erase(subkey.begin());
      }
      subkey = str::delete_after(subkey, {".", " "});
      
      vector<string> all_subkeys = program_opts.get_subkeys(key);
      
      context = str::delete_until(context, ".");
      
      if(util::vector_contains(all_subkeys, subkey) && subkey.size() < context.size()){
        // all good!
        is_key_valid = true;
        key += "." + subkey;
        
      } else {
        str::append_right(all_subkeys, ".");
        choices = all_subkeys;
        choices.set_continue();
      }
      
    } else {
      
      vector<string> option_names = program_opts.get_key_roots();
      str::append_right(option_names, ".");
      choices = option_names;
      choices.set_continue();
      
    }
    
  }
  
  if(is_key_valid){
    // key is valid
    
    context = str::delete_until(context, ".");
    
    string set_value = str::delete_after(context, {".", " "});
    
    vector<string> valid_set_values = {"set", "set_global", "set_local", "get"};
    vector<string> vec_get_reset = {"get"};
    const ArgumentFormat &fmt = program_opts.get_option_format(key);
    if(fmt.has_default()){
      valid_set_values.push_back("reset");
      vec_get_reset.push_back("reset");
    }
    
    if(!str::is_string_in(set_value, valid_set_values) || 
       set_value.size() == context.size()){
      
      AutocompChoices choices_set = vector<string>{"set ", "set_local ", "set_global "};
      
      if(fmt.is_shortcut()){
        // no continue
      } else {
        // continue AC
        choices_set.set_continue(AutocompChoices::STYPE::NO_INHERIT);
      }
      
      AutocompChoices choices_get = vec_get_reset;
      
      choices.push_back(choices_set).push_back(choices_get);
      
    } else {
      
      // set_value is valid
      context = str::delete_until(context, " +");
      
      if(fmt.is_color()){
        // first suggestion is RGB
        // we directly add the color in VTS
        // 
        vector<string> all_colors = util::map_names(VTS::all_html_colors);
        const size_t n = all_colors.size();
        vector<string> all_vts(n);
        for(size_t i = 0 ; i < n ; ++i){
          all_vts[i] = VTS::all_html_colors.at(all_colors[i]) + VTS::BG_BLACK;
        }
        
        all_colors.insert(all_colors.begin(), "#rrggbb");
        all_vts.insert(all_vts.begin(), "");
        
        choices = all_colors;
        choices.set_meta("vts", all_vts);
        
      } else if(fmt.is_logical()){
        choices = vector<string>{"true", "false"};
        
      } else if(fmt.is_shortcut()){
        
        // we parse the context
        const ParsedShortcut &shortcut(context);
        
        using sugg_sh_t = ParsedShortcut::SUGGEST_TYPE;
        sugg_sh_t sh_type = shortcut.get_sugg_type();
        
        const size_t i_context = shortcut.get_i_context();
        if(i_context < context.size()){
          context = context.substr(i_context);
        } else {
          context.clear();
        }
        
        if(sh_type == sugg_sh_t::DEFAULT){
          // We're in free form text, we only give advice
          choices.set_cause_empty(util::txt(
            "write text to insert it when the shortcut is run",
            "\n_all_: represents the existing line",
            "\nex: head(_all_)"
            "\n_sel_: represents the selection",
            "\nto add spaces at start/end: use quotes",
            "\nto add commands, use <cmd_name>"
          ));
          
        } else {
          // this is a command
          
          const string &cmd = shortcut.get_cmd();
          const string &value = shortcut.get_value();
          const string &freeform = shortcut.get_freeform();
          const bool is_first_condition = shortcut.get_is_first_condition();
          
          const bool already_closed = (cursor_str_x < pline->size()) ? pline->at(cursor_str_x) == ">" : false;
          
          if(sh_type == sugg_sh_t::COMMAND){
            // we suggest the cmd
            choices = already_closed ? shortcut_cmd_sugg_closed.copy() : shortcut_cmd_sugg_open.copy();
            
          } else if(sh_type == sugg_sh_t::CONDITION_FIRST){
            choices = gen_shortcut_condition_suggestions(already_closed, is_first_condition, cmd == "if");
            
          } else if(sh_type == sugg_sh_t::CONDITION){
            choices = gen_shortcut_condition_suggestions(already_closed, is_first_condition, cmd == "if");
            
          } else if(sh_type == sugg_sh_t::FREEFORM_INT){
            choices.set_cause_empty("freeform string");
                      
          } else if(sh_type == sugg_sh_t::FREEFORM_STRING){
            choices.set_cause_empty("integer");
            
          } else if(sh_type == sugg_sh_t::VALUE){
            
            vector<string> all_valid_values = shortcut_commands.at(cmd);
            
            // we may suggest the freeform
            const size_t n = all_valid_values.size();
            vector<string> cursor_shift(n);
            vector<string> append_right(n);
            
            int i = -1;
            for(auto &value : all_valid_values){
              ++i;
              if(!util::map_contains(shortcut_values_with_freeform, value)){
                if(!already_closed){
                  append_right[i] = ">";
                }
                
              } else {
                
                const string &type = shortcut_values_with_freeform.at(value);
                if(type == "string"){
                  value += ": \"\"";
                  cursor_shift[i] = already_closed ? "-1" : "-2";
                } else {
                  value += ": ";
                }
                
                if(!already_closed){
                  append_right[i] = ">";
                }
                
              }
            }
            
            AutocompChoices value_choices = all_valid_values;
            value_choices.set_meta("cursor_shift", cursor_shift);
            value_choices.set_meta("append_right", append_right);
            value_choices.set_append_left(" ");
            
            choices = value_choices;
            
          }
        }
        
      } else if(fmt.is_path()){
        const bool is_quoted = str::starts_with(context, "\"");
        if(is_quoted){
          context.erase(context.begin());
        }
        
        choices = suggest_path(context, !is_quoted);
      }
      
    } 
  }
  
  return str::string_match(context, choices);
}


void ConsoleCommand::special_command(const string &fun_raw){
  // fun_raw must start with %
  
  flush_cmd();
  
  if(fun_raw.empty() || fun_raw.front() != '%'){
    util::error_msg("\nINTERNAL ERROR: Special console functions/options must start with `%`.");
    return;
  }
  
  string fun_str = fun_raw.substr(1);
  
  //
  // step 1: options 
  //
  
  if(str::starts_with(fun_str, "options.")){
    
    vector<string> all_info = str::str_split(fun_str, {".", " +"});
    
    // accepted format:
    // options.color_fun.set #771155
    // => must be of length at least 4 or error
    
    if(all_info.size() < 3){
      util::error_msg("Error when setting options. It must be of the form:\n",
                      "either: options.option_key.set value\n",
                      "or    : option.option_key.get",
                      "\nHence ", str::bquote(fun_str), " is invalid.");
      
      return;
    }
    
    string key = all_info[1];
    string set_value = all_info[2];
    string value = str::delete_until(fun_str, " +");
    value = str::trim_WS_rm_quotes(value);
    
    // validation: key
    if(!program_opts.is_key_settable(key)){
      
      // maybe a nested option?
      if(util::vector_contains(program_opts.get_key_roots(), key)){
        
        if(util::vector_contains(program_opts.get_subkeys(key), set_value)){
          // OK
          key += "." + set_value;
          
          if(all_info.size() < 4){
            util::error_msg("Error when setting options. It must be of the form:\n",
                            "either: options." + key + ".set value\n",
                            "or    : option." + key + ".get",
                            "\n=> Add set/get after the option name");
            
            return;
          }
          
          set_value = all_info[3];
          
        } else {
          util::error_msg("The option key ", str::bquote(set_value), " is not ",
                          " a valid element of `" + key + "`.\n",
                          "FYI, the valid `" + key + "` are: \n", 
                          str::fit_screen(program_opts.get_subkeys(key)));
          
          return;
        }
        
      } else {
        util::error_msg("The option key ", str::dquote(key), " is invalid.\n",
                        "FYI, the valid options are: \n", 
                        str::fit_screen(program_opts.names()));
        return;
      }
      
    }
    
    // validation: set
    vector<string> valid_set_values = {"set", "set_global", "set_local", "get"};
    const ArgumentFormat &fmt = program_opts.get_option_format(key);
    if(fmt.has_default()){
      valid_set_values.push_back("reset");
    }
    
    if(!str::is_string_in(set_value, valid_set_values)){
      util::error_msg("After the option key must come one of ",
                      str::enumerate(valid_set_values, str::EnumOpts().bquote().add_or()), ".\n",
                      "The value ", str::bquote(set_value), " is not one of them.");
      
      
      return;
    }
    
    const bool is_set = str::starts_with(set_value, "set");
    
    // empty value check
    if(is_set){
      if(value.empty() && !str::starts_with(key, "shortcut.")){
        // NOTA: empty shortcuts are OK
        // 
        util::error_msg("Error when setting options. It must be of the form:\n",
                        "options.option_key.", set_value, " value, with `value` non-empty\n",
                        "Problem: the current value is empty");
        
        return;
      }
    }
    
    // validation set_option
    
    if(is_set){
      program_opts.set_option(key, value, set_value);
      
      // special case: we need this
      if(key == "prompt.color"){
        prompt_color = program_opts.get_option(key).get_color();
        current_prompt_color = program_opts.get_option(key).get_color();
        
      } else if(key == "prompt.continue"){
        prompt_cont = value;
        
      } else if(key == "prompt.main"){
        prompt_main = value;
        current_prompt_main = value;
        
      }
      
    } else if(set_value == "reset"){
      program_opts.reset_options(key);
      
    } else if(set_value == "get"){
      const ParsedArg &opt = program_opts.get_option(key);
      const string &current_opt = opt.get_input();
      if(!current_opt.empty() && current_opt[0] == '"'){
        cout << current_opt << "\n";
      } else {
        cout << str::dquote(current_opt) << "\n";
      }
    }
    
    return;
  }
  
  //
  // step 2: special functions 
  //
  
  fun_str = str::delete_after(fun_str, " ");
  
  if(util::map_contains(all_special_functions, fun_str)){
    
    SpecialFunctionInfo &sf = all_special_functions[fun_str];
    
    string cmd_line_no_fun = str::delete_until(fun_raw, " +");
    
    const vector<ArgumentFormat> &all_args_fmt = sf.get_arg_fmt();
    ParsedCommandArguments all_args_input(cmd_line_no_fun);
    
    const size_t n_input = all_args_input.size();
    const size_t n_fmt = all_args_fmt.size();
    
    vector<ParsedArg> args_clean(n_fmt);
    
    if(n_input > n_fmt){
      if(n_input == n_fmt + 1 && all_args_input.back().string().empty()){
        // OK
      } else {
        util::error_msg("Note: the function ", str::bquote(fun_str), " has only ", 
                      n_fmt, " argument", (n_fmt > 1 ? "s" : ""),  
                      " (", n_input, " provided)");
      }
    }
    
    if(n_fmt == 0){
      sf.fun();
      
    } else {
      
      string err;
      for(size_t i = 0 ; i < n_fmt ; ++i){
        
        const ArgumentFormat &fmt = all_args_fmt.at(i);
        
        if(i < n_input){
          ParsedArg opt(all_args_input.at(i).string(), fmt, util::DoCheck(err));
          
          if(opt.is_unset()){
            util::error_msg("Error: In function ", str::bquote(fun_str), 
                            ", the ", str::nth(i + 1), " argument could not be parsed.\n",
                            "It should be of type ", fmt.type_verbose(), ".\n",
                            "Problem: ", err);
            return;
          }
          
          args_clean[i] = opt;
          
        } else if(fmt.has_default()){
          ParsedArg opt(fmt);
          
          args_clean[i] = opt;
          
        } else {
          util::error_msg("Error: The function ", str::bquote(fun_str), " expects ", n_fmt, 
                          " argument", (n_fmt > 1 ? "s" : ""), " (", n_input, 
                          " provided)\n",
                          "FYI it expects: ", arg_signature(all_args_fmt));
          
          return;
        }
        
      }
      
      sf.fun(args_clean);
    }
    
    
  } else {
    util::error_msg(prog_name, ": the function `", fun_str, "` is not valid");
  }
  
}


//
// tools used by special functions --------------------------------------------- 
//

const string &ConsoleCommand::get_last_output() const {
  
  int n = io_backup_type.size();
  int i = n - 1;
  while(i >= 0){
    if(io_backup_type[i] == IO_TYPE::OUTPUT){
      return io_backup[i];
    }
    --i;
  }
  
  return UNSET::STRING;
}

const vector<string> ConsoleCommand::get_all_inputs() const {
  
  vector<string> all_inputs;
  
  const size_t n = io_backup_type.size();
  for(size_t i = 0 ; i < n ; ++i){
    if(io_backup_type[i] == IO_TYPE::INPUT){
      all_inputs.push_back(io_backup[i]);
    }
  }
  
  return all_inputs;
}

const vector<string> ConsoleCommand::get_all_outputs() const {
  
  vector<string> all_outputs;
  
  const size_t n = io_backup_type.size();
  for(size_t i = 0 ; i < n ; ++i){
    if(io_backup_type[i] == IO_TYPE::OUTPUT){
      all_outputs.push_back(io_backup[i]);
    }
  }
  
  return all_outputs;
}

std::shared_ptr<AutocompChoices> suggest_reprex_last(ConsoleCommand *pconcom){
  // we return the digits and the past commands as labels
  // NOTE: we reverse the order to make it more natural for the user
  // (like for the history)
  // 
  
  std::shared_ptr<AutocompChoices> pchoices = std::make_shared<AutocompChoices>();
  
  vector<string> past_commands = pconcom->get_all_inputs();
  
  if(past_commands.empty()){
    pchoices->set_cause_empty("No past commands");
    return pchoices;
  }
  
  const size_t n = past_commands.size();
  vector<string> numbers;
  vector<string> labels;
  
  for(size_t i = 0 ; i < n ; ++i){
    const string &cmd = past_commands[i];
    if(!str::no_nonspace_char(cmd)){
      numbers.push_back(std::to_string(n - i));
      labels.push_back("   <" + str::clean_VTS_markup(str::str_replace(cmd, "\n", "\\n")) + ">");
    }
    
  }
  
  if(labels.empty()){
    pchoices->set_cause_empty("No non-empty past commands");
    return pchoices;
  }
  
  *pchoices = numbers;
  pchoices->set_labels(labels);
  pchoices->set_meta("autocomp-bottom", "true");
  
  return pchoices;
}

//
// special functions ----------------------------------------------------------- 
//


void sf_time_all(ConsoleCommand *pconcom){
  pconcom->time_all = true;
}

void sf_time_none(ConsoleCommand *pconcom){
  pconcom->time_all = false;
}

void sf_debug_to_file([[maybe_unused]] ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  const ParsedArg arg = all_args.at(0);
  
  bool is_file = arg.get_logical();
  if(arg.is_default()){
    util::next_debug_type();
    
  } else if(is_file){
    util::set_debug_to_file();
    
  } else {
    util::set_debug_to_msg();
    
  }
  
}

void sf_path_history(ConsoleCommand *pconcom){
  const fs::path path = pconcom->hist_list["main"]->get_history_path();
  const string path_fmt = util::format_path(path);
  util::msg(path_fmt);
  simpleclipboard::set_clipboard(path_fmt);
  util::info_msg("Info: Path copied to clipboard");
}

void sf_path_options(ConsoleCommand *pconcom){
  const fs::path path = pconcom->get_path_options();
  const string path_fmt = util::format_path(path);
  util::msg(path_fmt);
  simpleclipboard::set_clipboard(path_fmt);
  util::info_msg("Info: Path copied to clipboard");
}

void sf_path_executable([[maybe_unused]] ConsoleCommand *pconcom){
  char path[555];
  int status = GetModuleFileNameA(NULL, path, 555);
  if(status == 0){
    util::error_msg("The path of the execuable could not be found");
  } else {
    const string path_fmt = util::format_path(string(path));
    
    std::cout << path_fmt << "\n";
    simpleclipboard::set_clipboard(path_fmt);
    util::info_msg("Info: Path copied to clipboard");
  }
}


void sf_clear_history(ConsoleCommand *pconcom){
  const fs::path &path = pconcom->hist_list["main"]->get_history_path();
  
  if(fs::exists(path)){
    bool success = fs::remove(path);
    if(!success){
      util::error_msg("Removing the history failed. ",
                      "Location of the existing history:\n", path);
    } else {
      std::cout << "Removing history ... success\n";
    }
  }
  
}



void sf_copy_last_output(ConsoleCommand *pconcom){
  
  const string &s = pconcom->get_last_output();
  if(util::is_unset(s)){
    util::error_msg("Error: Currently there is no output in cache.");
    return;
  }
  
  simpleclipboard::set_clipboard(s);
}

void sf_step_into_last_output(ConsoleCommand *pconcom){
  
  const string &s = pconcom->get_last_output();
  if(util::is_unset(s)){
    util::error_msg("Error: Currently there is no output in cache.");
    return;
  }
  
  // we always print the first 5 lines
  vector<string> all_lines = str::str_split(s, "\n");
  int n = all_lines.size();
  int i = 0;
  char command = '5';
  while(i < n){
    
    if(command == 'q'){
      return;
      
    } else if(command >= '1' && command <= '9'){
      
      int nlines = command - '0';
      int j = 0;
      while(j++ < nlines && i < n){
        std::cout << all_lines[i++] << "\n";
      }
      
      command = '0';
      
    } else {
      string user_input = pconcom->read_line(ReadOptions().no_print().n_char(1).to_lower());
      if(user_input.empty()){
        command = '1';
      } else {
        command = user_input[0];
      }
    }
    
  }
  
}

void sf_width(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  int new_width = all_args[0].get_int();
  
  if(new_width < 0){
    util::info_msg("Current width: ", pconcom->win_width);
    return;
  }
  
  pconcom->custom_win_width = true;
  CursorInfo curs(pconcom->handle_out);
  
  if(new_width == 0){
    // reset
    pconcom->custom_win_width = false;
    new_width = curs.win_width;
    util::info_msg("Setting width to: ", new_width);
    
  } else if(static_cast<size_t>(new_width) > curs.win_width){
    util::info_msg("Note: width,", new_width, ", > max width of ", curs.win_width, "\n"
                   "      Setting to ", curs.win_width, " instead");
    new_width = curs.win_width;
  }
  
  pconcom->win_width = new_width;
  pconcom->plgsrv->resize_window_width(new_width);
  
}

void sf_file_list(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  fs::path path = all_args.at(0).get_path();
  
  if(path.string() == "." || path.string().empty()){
    path = fs::current_path();
  }
  
  if(!fs::exists(path)){
    util::error_msg("Aborting: The current path does not exist");
    return;
  }
  
  if(!fs::is_directory(path)){
    util::error_msg("Aborting: The current path is not a directory");
    return;
  }
  
  vector<string> all_paths;
  for(auto &p : fs::directory_iterator(path)){
    const fs::path &tmp_path = p.path();
    if(fs::is_directory(tmp_path)){
      all_paths.push_back(tmp_path.filename().string() + "/");
    } else {
      all_paths.push_back(tmp_path.filename().string());
    }
  }
  
  if(all_paths.empty()){
    util::error_msg("No file found at this path.");
    return;
  }
  
  
  vector<string> colors = {
    VTS::all_html_colors.at("cornflower_blue"),
    VTS::all_html_colors.at("pale_violet_red")
  };
  
  uint win_width = pconcom->window_width();
  for(auto &p : all_paths){
    p = colors[p.back() == '/'] + p + VTS::FG_DEFAULT;
  }
  
  vector<string> all_lines = str::align_on_columns(all_paths, win_width, " | ", true);
  
  util::msg(str::collapse(all_lines, "\n"));
  
}

void sf_file_copy(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  if(all_args.size() < 2){
    util::error_msg("Internal error: there should always be 2 arguments in this function.",
                    "\nCopy aborted.");
    return;
  }
  
  fs::path path_from = all_args.at(0).get_path();
  fs::path path_to = all_args.at(1).get_path();
  
  if(path_to.string() == "." || path_to.string().empty()){
    path_to = fs::current_path();
  }
  const bool is_wd_to = path_to == fs::current_path();
  
  if(!fs::exists(path_from)){
    util::error_msg("Aborting: The path to copy does not exist");
    return;
  }
  
  bool is_dir_from = fs::is_directory(path_from);
  bool is_dir_to = fs::exists(path_to) && fs::is_directory(path_to);
  
  if(!fs::exists(path_to)){
    string err;
    int status = util::create_parent_path(path_to, 4, util::DoCheck(err));
    if(status != 0){
      util::error_msg("Aborting: could not create the directories to the destination path");
      util::error_msg(err);
      return;
    }
  }
  
  if(is_dir_from){
    if(is_wd_to){
      string dirname = util::format_path(path_from);
      
      while(dirname.back() == '/'){
        dirname.pop_back();
      }
      
      while(str::str_contains(dirname, '/')){
        dirname = str::delete_until(dirname, "/+");
      }

      path_to /= dirname;
      
    } 
    
    is_dir_to = fs::exists(path_to) && fs::is_directory(path_to);
    
    if(is_dir_to){
      // we forbid directory replacement, too dangerous
      util::msg("The directory ", path_to, " already exists. Please delete it beforehand.");
      return;
    }
    
  } else {
    if(is_dir_to){
      path_to /= path_from.filename();
    }
    
    if(path_to == path_from){
      util::error_msg("Aborting: the file to copy is the same as the destination");
      return;
    }
    
    if(fs::exists(path_to)){
      util::error_msg("The destination file already exists, do you want to replace it?");
      string choice = pconcom->read_line(ReadOptions().to_lower().prompt("y/n: ").n_char(1));
      if(choice != "y"){
        util::error_msg("Aborting.");
        return;
      }
    }
    
  }
  
  const auto copyOptions = 
    fs::copy_options::overwrite_existing
    | fs::copy_options::recursive;
  
  fs::copy(path_from, path_to, copyOptions);
  
}

void sf_file_peek(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  fs::path path = all_args.at(0).get_path();
  
  if(!fs::exists(path)){
    util::error_msg("Aborting: The current path does not exist");
    return;
  }
  
  if(fs::is_directory(path)){
    util::error_msg("Aborting: The current path leads to a directory and not to a file");
    return;
  }
  
  std::ifstream file_in(path);
  
  if(!file_in.is_open()){
    util::error_msg("The file could not be read.");
    return;
  }
  
  // we always print the first 5 lines
  uint win_width = pconcom->window_width();
  string line;
  int line_nb = 0;
  char command = '5';
  while(true){
    
    if(command == 'q'){
      file_in.close();
      return;
      
    } else if(command >= '1' && command <= '9'){
      
      int nlines = command - '0';
      int j = 0;
      while(j++ < nlines){
        if(std::getline(file_in, line)){
          
          if(line.size() > 2 * win_width){
            string extra = util::txt("[", line.size() - win_width - 22,
                                     " characters omitted]");
            line = line.substr(0, min(line.size() - extra.size() - 2, 0)) + extra;
          }
          
          std::cout << "[" << str::left_fill_with_space(std::to_string(line_nb++), 3) << "] " << line << "\n";
        } else {
          file_in.close();
          return;
        }
      }
      
      command = '0';
      
    } else {
      string user_input = pconcom->read_line(ReadOptions().no_print().n_char(1).to_lower());
      if(user_input.empty()){
        command = '1';
      } else {
        command = user_input[0];
      }
    }
    
  }
  
  file_in.close();
  
}

void sf_open_folder([[maybe_unused]] ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  fs::path path = all_args.at(0).get_path();
  
  if(path.string() == "." || path.string().empty()){
    path = fs::current_path();
  }
  
  if(!fs::exists(path)){
    util::error_msg("Aborting: The current path does not exist");
    return;
  }
  
  if(!fs::is_directory(path)){
    path = path.parent_path();
  }
  
  path = fs::absolute(path);
  
  string msg = run_shell_command("explorer.exe", path.string());
  
  
}


void sf_list_colors(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  const string pat = all_args.at(0).get_string();
  
  vector<string> all_colors;
  bool any_found = false;
  for(const auto &it : VTS::all_html_colors){
    if(str::str_contains(it.first, pat)){
      any_found = true;
      all_colors.push_back(it.second + it.first + VTS::RESET_FG_BG); 
    }
  }
  
  if(!any_found){
    util::error_msg("No color matches the pattern ", str::dquote(pat));
    return;
  }
  
  const int win_width = pconcom->window_width();
  util::msg(str::collapse(str::align_on_columns(all_colors, win_width, " | ", true), "\n"));
  
}

void sf_reprex_last(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  const int n_to_do = all_args.at(0).get_int();
  
  const int n = pconcom->io_backup.size();
  
  if(n == 0){
    util::error_msg("Error: No command in memory");
    return;
  }
  
  vector<string> res;
  
  const string prefix_output = pconcom->get_program_option("reprex.output_color").get_color() + pconcom->get_program_option("reprex.prompt").get_string();
  const string nl_prefix_output = "\n" + prefix_output;
  
  int n_done = 0;
  for(int i = n - 1 ; i >= 0 ; --i){
    string io = pconcom->io_backup.at(i);
    if(pconcom->io_backup_type.at(i) == ConsoleCommand::IO_TYPE::INPUT){
      
      res.push_back(io);
      ++n_done;
      if(n_done >= n_to_do){
        break;
      }
      
    } else {
      
      if(!io.empty() && io.back() == '\n'){
        io.pop_back();
      }
      
      if(str::str_contains(io, "\n")){
        io = str::str_replace(io, "\n", nl_prefix_output);
      }
      
      res.push_back(prefix_output + io + VTS::RESET_FG_BG);
    }
  }
  
  // we place in the right order
  std::reverse(res.begin(), res.end());
  
  string m = str::collapse(res, "\n");
  
  // Later: add formatted text to the keyboard
  // => implement it directly within the clipboard
  simpleclipboard::set_clipboard(str::clean_VTS_markup(m));
  
  util::info_msg("=== begin: reprex ===");
  
  std::cout << m << "\n";
  
  util::info_msg("===   end: reprex ===");
  util::info_msg("NOTE: reprex sent to clipboard");
  
}

void sf_reprex_mode(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  const ParsedArg arg = all_args.at(0);
  
  bool is_reprex_mode = arg.get_logical();
  if(arg.is_default()){
    // we switch the current values
    bool is_empty_line = pconcom->get_program_option("ignore_empty_lines").get_logical();
    bool is_comment = pconcom->get_program_option("ignore_empty_lines").get_logical();
    
    if(is_empty_line && is_comment){
      is_reprex_mode = false;
    } else {
      is_reprex_mode = true;
    }
  }
  
  pconcom->set_program_option("ignore_comment", is_reprex_mode);
  pconcom->set_program_option("ignore_empty_lines", is_reprex_mode);
  
}



//
// shortcut -------------------------------------------------------------------- 
//










