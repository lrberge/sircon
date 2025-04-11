    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "util.hpp"
#include "stringtools.hpp"
#include "autocomplete.hpp"
#include "cache.hpp"
#include "R.hpp"
#include "to_index.hpp"
#include <map>
  
namespace str = stringtools;
using stringtools::StringMatch;
using util::msg;
using util::bad_type;
  

//
// Minor utilities -------------------------------------------------------------
//

enum class AC_FINALIZE: char {
  DEFAULT = 'a',
  FUNCTION,
  POSSIBLE_FUNCTION,
  QUOTE,
  PACKAGE_EXPORT,
  INTROSPECTION,
  PATH,
  NONE,
};

inline string finalize_to_string(const AC_FINALIZE x){
  return string{static_cast<char>(x)};
}

inline AC_FINALIZE string_to_finalize(const string x){
  // internal function: no need to check
  char c = x[0];
  return static_cast<AC_FINALIZE>(c);
}

enum class AC_TYPE {
  FUNCTION,
  FUNCTION_ARGUMENT,
  DATA_ARGUMENT,
  DOLLAR,
  AROBASE,
  NAMESPACE_EXPORTS,
  NAMESPACE_ALL,
  INTROSPECTION,
  VARIABLE,
  GLOBAL_ENV,
  STRING,
  STRING_INTERPOL,
  PACKAGE,
  TILDE,
  DEFAULT,
};

const std::map<char, string> CODE_VERBOSE{
  {'V', "Variables"},       // ctrl-V allowed
  {'K', "Packages"},        // ctrl-P forbidden
  {'C', "Functions"},       // ctrl-F forbidden
  {'A', "Arguments"},       // ctrl-A allowed
  {'D', "Default"},         // ctrl-D allowed
  {'O', "Global Env"},      // ctrl-G forbidden
};

const std::map<char, AC_TYPE> MAP_CODE_TYPE{
  {'V', AC_TYPE::VARIABLE},
  {'K', AC_TYPE::PACKAGE},
  {'C', AC_TYPE::FUNCTION},
  {'A', AC_TYPE::FUNCTION_ARGUMENT},
  {'D', AC_TYPE::DEFAULT},
  {'O', AC_TYPE::GLOBAL_ENV},
};

const string DEFAULT_CODES = "DVCOK";
const string DEFAULT_CODES_ARG = "DAVCOK";

//
// AC_Meta --------------------------------------------------------------------- 
//

class AC_Meta: public str::Meta {
public:
  
  AC_Meta() = default;
  
  AC_Meta(string final, str::vec_str labels): Meta("finalize", final){
    set("labels", labels);
  }
  
  AC_Meta& set_finalize(string final){
    set("finalize", final);
    return *this;
  }
  
  AC_Meta& set_labels(str::vec_str labels){
    set("labels", labels);
    return *this;
  }
  
};


//
// AC_String ------------------------------------------------------------------- 
//

class AC_String : public str::MetaStringVec {
public:
  
  AC_String() = default;
  
  AC_String(R::CPP_SEXP &&x){
    str::vec_str &&v = static_cast<str::vec_str>(x);
    set_string_vector(v);
  }
  
  AC_String(const str::vec_str &x){
    str::vec_str &&v = static_cast<str::vec_str>(x);
    set_string_vector(v);
  }
  
  AC_String(str::vec_str &&x){
    set_string_vector(x);
  }
  
  AC_String& set_finalize(AC_FINALIZE x){
    str::MetaStringVec::set_meta("finalize", finalize_to_string(x));
    return *this;
  }
  
  AC_String& operator=(R::CPP_SEXP &&x){
    str::vec_str &&v = static_cast<str::vec_str>(x);
    set_string_vector(v);
    return *this;
  }
  
  AC_String& push_back(AC_String& x){
    str::MetaStringVec::push_back(x);
    return *this;
  }
  
};


//
// ParsedVariable --------------------------------------------------------------
//

