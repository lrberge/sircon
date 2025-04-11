
#include "pathmanip.hpp"

stringtools::StringMatch list_files_from_context(string x){
  // Examples of contexts
  // 
  // context = '/'
  //           './bon/jo'
  //           '../'
  //           'src\\module\\'
  //           
  // Algorithm
  // 1) is the current context a folder?
  //   a) yes: we list all the files 
  //   b) no:  we go to the parent directory (if it exists) and list the files
  // 2) list all the files and return them
  // 
  // causes of a 0-length return:
  // 1. no files in the directory
  // 2. the directory does not exist
  // 
  
  
  //
  // step 1: normalizing the path 
  //
  
  
  string x_clean = util::format_path(x);
  bool is_folder = x_clean.back() == '/';
  
  fs::path origin_path = x_clean;
  fs::path parent_path = origin_path;
  
  if(!is_folder){
    parent_path = origin_path.parent_path();
  }
  
  if(parent_path.string() == ""){
    parent_path = ".";
  }
  
  //
  // step 2: finding candidates 
  //
  
  
  vector<string> all_paths;
  
  stringtools::StringMatch res;
  
  if(fs::exists(parent_path)){
    for(auto &p : fs::directory_iterator(parent_path)){
      const fs::path &tmp_path = p.path();
      if(fs::is_directory(tmp_path)){
        all_paths.push_back(tmp_path.filename().string() + "/");
      } else {
        all_paths.push_back(tmp_path.filename().string());
      }
    }
    
    if(all_paths.empty()){
      res.set_cause_no_match("no file found in the current path");
    }
  } else {
    res.set_cause_no_match("the current path does not exist");
  }
  
  if(all_paths.empty()){
    return res;
  }
  
  //
  // step 3: context matches
  //
  
  if(is_folder){
    // we return everyone
    res = stringtools::StringMatch("", all_paths);
  } else {
    string file = origin_path.filename().string();
    res = stringtools::string_match(file, all_paths);
  }
  
  return res;
}

fs::path get_path_to_program_cache(const string &program_name){
  
  if(util::is_unset(program_name)){
    std::cout << "The program name is not set\n";
    return UNSET::PATH;
  }
  
  // we will locate the file at:
  // ROAMING/basic-console/program_name/file_name
  // 
  
  fs::path dest_path = get_path_RoamingAppData();
  if(!fs::exists(dest_path)){
    std::cerr << "the APPDATA folder does not exist: aborting history saving\n";
    return UNSET::PATH;
  }
  
  dest_path /= program_name;
  if(!fs::exists(dest_path)){
    fs::create_directory(dest_path);
    // std::cout << "Creating the directory '" << dest_path << "'\n";
  }
  
  return dest_path;
}

fs::path get_path_to_program_history(const string &program_name){
  
  fs::path prog_cache = get_path_to_program_cache(program_name);
  if(util::is_unset(prog_cache)){
    return prog_cache;
  }
  
  // current path
  fs::path current_wd = fs::current_path();
  
  uint64_t hash_i = std::hash<string>{}(current_wd.string());
  string hash = std::to_string(hash_i).substr(0, 8);
  
  string dir_name = current_wd.filename().string();
  string file_name = dir_name + "_" + hash + ".txt";
  
  // we will locate the file at:
  // ROAMING/program_name/history/file_name
  // 
  
  fs::path dest_path = prog_cache / "history";
  if(!fs::exists(dest_path)){
    fs::create_directory(dest_path);
    // std::cout << "Creating the directory '" << dest_path << "'\n";
  }
  
  dest_path /= file_name;
  // std::cout << "hist_file = " << dest_path << "\n";
  
  return dest_path;
}


fs::path get_path_to_program_global_options(const string &prog){
  
  if(prog.empty()){
    return UNSET::PATH;
  }
  
  fs::path path_user = get_path_userprofile();
  
  if(!fs::exists(path_user)){
    return UNSET::PATH;
  }
  
  return path_user / ("." + prog);
}

fs::path get_path_to_program_local_options(const string &prog){
  return fs::current_path() / ("." + prog);
}



