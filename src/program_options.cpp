    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#include "program_options.hpp"


namespace {

//
// validation ------------------------------------------------------------------
//

bool is_valid_key(const string &key, string &key_clean, 
                  const options_fmt_t &all_opts_fmt, string &error){
  
  bool is_key_ok = false;
  key_clean = key;
  
  if(util::map_contains(all_opts_fmt, key)){
    is_key_ok = true;
  } else {
    error = "Invalid option key " + str::bquote(key) + ".";
  }
  
  if(!is_key_ok && util::map_contains(all_opts_fmt, "freeform")){
    const ArgumentFormat &fmt = all_opts_fmt.at("freeform");
    if(fmt.should_freeform_key_start_with()){
      string start = fmt.get_freeform_key_start();
      if(str::starts_with(key, start)){
        key_clean = "freeform";
        is_key_ok = true;
      }
    } else {
      key_clean = "freeform";
      is_key_ok = true;
    }
  }
  
  return is_key_ok;
}

bool is_valid_color(const string &col){
  
  if(util::map_contains(VTS::all_html_colors, col)){
    return true;
  }
  
  if(str::starts_with(col, "raw:")){
    // special: used for debugging
    return true;
  }
  
  if(col.size() != 7){
    return false;
  }
  
  if(col.front() != '#'){
    return false;
  }
  
  for(int i = 1 ; i < 7 ; ++i){
    const char c = col[i];
    const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if(!ok){
      return false;
    }
  }
  
  return true;
}

bool is_valid_path(const string &x, const ArgumentFormat &fmt, string &error){
  fs::path path(x);
  
  // What is a valid path?
  // - has a root
  // - exists?
  // - has a parent?
  // - ends with?
  
  if(fmt.should_path_exist() && !fs::exists(path)){
    error = util::txt("The path should exist but \"", path, "\" does not");
    return false;
  }
  
  if(fmt.should_path_parent_exist()){
    fs::path par_path = path.parent_path();
    if(!fs::exists(par_path)){
      error = util::txt("The parent of the path should exist but, when setting \"",
                        path, "\"\nThe path \"", par_path, "\" does not exist.");
      return false;
    }
  }
  
  if(fmt.should_path_end_with_filename()){
    const string filename = path.filename().string();
    if(filename != fmt.get_path_filename()){
      error = util::txt("The path should end with the following file name: \"", 
                        fmt.get_path_filename(), "\"\nProblem: this is not the case for \"", 
                        path, "\"");
      return false;
    }
  }
  
  if(fmt.should_path_end_with()){
    const string end = fmt.get_path_ending();
    if(!str::ends_with(path.string(), end)){
      error = util::txt("The path should end with the following string: \"", 
                        end, "\"\nProblem: this is not the case for \"", path, "\".");
      return false;
    }
  }
  
  return true;
}

bool is_valid_shortcut(const string &x, string &error){
  ParsedShortcut shortcut(x);
  
  if(!shortcut.is_valid()){
    error = shortcut.get_error();
    return false;
  }
  
  return true;
}

inline bool is_valid_logical(const string &x){
  return x == "true" || x == "false";
}

bool is_valid_int(const string &x, const ArgumentFormat &fmt, string &error){
  
  if(x.empty()){
    error = "We expect an integer but the input is empty.";
    return false;
  }
  
  bool is_neg = x[0] == '-';
  if(is_neg && x.size() == 1){
    error = "We expect an integer but the input is equal to '-'.";
    return false;
  }
  
  int n = x.size();
  int i = is_neg ? 1 : 0;
  for(; i < n ; ++i){
    if(x[i] < '0' || x[i] > '9'){
      error = util::txt("We expect an integer, hence the input ",
                        str::dquote(x), " is incorrect.\n",
                        "The character in position ", i, ", [", x[i], "], is invalid.");
      return false;
    }
  }
  
  if(fmt.should_int_be_positive() && is_neg){
    error = util::txt("We expect a positive integer, but the input ",
                        str::dquote(x), " is negative.");
    return false;
  }
  
  return true;
}

bool is_valid_format(string &value_clean, const ArgumentFormat &fmt, string &error){
  // we set the value
  const string value = value_clean;
  
  if(fmt.is_color()){
    value_clean = str::to_lower(value_clean);
    if(!is_valid_color(value_clean)){
      error = util::txt(
        "The color \"", value, "\" is invalid.\n",
        "It should be either an HTML color name in snake case (see https://www.w3schools.com/tags/ref_colornames.asp)\n",
        "Either a color in hexadecimal form: \"#rrggbb\""
      );
      return false;
    }
    
  } else if(fmt.is_path()){
    if(!is_valid_path(value_clean, fmt, error)){
      return false;
    }
    
  } else if(fmt.is_shortcut()){
    if(!is_valid_shortcut(value, error)){
      return false;
    }
    
  } else if(fmt.is_logical()){
    if(!is_valid_logical(value_clean)){
      error = util::txt("This option must be logical (i.e. `true` or `false`), hence ",
                        str::bquote(value_clean), "is invalid.");
      return false;
    }
    
  } else if(fmt.is_int()){
    if(!is_valid_int(value_clean, fmt, error)){
      return false;
    }
    
  } else if(fmt.is_string()){
    // no check: freeform
    return true;
  }
  
  return true;
}

bool is_valid_option_line(const string &x, const options_fmt_t &all_opts_fmt, 
                          string &key, string &key_clean, string &value, string &error){
  
  const int n = x.size();
  int i = 0;
  
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  
  while(i < n && (str::is_ascii_letter(x[i]) || x[i] == '_' || x[i] == '.' || x[i] == '+')){
    key += x[i++];
  }
  
  key_clean = key;
  if(!is_valid_key(key, key_clean, all_opts_fmt, error)){
    return false;
  }
  
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  if(i >= n || x[i] != '='){
    error = util::txt("The options must be of the format key = value. ",
                      "The current line does not contain equal:\n",
                      str::dquote(x));
    return false;
  }
  
  // we move past the '='
  ++i;
  
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  while(i < n){
    value += x[i++];
  }
  
  if(value.empty()){
    error = util::txt("The options must be of the format key = value. ",
                      "The current line has no value set:\n",
                      str::dquote(x));
    return false;
  }
  
  value = str::trim_WS_rm_quotes(value);
  
  const ArgumentFormat &fmt = all_opts_fmt.at(key_clean);
  if(!is_valid_format(value, fmt, error)){
    return false;
  }
  
  return true;
}

} // end anonymous namespace

