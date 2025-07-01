    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "pathmanip.hpp"
#include "stringtools.hpp"
#include "util.hpp"
#include "VTS.hpp"
#include "shortcuts.hpp"
#include <string>
#include <map>
#include <filesystem>
#include <iostream>

using std::string;
namespace fs = std::filesystem;
namespace str = stringtools;


//
// ArgumentFormat --------------------------------------------------------------
//

class AutocompChoices;
class ConsoleCommand;
class LanguageServer;
class ParsedArg;

class ArgumentFormat {
  
public:
  
  enum class TYPE {
    COLOR,
    PATH,
    SHORTCUT,
    LOGICAL,
    INT,
    STRING,
  };
  
  ArgumentFormat() = default;
  ArgumentFormat(const TYPE &x): type(x) {};
  ArgumentFormat(const ArgumentFormat &x) = default;
  
  bool is_color()    const { return type == TYPE::COLOR; }
  bool is_path()     const { return type == TYPE::PATH; }
  bool is_shortcut() const { return type == TYPE::SHORTCUT; }
  bool is_logical()  const { return type == TYPE::LOGICAL; }
  bool is_int()      const { return type == TYPE::INT; }
  bool is_string()   const { return type == TYPE::STRING; }
  
  string type_verbose() const {
    switch(type){
      case TYPE::COLOR: return "color";
      case TYPE::PATH: return "path";
      case TYPE::SHORTCUT: return "shortcut";
      case TYPE::LOGICAL: return "logical";
      case TYPE::INT: return "int";
      case TYPE::STRING: return "string";
    }
    
    return "TYPE ERROR";
  }
  
  //
  // color 
  //
  
  ArgumentFormat& bg(){
    _is_background_color = true;
    return *this;
  }
  
  bool is_bg() const {
    return _is_background_color;
  }
  
  
  //
  // path 
  //
  
  ArgumentFormat& path_must_end_with(const string &x){
    _path_ends_with = x;
    return *this;
  }
  
  string get_path_ending() const { return _path_ends_with; }
  bool should_path_end_with() const { return !_path_ends_with.empty(); }
  
  ArgumentFormat& path_must_end_with_filename(const string &x){
    _path_filename = x;
    return *this;
  }
  
  string get_path_filename() const { return _path_filename; }
  bool should_path_end_with_filename() const { return !_path_filename.empty(); }
  
  ArgumentFormat& path_must_exist(){
    _path_exists = true;
    return *this;
  }
  
  bool should_path_exist() const { return _path_exists; }
  
  ArgumentFormat& path_parent_must_exist(){
    _path_parent_exists = true;
    return *this;
  }
  
  bool should_path_parent_exist() const { return _path_parent_exists; }
  
  bool should_path_be_absolute() const { return _path_make_absolute; }
  void make_path_absolute() {
    _path_make_absolute = true;
  }
  
  //
  // freeform 
  //
  
  ArgumentFormat& freeform_key_must_start_with(const string &x){
    _freeform_start = x;
    return *this;
  }
  
  string get_freeform_key_start() const { return _freeform_start; }
  bool should_freeform_key_start_with() const { return !_freeform_start.empty(); }
  
  //
  // int 
  //
  
  ArgumentFormat& int_must_be_positive(){
    _int_positive = true;
    return *this;
  }
  bool should_int_be_positive() const { return _int_positive; }
  
  //
  // suggestion 
  //
  
  ArgumentFormat& set_suggestion(std::shared_ptr<AutocompChoices> px){
    _has_suggestion = true;
    _suggest_vec = px;
    return *this;
  }
  
  ArgumentFormat& set_suggestion(std::function<std::shared_ptr<AutocompChoices>(ConsoleCommand *)> fun){
    _has_suggestion = true;
    _suggest_fun = fun;
    return *this;
  }
  
  bool has_suggestion() const {
    return _has_suggestion;
  }
  
  std::shared_ptr<AutocompChoices> get_suggestion(ConsoleCommand * pconcom) const {
    if(_suggest_vec){
      return _suggest_vec;
    } else {
      return _suggest_fun(pconcom);
    }
  }
  
  //
  // hook 
  //
  
  ArgumentFormat& set_hook(ConsoleCommand *pconcom_in, 
                          std::function<void(ConsoleCommand*, ParsedArg*)> fun){
    _has_hook = true;
    pconcom = pconcom_in;
    pfun_cons = fun;
    return *this;
  }
  
  ArgumentFormat& set_hook(LanguageServer *plang_in, 
                          std::function<void(LanguageServer*, ParsedArg*)> fun){
    
    _has_hook = true;
    plang = plang_in;
    pfun_serv = fun;
    return *this;
  }
  
