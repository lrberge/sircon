    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#pragma once

#include "util.hpp"
#include "stringtools.hpp"
#include "program_options.hpp"


  
//
// SpecialFunctionInfo --------------------------------------------------------- 
//

class ConsoleCommand;
class LanguageServer;

class SpecialFunctionInfo {
  void (*fun_without_args)(ConsoleCommand *) = nullptr;
  void (*fun_with_args)(ConsoleCommand *, const vector<ParsedArg> &) = nullptr;
  void (*fun_server_without_args)(ConsoleCommand *, LanguageServer *) = nullptr;
  void (*fun_server_with_args)(ConsoleCommand *, LanguageServer *, const vector<ParsedArg> &) = nullptr;
  vector<ArgumentFormat> args_fmt;
  
  ConsoleCommand *pconcom = nullptr;
  LanguageServer *plgsrv = nullptr;
  bool is_server = false;
  
public:
  
  SpecialFunctionInfo() = default;
  
  SpecialFunctionInfo(ConsoleCommand *pconsole, void (*fun)(ConsoleCommand *)): 
    fun_without_args(fun), pconcom(pconsole) {};
  
  SpecialFunctionInfo(ConsoleCommand *pconsole, 
                      void (*fun)(ConsoleCommand *, const vector<ParsedArg> &), 
                      const vector<ArgumentFormat> &all_args): 
                      fun_with_args(fun), args_fmt(all_args), pconcom(pconsole) {};
  
  SpecialFunctionInfo(ConsoleCommand *pconsole, 
                      LanguageServer *pserv,
                      void (*fun)(ConsoleCommand *, LanguageServer *)): 
                      fun_server_without_args(fun), pconcom(pconsole), 
                      plgsrv(pserv), is_server(true) {};
                    
  SpecialFunctionInfo(ConsoleCommand *pconsole, 
                      LanguageServer *pserv,
                      void (*fun)(ConsoleCommand *, LanguageServer *, const vector<ParsedArg> &), 
                      const vector<ArgumentFormat> &all_args): 
                      fun_server_with_args(fun), args_fmt(all_args), pconcom(pconsole),
                      plgsrv(pserv), is_server(true) {};
  
  
  const vector<ArgumentFormat> &get_arg_fmt() const {
    return args_fmt;
  }
  
  bool has_args() const {
    return !args_fmt.empty();
  }
  
  void fun(){
    if(is_server){
      fun_server_without_args(pconcom, plgsrv);
    } else {
      fun_without_args(pconcom);
    }
  }
  
  void fun(const vector<ParsedArg> &x){
    if(is_server){
      fun_server_with_args(pconcom, plgsrv, x);
    } else {
      fun_with_args(pconcom, x);
    }
  }
  
};

//
// special functions 
//

void sf_debug_to_file(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_time_all(ConsoleCommand *pconcom);
void sf_time_none(ConsoleCommand *pconcom);
void sf_path_history(ConsoleCommand *pconcom);
void sf_path_options(ConsoleCommand *pconcom);
void sf_path_executable(ConsoleCommand *pconcom);
void sf_clear_history(ConsoleCommand *pconcom);
void sf_copy_last_output(ConsoleCommand *pconcom);
void sf_step_into_last_output(ConsoleCommand *pconcom);
void sf_width(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_file_list(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_file_copy(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_file_peek(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_open_folder([[maybe_unused]] ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_list_colors(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_reprex_last(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);
void sf_reprex_mode(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args);

std::shared_ptr<AutocompChoices> suggest_reprex_last(ConsoleCommand *pconcom);

inline std::map<string, SpecialFunctionInfo> get_main_special_functions(ConsoleCommand *pconcom){
  
  const std::map<string, SpecialFunctionInfo> all_sf = {
    {"debug_to_file", SpecialFunctionInfo(pconcom, sf_debug_to_file, {argtype::LOGICAL("true")})},
    {"time_all", SpecialFunctionInfo(pconcom, sf_time_all)},
    {"time_none", SpecialFunctionInfo(pconcom, sf_time_none)},
    {"path_history", SpecialFunctionInfo(pconcom, sf_path_history)},
    {"path_options", SpecialFunctionInfo(pconcom, sf_path_options)},
    {"path_executable", SpecialFunctionInfo(pconcom, sf_path_executable)},
    {"clear_history", SpecialFunctionInfo(pconcom, sf_clear_history)},
    {"copy_last_output", SpecialFunctionInfo(pconcom, sf_copy_last_output)},
    {"step_into_last_output", SpecialFunctionInfo(pconcom, sf_step_into_last_output)},
    {"width", SpecialFunctionInfo(pconcom, sf_width, {argtype::INT("-1")})},
    {"file_list", SpecialFunctionInfo(pconcom, sf_file_list, {argtype::PATH(".").path_must_exist()})},
    {"file_peek", SpecialFunctionInfo(pconcom, sf_file_peek, {argtype::PATH().path_must_exist()})},
    {"open_folder", SpecialFunctionInfo(pconcom, sf_open_folder, {argtype::PATH(".").path_must_exist()})},
    {"file_copy", SpecialFunctionInfo(pconcom, sf_file_copy, {argtype::PATH().path_must_exist(), argtype::PATH(".")})},
    {"list_colors", SpecialFunctionInfo(pconcom, sf_list_colors, {argtype::STRING("")})},
    {"reprex_last", SpecialFunctionInfo(pconcom, sf_reprex_last, {argtype::INT("1").set_suggestion(suggest_reprex_last)})},
    {"reprex_mode", SpecialFunctionInfo(pconcom, sf_reprex_mode, {argtype::LOGICAL("true")})},
  };
  
  return all_sf;
}