//
// ProgramOptions --------------------------------------------------------------
//


void ProgramOptions::set_paths(const string &prog_name){
  global_path = get_path_to_program_global_options(prog_name);
  local_path = get_path_to_program_local_options(prog_name);
  is_initialized = true;
}

ProgramOptions& ProgramOptions::add_options_fmt(const options_fmt_t &new_opts){
  
  for(const auto &it : new_opts){
    all_options_format[it.first] = it.second;
    if(it.second.has_default()){
      // we parse the option if it has a default
      all_options[it.first] = ParsedArg(it.second.get_default(), it.second, util::DoCheck(true));
    }
  }
  
  return *this;
}

ProgramOptions& ProgramOptions::read_options(){
  
  if(!is_initialized){
    throw util::internal_error(
      "read_options(): You must set the program before using the options.",
      " Please use the method ProgramOptions.set_program(string prog_name)."
    );
  }
  
  vector<fs::path> all_paths = {global_path, local_path};
  string global_local = "global";
  
  for(const auto &path : all_paths){
    
    if(!fs::exists(path)){
      continue;
    }
    
    std::ifstream options_file_in(path);
    
    bool first_error = true;
    
    int line_nb = 0;
    int line_nb_end = 0;
    if(options_file_in.is_open()){
      string line;
      while(util::get_full_line(options_file_in, line, line_nb, line_nb_end)){
        
        if(str::no_nonspace_char(line)){
          // empty lines
          continue;
        } else if(line.front() == '#'){
          // comments
          continue;
        } else {
          // should be key = value
          
          line_nb_end = line_nb;
          util::merge_extra_lines_if_needed(options_file_in, line, line_nb_end);
          
          string key, key_clean, value, line_error;
          if(!is_valid_option_line(line, all_options_format, key, key_clean, value, line_error)){
            if(first_error){
              first_error = false;
              util::error_msg("Problem in line ", line_nb, " of the ", global_local, 
                              " options file located at:\n", util::format_path(path), 
                              "\n", line_error);
            } else {
              util::error_msg("Problem in line ", line_nb, " of the ", global_local, 
                              " options file.\n", line_error);
            }
            
          } else {
            const ArgumentFormat &fmt = all_options_format[key_clean];
            all_options[key] = ParsedArg(value, fmt);
          }
          
          line_nb = line_nb_end;
        }
      }
      
      options_file_in.close();
    }
    
    global_local = "local";
  }
  
  return *this;
}

