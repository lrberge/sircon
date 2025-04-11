    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "util.hpp"
#include "stringtools.hpp"
#include <string>
#include <vector>

using std::string;
using stringtools::StringMatch;
namespace str = stringtools;


class ConsoleCommand;

//
// functions ------------------------------------------------------------------- 
//


str::MetaStringVec suggest_path(string &context, const bool add_quotes = false);


//
// AutocompleteContext --------------------------------------------------------- 
//


struct AutocompleteContext {
  string before_cursor;
  string after_cursor;
  string path_query;
  bool is_in_path;
};

//
// AutocompChoices ------------------------------------------------------------- 
//

class AutocompChoices : public str::MetaStringVec {
public:
  
  using STYPE = str::Meta::SCALAR_TYPE;
  
  AutocompChoices() = default;
  
  AutocompChoices(const str::vec_str &x){
    str::vec_str &&v = static_cast<str::vec_str>(x);
    set_string_vector(v);
  }
  
  AutocompChoices(str::vec_str &&x){
    set_string_vector(x);
  }
  
  AutocompChoices(const MetaStringVec &x): MetaStringVec(x){}
  
  AutocompChoices& push_back(AutocompChoices& x){
    str::MetaStringVec::push_back(x);
    return *this;
  }
  
  AutocompChoices& set_continue(const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("continue", "true", stype);
    return *this;
  }
  
  AutocompChoices& set_cursor_shift(int i, const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("cursor_shift", std::to_string(i), stype);
    return *this;
  }
  
  AutocompChoices& set_n_delete_left(int i, const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("n_delete_left", std::to_string(i), stype);
    return *this;
  }
  
  AutocompChoices& set_append_left(const string x, const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("append_left", x, stype);
    return *this;
  }
  
  AutocompChoices& set_append_right(const string x, const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("append_right", x, stype);
    return *this;
  }
  
  AutocompChoices& set_add_quotes(const STYPE stype = STYPE::INHERIT){
    str::MetaStringVec::set_meta("add_quotes", "true", stype);
    return *this;
  }
  
  AutocompChoices& set_labels(const vector<string> &labels){
    str::MetaStringVec::set_meta("labels", labels);
    return *this;
  }
  
  AutocompChoices& set_info(const vector<string> &info){
    str::MetaStringVec::set_meta("info", info);
    return *this;
  }
};

//
// AutocompleteResult ----------------------------------------------------------
//


class AutocompleteResult {
  str::MetaString final_choice;
  int _cursor_shift = 0;
  bool _continue = false;
  size_t _n_delete_left = 0;
  
public:
  
  AutocompleteResult(const str::MetaString &choice):
    final_choice(choice) 
  {
    
    if(final_choice.has_key("continue")){
      _continue = final_choice.get_meta("continue") == "true";
    }
    
    if(final_choice.has_key("cursor_shift")){
      const string &val = final_choice.get_meta("cursor_shift");
      if(val.empty()){
        _cursor_shift = 0;
      } else {
        _cursor_shift = std::stoi(val);
      }
    }
    
    if(final_choice.has_key("n_delete_left")){
      const string &val = final_choice.get_meta("n_delete_left");
      if(val.empty()){
        _n_delete_left = 0;
      } else {
        _n_delete_left = std::stoi(val);
      }
    }
    
    if(final_choice.has_key("append_right")){
      string val = final_choice.get_meta("append_right");
      if(!val.empty()){
        final_choice.set_string(final_choice.get_string() + val);
      }
    }
    
    if(final_choice.has_key("append_left")){
      string val = final_choice.get_meta("append_left");
      if(!val.empty()){
        final_choice.set_string(val + final_choice.get_string());
      }
    }
    
    if(final_choice.has_key("add_quotes")){
      string val = final_choice.get_meta("add_quotes");
      if(val == "true"){
        final_choice.set_string(str::dquote(final_choice.get_string()));
      }
    }
    
  };
  
  string get_string() const { return final_choice.get_string(); }
  
  int cursor_shift() const { return _cursor_shift; }
  
  bool continue_autocomp() const { return _continue; }
  
  size_t n_delete_left() const { return _n_delete_left; }
  
  AutocompleteResult& set_shift(const int i){
    _cursor_shift = i;
    return *this;
  }
  
  AutocompleteResult& set_continue(const bool x){
    _continue = x;
    return *this;
  }
  