// bon@jour[[les]]$gens
// all_expr = bon  | jour    | les                   | gens
// all_type = ROOT | AROBASE | DOUBLE_SQUARE_BRACKET | DOLLAR

class ParsedVariable {
  
  enum class TYPE {
    ROOT,
    DOLLAR,
    AROBASE,
    DOUBLE_SQUARE_BRACKET,
    SINGLE_SQUARE_BRACKET,
  };
  
  vector<string> all_expr;
  vector<TYPE> all_type;
  
  bool is_set = false;
  bool _is_valid = false;
  bool _is_full_string = false;
  
  vector<string> current_names;
  
  string error;
  string full_expr;
  size_t full_expr_wide_width = 0;
  
  inline bool set_error(const string &);
  void init(const string &, int &, const int, const int);

public:
  
  ParsedVariable() = default;
  
  ParsedVariable(const ParsedVariable &x) = default;
  
  ParsedVariable(const string &x);
  
  ParsedVariable(const string &x, int &i, const int n, const int side = SIDE::LEFT);
  
  bool is_valid();
  
  string get_error() const { return error; }
  
  string get_data_name();
  
  vector<string> names();
  
  size_t get_expr_wide_width() const { return full_expr_wide_width; }
  
  bool is_full_string() const { return _is_full_string; }
  
};

//
// ParsedFunction -------------------------------------------------------------- 
//

class ParsedFunction {
  
  enum class TYPE {
    LOADED_FUN,
    DOUBLE_COLON,
    TRIPLE_COLON,
  };
  
  string pkg;
  string fun_name;
  string error;
  TYPE type = TYPE::LOADED_FUN;
  bool is_set = false;
  
public:
  
  ParsedFunction() = default;
  
  ParsedFunction(const string &x): fun_name(x), is_set(true) {};
  
  ParsedFunction(const string&, int&, const int, const int);
  
  bool exists();
  
  bool empty() const { return str::no_nonspace_char(fun_name); }
  
  bool is_from_namespace() const { return type != TYPE::LOADED_FUN; }
  
  string get_fun_name() const { return fun_name; }
  
  string get_complete_fun_name() const;
  
  string get_error() const { return error; }
  
};

//
// Container -------------------------------------------------------------------
//

class Container {
  vector<string> arg_values;
  vector<string> arg_names;
  int cursor_pos = 0;
  bool is_data = false;
  ParsedFunction function;
  string data_name;
  bool any_container = false;
  int pos_start = 0;
  int pos_end = 0;
public:
  Container() = default;
  Container(const string &, const int);
  
  bool is_container() const { return any_container; }
  
  ParsedFunction get_parsed_function() const { return function; }
  
  int get_cursor_position() const { return cursor_pos < 0 ? 0 : cursor_pos; }
  
  vector<string> get_arg_names() const { return arg_names; }
  
  vector<string> get_arg_values() const { return arg_values; }
  
  int get_pos_start() const { return pos_start; }
  
  string get_data_name() const { return data_name; }
  
  bool is_data_container() const { return is_data; }
  
  string get_cursor_arg_name() const { 
    return cursor_pos < 0 || arg_names.empty() ? UNSET::STRING : arg_names.at(cursor_pos); 
  }
  
  string get_cursor_arg_value() const { 
    return cursor_pos < 0 || arg_values.empty() ? UNSET::STRING : arg_values.at(cursor_pos); 
  }
  
  vector<string> get_previous_arg_names() const {
    if(cursor_pos <= 0){
      return vector<string>();
    }
    
    vector<string> res;
    res.insert(res.begin(), arg_names.begin(), arg_names.begin() + cursor_pos);
    
    return res;
  }
  
  vector<string> get_previous_arg_values() const {
    if(cursor_pos <= 0){
      return vector<string>();
    }
    
    vector<string> res;
    res.insert(res.begin(), arg_values.begin(), arg_values.begin() + cursor_pos);
    
    return res;
  }
  
  vector<string> get_other_arg_names() const {
    if(cursor_pos <= -1){
      return vector<string>();
    }
    
    vector<string> res;
    for(size_t i=0 ; i<arg_names.size() ; ++i){
      if(static_cast<int>(i) != cursor_pos){
        res.push_back(arg_names[i]);
      }
    }
    
    return res;
  }
  
