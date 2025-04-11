    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include "constants.hpp"
#include "stringtools.hpp"
#include "util.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using std::string;
using std::vector;

using uint = unsigned int;



stringtools::StringMatch list_files_from_context(string x);

fs::path get_path_to_program_cache(const string &);

fs::path get_path_to_program_history(const string &);

fs::path get_path_to_program_global_options(const string &);

fs::path get_path_to_program_local_options(const string &);

// NOTE: if env vars don't work we may use the stuff here:
// https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid
// 
inline fs::path get_path_RoamingAppData(){
  // Get-ChildItem env:
  // => displays the env variables
  
  char *path_appdata = getenv("APPDATA");
  
  if(!path_appdata){
    std::cerr << "RoamingAppData path could not be located\n";
    return fs::current_path();
  }
  
  return fs::path(path_appdata);
}

inline fs::path get_path_userprofile(){
  // Get-ChildItem env:
  // => displays the env variables
  
  char *path_UP = getenv("USERPROFILE");
  
  if(!path_UP){
    std::cerr << "USERPROFILE path could not be located\n";
    return fs::current_path();
  }
  
  return fs::path(path_UP);
}