  AutocompleteResult& set_n_delete_left(const int n){
    if(n < 0){
      throw util::bad_type("The value of n, here ", n, ", cannot be negative.");
    }
    _n_delete_left = n;
    return *this;
  }
  
  bool has_key(const std::string &key) const {
    return final_choice.has_key(key);
  }
  
  template<typename T>
  T get_meta(const std::string &key) const {
    
    if(!has_key(key)){
      throw util::bad_type("meta(\"", key, "\"): a valid key must be provided. Else use this function with a default.");
    }
    
    T res;
    
    if constexpr (std::is_integral_v<T>){
      res = std::stoi(final_choice.get_meta(key));
      
    } else if constexpr (std::is_floating_point_v<T>){
      res = std::stod(final_choice.get_meta(key));
      
    } else if constexpr (std::is_same_v<T, std::string>){
      res = final_choice.get_meta(key);
      
    } else {
      throw util::bad_type("get_meta(\"", key, "\"): The current type is not supported.");
    }
    
    return res;
  }
  
  template<typename T>
  T get_meta(const std::string &key, T _default) const {
    
    if(!has_key(key)){
      return _default;
    }
    
    return get_meta<T>(key);
  }
  
  std::string get_meta_str(const std::string &key, std::string _default = ""){
    
    if(!has_key(key)){
      return _default;
    }
    
    return final_choice.get_meta(key);
  }
  
};


//
// ServerAutocomplete ----------------------------------------------------------
//


class ServerAutocomplete {
  
str::StringMatch suggest_path(const string &);
  
public:
  
  virtual StringMatch make_suggestions(const AutocompleteContext &context){
    if(context.is_in_path){
      return suggest_path(context.path_query);
    }
    
    return StringMatch();
  }
  
  virtual StringMatch update_suggestions([[maybe_unused]] const char c){
    return StringMatch().set_cause_no_match("unavailable");
  }
  
  virtual AutocompleteResult finalize_autocomplete(const str::MetaString &x){
    return AutocompleteResult(x);
  }
  
  virtual void quit_autocomp(){}
};


//
// LanguageServer --------------------------------------------------------------
//


struct CmdParsed {
  bool is_continuation = false;
  string indent;
  bool is_error = false;
  string error_msg = "";
  vector<uint> error_location;
};

class LanguageServer {
  
  ConsoleCommand *pcon;
  
public:
  
  LanguageServer(){ }
  
  virtual ~LanguageServer(){};
  
  virtual CmdParsed parse_command(const string &cmd){
    int i = cmd.size() - 1;
    str::move_i_to_non_WS_if_i_WS(cmd, i, SIDE::LEFT);
    
    CmdParsed res;
    res.is_continuation = i >= 0 && cmd[i] == '{';
    
    return res;
  }
  
  virtual vector<string> autocomplete([[maybe_unused]] const string &context){
    // no default implemented
    
    vector<string> res;
    return res;
  }
  
  virtual void resize_window_width(uint width){
    if(width > 0){
      // do something, to be implemented in custom classes
    }
  }
  
  virtual bool is_line_comment([[maybe_unused]] const string &str){
    return false;
  }
  
};

//
// ConsoleAutocomplete ----------------------------------------------------------------
//

class ConsoleAutocomplete {
  
  ConsoleCommand *pconcom;
  
  StringMatch all_matches;
  vector<string> lines_fmt;
  uint index = 0;
  uint screen_start = 0;
  uint n_matches = 0;
  uint n_display = 0;
  bool is_empty = true;
  bool is_active = false;
  bool in_path = false;
  
  const uint max_height = 7;
  const uint default_max_width = 65;
  
  vector<string> gen_scrollbar(uint, uint);
  uint get_max_width();
  
public:
  
  ConsoleAutocomplete() = default;
  
  void set_console(ConsoleCommand* x) { pconcom = x; };
  void set_matches(StringMatch x, bool in_path = false);
  
  void move(int);
  void display(bool init = false);
  void clear();
  void reset();
  
  bool is_in_path() const { return in_path; }
  bool no_suggests() const { return is_empty; }
  
  string get_target() const {
    return all_matches.get_target();
  }
  
  AutocompleteResult get_selection() const;
  
  uint get_selection_id() const {
    return all_matches.empty() ? 0 : all_matches.id_at(index);
  }
  
  bool is_identity_suggest() const {
    return n_matches == 1 && all_matches.get_target() == all_matches.string_at(0);
  }
  
};