  vector<string> get_other_arg_values() const {
    if(cursor_pos <= -1){
      return vector<string>();
    }
    
    vector<string> res;
    for(size_t i=0 ; i<arg_values.size() ; ++i){
      if(static_cast<int>(i) != cursor_pos){
        res.push_back(arg_values[i]);
      }
    }
    
    return res;
  }
  
};


//
// AutocompleteRContext --------------------------------------------------------
//


class AutocompleteRContext {
  void set_context_from_previous_args();
  void init(const AutocompleteContext &);
public:
  // default type is always a variable
  AC_TYPE type = AC_TYPE::VARIABLE;
  Container container;
  ParsedFunction function_container;
  bool is_data_function = false;
  string data_container = UNSET::STRING;
  string query;
  ParsedVariable contextual_object;
  vector<ParsedVariable> possible_tilde_data;
  
  // if the query is within a function/data call
  // => information on the argument it relates to
  string query_arg_name = UNSET::STRING;
  uint query_arg_pos = UNSET::UINT;
  
  // we keep track of the previous arguments
  vector<string> previous_arg_names;
  vector<string> previous_arg_values;
  
  AutocompleteRContext() = default;
  AutocompleteRContext(const AutocompleteContext &);
};

//
// AutocompleteSuggestion ------------------------------------------------------
//


class AutocompleteSuggestion {
  StringMatch suggestion;
  char code = 'D';
  
public:
  AutocompleteSuggestion() = default;
  
  AutocompleteSuggestion(StringMatch _suggestion, char _code):
    suggestion(_suggestion), code(_code) {};
  
  void clear(){
    suggestion = StringMatch{};
    code = 'D';
  }
  
  AutocompleteSuggestion &set_suggestion(StringMatch x){
    suggestion = x;
    return *this;
  }
  
  AutocompleteSuggestion &set_code(char x){
    code = x;
    return *this;
  }
  
  StringMatch get_suggestion(){ return suggestion; }
  char get_code(){ return code; }
    
};

//
// RAutocomplete ---------------------------------------------------------------
//


class RAutocomplete: public ServerAutocomplete {
  
  bool in_autocomp = false;
  AutocompleteRContext parsed_context;
  string context;
  string after_context;
  bool in_path;
  string path_query;
  
  AutocompleteSuggestion current_suggestion{};
  std::map<char, AutocompleteSuggestion> map_code_suggestion;
  string allowed_codes;
  bool first_update = false;
  
  string Rversion;
  string get_Rversion(){
    if(Rversion.empty()){
      Rversion = "R_" + static_cast<string>(R::R_run("R.version$major")) + "." + static_cast<string>(R::R_run("R.version$minor"));
    }
    
    return Rversion;
  }
  
  // Internal AC functions
  AC_String suggest_path();
  AC_String suggest_default();
  AC_String suggest_dollar_arobase();
  AC_String suggest_introspection();
  AC_String suggest_namespace_exports();
  AC_String suggest_package(bool add_colon = false);
  AC_String suggest_CRAN_package();
  AC_String suggest_argument();
  AC_String suggest_global_env();
  AC_String suggest_variables(bool);
  AC_String suggest_functions(bool);
  AC_String suggest_basic_datasets();
  AC_String suggest_all_datasets();
  AC_String suggest_env();
  AC_String suggest_tilde();
  
  inline StringMatch build_and_save_suggestion(const string &query, str::MetaStringVec &choices){
    StringMatch res = str::string_match(query, choices);
    current_suggestion.set_suggestion(res);
    return res;
  }

public:
  
  virtual StringMatch make_suggestions(const AutocompleteContext &) override;
  
  virtual StringMatch update_suggestions([[maybe_unused]] const char c) override;
  
  virtual AutocompleteResult finalize_autocomplete(const str::MetaString &) override;
  
  virtual void quit_autocomp() override;
 
};