  ArgumentFormat& set_hook(ConsoleCommand *pconcom_in, LanguageServer *plang_in, 
                          std::function<void(ConsoleCommand*, LanguageServer*, ParsedArg*)> fun){
    
    _has_hook = true;
    pconcom = pconcom_in;
    plang = plang_in;
    pfun_cons_serv = fun;
    return *this;
  }
  
  bool has_hook() const {
    return _has_hook;
  }
  
  void run_hook(ParsedArg *opt) const {
    if(pfun_cons){
      pfun_cons(pconcom, opt);
      return;
    }
    
    if(pfun_serv){
      pfun_serv(plang, opt);
      return;
    }
    
    if(pfun_cons_serv){
      pfun_cons_serv(pconcom, plang, opt);
      return;
    }
  }
  
  //
  // default
  //
  
  ArgumentFormat& set_default(const string &x){
    _default_value = x;
    return *this;
  }
  
  bool has_default() const {
    return !util::is_unset(_default_value);
  }
  
  string get_default() const {
    return _default_value;
  }
  
private:
  TYPE type = TYPE::COLOR;

  string _path_ends_with;
  string _path_filename;
  bool _path_exists = false;
  bool _path_parent_exists = false;
  bool _path_make_absolute = false;
  
  bool _is_background_color = false;
  
  bool _int_positive = false;
  
  string _freeform_start;
  
  string _default_value = UNSET::STRING;
  
  bool _has_suggestion = false;
  std::shared_ptr<AutocompChoices> _suggest_vec = nullptr;
  std::function<std::shared_ptr<AutocompChoices>(ConsoleCommand *)> _suggest_fun = nullptr;
  
  // hook
  bool _has_hook = false;
  ConsoleCommand *pconcom = nullptr;
  LanguageServer *plang = nullptr;
  std::function<void(ConsoleCommand*, ParsedArg*)> pfun_cons = nullptr;
  std::function<void(LanguageServer*, ParsedArg*)> pfun_serv = nullptr;
  std::function<void(ConsoleCommand*, LanguageServer*, ParsedArg*)> pfun_cons_serv = nullptr;
  
};


//
// ParsedArg ---------------------------------------------------------------- 
//

class ParsedArg {
  string input;
  string color;
  fs::path path;
  ParsedShortcut shortcut;
  bool logical = false;
  int number = 0;
  bool _is_default = false;
  
  enum class TYPE {
    COLOR,
    PATH,
    SHORTCUT,
    LOGICAL,
    INT,
    STRING,
    UNSET,
  };
  
  TYPE type = TYPE::UNSET;
  
public:
  
  ParsedArg() = default;
  ParsedArg(const string &, const ArgumentFormat &, const util::DoCheck opts = util::DoCheck(false));
  ParsedArg(const ArgumentFormat &);
  
  void initialize(const string &x, const ArgumentFormat &fmt, const util::DoCheck options);
  
  string get_input() const { return input; }
  
  string         get_color()    const { return color; }
  fs::path       get_path()     const { return path; }
  ParsedShortcut get_shortcut() const { return shortcut; }
  bool           get_logical()  const { return logical; }
  int            get_int()      const { return number; }
  string         get_string()   const { return input; }
  
  bool is_color()    const { return type == TYPE::COLOR; }
  bool is_path()     const { return type == TYPE::PATH; }
  bool is_shortcut() const { return type == TYPE::SHORTCUT; }
  bool is_logical()  const { return type == TYPE::LOGICAL; }
  bool is_int()      const { return type == TYPE::INT; }
  bool is_string()   const { return type == TYPE::STRING; }
  bool is_unset()    const { return type == TYPE::UNSET; }
  
  bool is_default()  const { return _is_default; }
  
};

//
// argtype ---------------------------------------------------------------
//


namespace argtype {
// option formatting

class COLOR : public ArgumentFormat {
public:
  COLOR(): ArgumentFormat(ArgumentFormat::TYPE::COLOR){};
  COLOR(const string &x): ArgumentFormat(ArgumentFormat::TYPE::COLOR){
    this->set_default(x);
  };
};

class PATH : public ArgumentFormat {
public:
  PATH(): ArgumentFormat(ArgumentFormat::TYPE::PATH){};
  PATH(const string &x): ArgumentFormat(ArgumentFormat::TYPE::PATH){
    this->set_default(x);
  };
};

class SHORTCUT : public ArgumentFormat {
public:
  SHORTCUT(): ArgumentFormat(ArgumentFormat::TYPE::SHORTCUT){};
  SHORTCUT(const string &x): ArgumentFormat(ArgumentFormat::TYPE::SHORTCUT){
    this->set_default(x);
  };
};

class LOGICAL : public ArgumentFormat {
public:
  LOGICAL(): ArgumentFormat(ArgumentFormat::TYPE::LOGICAL){};
  LOGICAL(const string &x): ArgumentFormat(ArgumentFormat::TYPE::LOGICAL){
    this->set_default(x);
  };
};

class INT : public ArgumentFormat {
public:
  INT(): ArgumentFormat(ArgumentFormat::TYPE::INT){};
  INT(const string &x): ArgumentFormat(ArgumentFormat::TYPE::INT){
    this->set_default(x);
  };
};

class STRING : public ArgumentFormat {
public:
  STRING(): ArgumentFormat(ArgumentFormat::TYPE::STRING){};
  STRING(const string &x): ArgumentFormat(ArgumentFormat::TYPE::STRING){
    this->set_default(x);
  };
};

} // namespace argtype

