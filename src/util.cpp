    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//


#include "util.hpp"

namespace util {
  
namespace internal {
  static bool debug_init = true;
}

DEBUG_TYPE _debug = DEBUG_TYPE::MSG;

int create_parent_path(const fs::path &p, int max_depth, DoCheck opt){
  
  if(max_depth < 0){
    max_depth = 500;
  }
  
  if(p.has_parent_path()){
    vector<fs::path> all_parents;
    fs::path parent = p.parent_path();
    
    if(fs::exists(parent)){
      return 0;
    }
    
    int depth = 0;
    
    while(depth < max_depth){
      depth++;
      if(!fs::exists(parent)){
        // the parent does not exist: we stack it (should be created)
        all_parents.push_back(parent);
        if(parent.has_parent_path()){
          parent = parent.parent_path();
          
        } else {
          
          if(opt.is_check()){
            string err = txt("Problem: this parent path does not exist and has no parent:\n", parent);
            if(opt.is_set_error()){
              opt.set_error(err);
            } else {
              error_msg(err);
            }
          }
          
          return 1;
        }
      } else {
        // the parent exist: we create all the subdirectories
        for(auto it = all_parents.crbegin() ; it != all_parents.crend() ; ++it){
          fs::create_directory(*it);
        }
        return 0;
      }
    }
    
    if(opt.is_check()){
      string err = txt("Problem: more than ", max_depth, " parent directories do not exist -- aborting");
      if(opt.is_set_error()){
        opt.set_error(err);
      } else {
        error_msg(err);
      }
    }
    
    return 1;
  }
  
  return 0;
}


void next_debug_type(){
  if(_debug == DEBUG_TYPE::MSG){
    _debug = DEBUG_TYPE::PIPE;
    
  } else if(_debug == DEBUG_TYPE::PIPE){
    _debug = DEBUG_TYPE::MSG;
  }
}


void write_debug(const string &x){
  
  std::ios_base::openmode mode = std::ios::out;
  
  if(internal::debug_init){
    internal::debug_init = false;
  } else {
    mode |= std::ios::app;
  }
  
  std::ofstream file_out("debug.txt", mode);
  
  if(file_out.is_open()){
    file_out << x << "\n";
  }
  
  file_out.close();
  
}


} // end namespace util

