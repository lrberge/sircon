    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#pragma once



#include "util.hpp"
#include "shellrun.hpp"
#include "console.hpp"
#include "R.hpp"
#include "RAutocomplete.hpp"

#include <windows.h>
#include <signal.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

using std::string;
using std::vector;
using std::cout;
using std::endl;

namespace str = stringtools;

//
// loading functions from DLL --------------------------------------------------
//

template<typename T> T extract_dll_function(HMODULE lib, const char* name, bool error = true){
   
   PROC addr = GetProcAddress(lib, name);
   if(addr == nullptr){
      if(error){
        std::cout << "The function " << name << " could not be loaded\n";
        throw std::runtime_error("DLL could not be loaded.");
      }
      return nullptr;
   }
   
   return reinterpret_cast<T>(addr);
}

template<typename T> T extract_dll_variable(HMODULE lib, const char* name){
   
   PROC addr = GetProcAddress(lib, name);
   if(addr == nullptr){
      std::cout << "The variable " << name << " could not be loaded\n";
      throw std::runtime_error("DLL could not be loaded.");
      return 0;
   }
   
   return *(reinterpret_cast<T*>(addr));
}

template<typename T> T* extract_dll_pointer(HMODULE lib, const char* name){
   
   PROC addr = GetProcAddress(lib, name);
   if(addr == nullptr){
      std::cout << "The variable " << name << " could not be loaded\n";
      throw std::runtime_error("DLL could not be loaded.");
      return 0;
   }
   
   return reinterpret_cast<T*>(addr);
}

//
// RLanguageServer -------------------------------------------------------------
//

class RLanguageServer : public LanguageServer {
private:
  string prompt = "> ";
  string prompt_cont = "+ ";
  string prompt_color = VTS::FG_MAGENTA;
  
  bool init_ok = false;
  
  std::map<string, HMODULE> dll_handles;
  std::map<string, vector<string>> cached_ns_functions;
  std::map<string, vector<string>> cached_ns_functions_all;
  void init_R();
  
  int argc = 0;
  char **argv = nullptr;
  
  //
  // Options
  //
  
  friend void hook_set_prompt_in_R(ConsoleCommand *pconcom, LanguageServer *plang, ParsedArg *x);
  
  //
  // special functions
  //
  
  bool is_vanilla = false;
  bool do_restart = false;
  
public:
  
  static ConsoleCommand concom;
  RAutocomplete autocomp;
  
  RLanguageServer(int, char**);
  virtual CmdParsed parse_command(const string &cmd);
  virtual void resize_window_width(const uint width);
  void run_repl();
  void main_loop();
  void exit_R();
  
  virtual bool is_line_comment(const string &str){
    if(str.empty()) return false;
    int i = 0;
    str::move_i_to_non_WS_if_i_WS(str, i, SIDE::RIGHT);
    return str[i] == '#';
  }
  
};