using options_fmt_t = std::map<string, ArgumentFormat>;
using options_t = std::map<string, ParsedArg>;


//
// ProgramOptions -------------------------------------------------------------- 
//

class ProgramOptions {
  fs::path global_path;
  fs::path local_path;
  options_fmt_t all_options_format;
  options_t all_options;
  bool is_initialized = false;
  ParsedArg empty_option;
  ArgumentFormat empty_option_format;
  
  void set_paths(const string &);
  bool is_unset_option(const string &) const;
  
public:
  
  enum class TYPE {
    LOCAL,
    GLOBAL,
    TEMP,
  };
  
  ProgramOptions() = default;
  
  ProgramOptions(const string &prog_name){
    set_paths(prog_name);
  }
  
  ProgramOptions(const string &prog_name, const options_fmt_t &opts): all_options_format(opts){
    set_paths(prog_name);
  };
  
  ProgramOptions& set_program(const string &prog_name){
    set_paths(prog_name);
    return *this;
  }

  ProgramOptions& add_options_fmt(const options_fmt_t &);

  ProgramOptions& read_options();
  
  void reset_options(const string &key);
  
  const ParsedArg& set_option(const string &, const string &, const TYPE);
  
  const ParsedArg& set_option(const string &key, const string &value, const string &type){
    TYPE write_type = type == "set" ? TYPE::TEMP : (type == "set_local" ? TYPE::LOCAL : TYPE::GLOBAL);
    return set_option(key, value, write_type);
  }
  
  const ParsedArg& get_option(const string &, const util::DoCheck options = util::DoCheck(false)) const;
  
  const ArgumentFormat& get_option_format(const string &, const util::DoCheck options = util::DoCheck(false)) const;
  
  const options_fmt_t get_all_options_format() const { return all_options_format; }
  
  bool has_option(const string &key) const { return util::map_contains(all_options, key); }
  
  bool is_key_settable(const string &key) const;
  
  bool key_has_option(const string &key) const { return util::map_contains(all_options, key); };
  
  vector<string> names() const { return util::map_names(all_options_format); }
  
  const string get_global_path() const { return global_path.string(); }
  const string get_local_path() const { return local_path.string(); }
  
  //
  // nested options 
  //
  
  vector<string> get_key_roots() const;
  vector<string> get_subkeys(const string &key) const;
  
  //
  // specific options
  //
  
  string get_color(const string &key) const;
  ParsedShortcut get_shortcut(const string &key) const;
  
};


//
// ParsedCommandArguments ------------------------------------------------------- 
//

class CommandArg {
  std::string arg;
  bool _is_quoted = false;
  std::string _trailing_space;
public:
  CommandArg() = default;
  CommandArg(const string &x, bool y): arg(x), _is_quoted(y) {}
  CommandArg(const string &x, bool y, const string &space): 
    arg(x), _is_quoted(y), _trailing_space(space) {}
  
  bool is_quoted() const { return _is_quoted; }
  void set_space(const string &space) { _trailing_space = space; }
  
  std::string string() const { return arg; }
  std::string trailing_space() const { return _trailing_space; }
  std::string full_string() const { return arg + _trailing_space; }
};

class ParsedCommandArguments {
  vector<CommandArg> all_args;
  
  vector<string> to_string_vector(const vector<CommandArg> &x) const {
    vector<string> res;
    for(const auto &arg : x){
      res.push_back(arg.string());
    }
    return res;
  }
  
public:
  ParsedCommandArguments(const string &x);
  
  CommandArg &operator[](int i){ return all_args[i]; }
  const CommandArg at(int i) const { return all_args[i]; }
  bool empty() const { return all_args.empty(); }
  int size() const { return all_args.size(); }
  CommandArg &back() { return all_args.back(); }
  vector<string> get_arg_string_vector() const {
    return to_string_vector(all_args);
  }
  
};


//
//  utilities
//

inline string arg_signature(const vector<ArgumentFormat> &all_args){
  
  if(all_args.empty()){
    return "";
  }
  
  string sign;
  
  for(size_t i = 0 ; i < all_args.size() ; ++i){
    const ArgumentFormat &arg = all_args.at(i);
    sign += (i > 0 ? " " : "") + arg.type_verbose() + (arg.has_default() ? "?" : "");
  }
  
  return sign;
}