const ParsedArg& ProgramOptions::set_option(const string &key, const string &value, const TYPE type){
  
  if(!is_initialized){
    throw util::internal_error(
      "set_option(", key, ", ", value, "): You must set the program before using the options.",
      " Please use the method ProgramOptions.set_program(string prog_name)."
    );
  }
  
  //
  // step 1: validation
  //
  
  //
  // validate key
  //
  
  string key_clean;
  string key_error;
  if(!is_valid_key(key, key_clean, all_options_format, key_error)){
    util::error_msg(key_error);
    return empty_option;
  }
  
  //
  // validate format
  //
  
  const ArgumentFormat &fmt = all_options_format[key_clean];
  // value_clean will be modified in is_valid_format()
  string value_clean = value;
  string fmt_error;
  
  if(!is_valid_format(value_clean, fmt, fmt_error)){
    util::error_msg("When setting the option ", str::dquote(key), ". ", fmt_error);
    return empty_option;
  }
  
  // if memory only, we return directly
  all_options[key] = ParsedArg(value_clean, fmt);
  if(type == TYPE::TEMP){
    return all_options[key];
  }
  
  //
  // step 2: reading the existing otpions
  //
  
  fs::path path = type == TYPE::LOCAL ? local_path : global_path;
  
  vector<string> all_lines;
  string line;
  int pos = -1;
  int i = 0;
  
  if(fs::exists(path)){
    std::ifstream options_file_in(path);
    if(options_file_in.is_open()){
      while(std::getline(options_file_in, line)){
        all_lines.push_back(line);
        
        if(str::starts_with(line, key)){
          pos = i;
          
          // we remove the extra lines if needed
          util::merge_extra_lines_if_needed(options_file_in, line);
        }
        
        ++i;
      }
    }
  }
  
  
  //
  // step 3: writing the option 
  //
  
  
  std::ofstream options_file_out{path};
  
  if(!options_file_out.is_open()){
    util::error_msg("Could not write into the existing options located at:\n",
                    path.string());
    return all_options[key];
  }
  
  string option_line = key + " = " + str::dquote(value_clean);
  
  if(pos == -1){
    all_lines.push_back(option_line);
    all_lines.push_back("");
    
  } else {
    all_lines[pos] = option_line;
    
  }
  
  // now we write
  
  for(const auto &s : all_lines){
    options_file_out << s << "\n";
  }
  
  if(all_lines.back() != ""){
    options_file_out << "\n";
  }
  
  options_file_out.close();
  
  return all_options[key];
}

void ProgramOptions::reset_options(const string &key){
  
  string key_clean, error;
  if(!is_valid_key(key, key_clean, all_options_format, error)){
    util::error_msg("Error reset(", str::dquote(key), "): ", error);
    return;
  }
  
  const ArgumentFormat &fmt = all_options_format[key];
  
  if(!fmt.has_default()){
    util::error_msg("Only the options with a default can be reset. ",
                    "Problem: option ", str::dquote(key), " has no default.");
    return;
  }
  
  
  //
  // step 1: removing the key from the files 
  //
  
  
  // we first delete from the local, and then, if needed, from the global
  vector<fs::path> all_paths = {local_path, global_path};
  std::map<fs::path, string> global_local = {{local_path, "local"}, {global_path, "global"}};
  
  vector<string> all_lines;
  string line_found;
  fs::path path_found;
  bool any_found = false;
  for(const auto &path : all_paths){
    
    if(!fs::exists(path)){
      continue;
    }
    
    all_lines.clear();
    
    std::ifstream options_file_in(path);
    
    if(options_file_in.is_open()){
      string line;
      while(std::getline(options_file_in, line)){
        
        if(str::starts_with(line, key)){
          any_found = true;
          
          // we merge the multilines if needed
          util::merge_extra_lines_if_needed(options_file_in, line);
          
          line_found = line;
          path_found = path;
        } else {
          all_lines.push_back(line);
        }
      }
      
      options_file_in.close();
    }
    
    if(any_found){
      break;
    }
  }
  
  //
  // step 2: rewriting the file, if the line was found 
  //
  
  if(any_found){
    
    std::ofstream options_file_out(path_found);
    
    if(options_file_out.is_open()){
      
      for(const auto &s : all_lines){
        options_file_out << s << "\n";
      }
      
      if(all_lines.back() != ""){
        options_file_out << "\n";
      }
      
      options_file_out.close();
      
      util::info_msg("The line below: \n", 
                     line_found,
                     "\nwas successfully removed from the ", global_local[path_found], " options file, located at:\n",
                     path_found);
      
    }
  }
  
  //
  // step 3: resetting the option 
  //
  
  all_options[key] = ParsedArg(fmt);
  
}

const ParsedArg& ProgramOptions::get_option(const string &key, const util::DoCheck options) const {
  
  bool option_exists = util::map_contains(all_options, key);
  
  if(!option_exists){
    if(options.is_check()){
      string error = util::txt("get_error(", str::dquote(key), "): This option has not been set.",
                               " Please set this option before accessing it.");
      if(options.is_set_error()){
        options.set_error(error);
      } else {
        throw util::internal_error(error);
      }
    }
    
    return empty_option;
  }
  
  return all_options.at(key);
}

const ArgumentFormat& ProgramOptions::get_option_format(const string &key, const util::DoCheck options) const {
  
  string key_clean = key;
  string error;
  if(is_valid_key(key, key_clean, all_options_format, error)){
    return all_options_format.at(key_clean);
  }
  
  if(options.is_check()){
    string error = util::txt("get_error(", str::dquote(key), "): This option has not been set.",
                              " Please set this option before accessing it.");
    if(options.is_set_error()){
      options.set_error(error);
    } else {
      throw util::internal_error(error);
    }
  }
    
    return empty_option_format;
}

bool ProgramOptions::is_key_settable(const string &key) const {
  
  string key_clean = key;
  string error;
  bool res = is_valid_key(key, key_clean, all_options_format, error);
  if(key_clean == "freeform"){
    res = false;
  }
  
  return res;
}

inline bool ProgramOptions::is_unset_option(const string &key) const {
  
  if(!util::map_contains(all_options, key)){
    if(util::map_contains(all_options_format, key)){
      return true;
    }
    
    util::error_msg("In get_color(", str::dquote(key), "): the key has no format attached => this is an internal error.");
    return true;
  }
  
  return false;
}

string ProgramOptions::get_color(const string &key) const {
  
  if(is_unset_option(key)){
    util::error_msg("The option ", str::dquote(key), " is unset");
    return VTS::FG_DEFAULT;
  }
  
  return all_options.at(key).get_color();
}

ParsedShortcut ProgramOptions::get_shortcut(const string &key) const {
  if(is_unset_option(key)){
    util::error_msg("The option ", str::dquote(key), " is unset");
    return ParsedShortcut();
  }
  
  return all_options.at(key).get_shortcut();
}


vector<string> ProgramOptions::get_subkeys(const string &key) const {
  vector<string> res;
  
  const vector<string> all_keys = util::map_names(all_options_format);
  for(const auto &full_key : all_keys){
    if(str::starts_with(full_key, key)){
      res.push_back(str::delete_until(full_key, "."));
    }
  }
  
  return res;
}

vector<string> ProgramOptions::get_key_roots() const {
  
  vector<string> all_keys = util::map_names(all_options_format);
  
  for(auto &full_key : all_keys){
    if(str::str_contains(full_key, '.')){
      full_key = str::delete_after(full_key, ".");
    }
  }
  
  util::vector_sort_unique(all_keys);
  
  return all_keys;
}



//
// ParsedArg ----------------------------------------------------------------
//


ParsedArg::ParsedArg(const ArgumentFormat &fmt){
  
  if(!fmt.has_default()){
    util::error_msg("Internal error: In ParsedArg(fmt): the current format has no default. ",
                    "You need to check beforehand before calling this constructor.");
    return;
  }
  
  string err;
  initialize(fmt.get_default(), fmt, util::DoCheck(err));
  
  if(!err.empty()){
    util::error_msg("Internal error: In ParsedArg(fmt): the default value for an argument (or option) could not be parsed.\n",
                    "The argument was of type:\n", fmt.type_verbose(), "\n",
                    "The default value was:\n", fmt.get_default());
  }
  
  _is_default = true;
}

ParsedArg::ParsedArg(const string &x, const ArgumentFormat &fmt, const util::DoCheck options){
  initialize(x, fmt, options);
}

void ParsedArg::initialize(const string &x, const ArgumentFormat &fmt, const util::DoCheck options){
  // when here: the format MUST be valid! It is not rechecked
  
  input = x;
  
  if(options.is_check()){
    
    string error;
    string value_clean = x;
    if(!is_valid_format(value_clean, fmt, error)){
      
      if(options.is_set_error()){
        options.set_error(error);
        return;
      } else {
        util::error_msg(error);
        return;
      }
    }
  }
  
  if(fmt.is_color()){
    type = TYPE::COLOR;
    
    if(util::map_contains(VTS::all_html_colors, x)){
      color = VTS::all_html_colors.at(x);
      
    } else if(str::starts_with(x, "raw:")){
      // only used for debugging
      string col_clean = x.substr(4);
      color = "\033[38;2;" + col_clean + "m";
      
    } else {
      color = VTS::fg_rgb(x);
      
    }
    
    if(fmt.is_bg()){
      // we transform the VTS sequence from fg to bg
      
      if(color[2] == '3'){
        color[2] = '4';
        
      } else if(color[2] == '9'){
        color = "\033[10" + color.substr(3);
        
      } else {
        util::error_msg("Internal error: The VTS FG command is invalid.");
      }
      
    }
    
  }
  
  if(fmt.is_path()){
    type = TYPE::PATH;
    path = x;
  }
  
  if(fmt.is_shortcut()){
    type = TYPE::SHORTCUT;
    shortcut = ParsedShortcut(x);
  }
  
  if(fmt.is_logical()){
    type = TYPE::LOGICAL;
    logical = x == "true";
  }
  
  if(fmt.is_int()){
    type = TYPE::INT;
    number = std::stoi(x);
  }
  
  if(fmt.is_string()){
    type = TYPE::STRING;
    // the string is not modified, and is stored in 'inpu't
  }
  
  if(fmt.has_hook()){
    fmt.run_hook(this);
  }
  
}


//
// ParsedCommandArguments ------------------------------------------------------
//


ParsedCommandArguments::ParsedCommandArguments(const string &x){
  
  int i = 0;
  int n = x.size();
  bool cursor_outside = true;
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  while(i < n){
    
    string arg;
    const bool in_path = str::is_quote(x[i]);
    if(in_path){
      
      char quote = x[i++];
      while(i < n && !(x[i] == quote && !str::is_escaped(x, i))){
        arg += x[i++];
      }
      
      if(i < n){
        // we go past the quote
        ++i;
      } else {
        cursor_outside = false;
      }
      
    } else {
      
      while(i < n && x[i] != ' '){
        arg += x[i++];
      }
    }
    
    string space;
    while(i < n && x[i] == ' '){
      ++i;
      space += " ";
    }
    
    all_args.push_back(CommandArg(arg, in_path, space));
    
  }
  
  // is the cursor attached to the last character?
  if(cursor_outside){
    if(n > 0 && (x.back() == ' ' || str::is_quote(x.back()))){
      cursor_outside = true;
    } else {
      cursor_outside = false;
    }
  }
  
  if(cursor_outside){
    all_args.push_back(CommandArg("", false, ""));
  }
  
}

