    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#include "RAutocomplete.hpp"

#include <algorithm>

using stringtools::dquote;
namespace str = stringtools;



namespace {

// static values
string package_name = UNSET::STRING;
const string NOT_A_PACKAGE = "__123__";

const vector<string> CONTROL_FUNCTIONS = {"if", "while", "for", "function"};

inline bool is_valid_R_name(const string &x){
  if(x.empty()){
    return false;
  }
  
  if(x[0] == '`'){
    if(x.size() < 3){
      return false;
    }
    
    return x.back() == '`';
  }
  
  return R::is_valid_name(x);
}

inline bool is_R_var_in_vector(const string &x, const vector<string> &vars){
  if(x.empty()){
    return false;
  }
  
  if(x[0] == '`'){
    if(x.size() < 3){
      return false;
    }
    
    string x_new = x.substr(1, x.size() - 2);
    return str::is_string_in(x_new, vars);
  }
  
  return str::is_string_in(x, vars);
}

inline bool move_after_end_of_quote(const string &x, int &i, const int n, 
                                    vector<string> &values){
  // i: points to the quote
  
  if(n <= 2){
    return false;
  }
  
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  
  if(i < n && (x[i] == '"' || x[i] == '\'')){
    
    string val;
    const char quote = x[i];
    ++i;
    while(i < n && !(x[i] == quote && !str::is_escaped(x, static_cast<uint>(i)))){
      val += x[i++];
    }
    
    if(i == n){
      return false;
    }
    
    values.push_back(val);
    ++i;
    return true;
  }
  
  ++i;
  return false;
}


inline bool is_R_string_vector(const string &x, vector<string> &values){
  
  const int n = x.size();
  int i = 0;
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  
  if(i + 2 < n && x[i] == 'c' && x[i + 1] == '('){
    i += 2;
    
    while(i < n){
      // 1) we go after the quote
      bool ok = move_after_end_of_quote(x, i, n, values);
      if(!ok){
        return false;
      }
      
      // 2) we skip the comma
      str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
      if(i < n && x[i] == ','){
        ++i;
      } else if(i < n && x[i] == ')'){
        return true;
      } else {
        // this is a problem
        return false;
      }
    }
    
    // if here => problem
    return false;
  }
  
  // then we assume it's a quote
  bool ok = move_after_end_of_quote(x, i, n, values);
  if(!ok){
    return false;
  }
  
  // there can be still some parsing issues
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  // if we're at the end of the string, we're good
  return i == n;
}

inline bool move_after_end_of_numeric(const string &x, int &i, const int n, 
                                      vector<int> &values){
  // i: points to the digit

  if(n == 0){
    return false;
  }

  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);

  if(i < n && str::is_digit(x[i])){

    string val{x[i++]};
    
    while(i < n && str::is_digit(x[i])){
      val += x[i++];
    }

    values.push_back(std::stoi(val));
    return true;
  }

  ++i;
  return false;
}


inline bool is_R_numeric_vector(const string &x, vector<int> &values){

  const int n = x.size();

  int i = 0;
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);

  if(i + 2 < n && x[i] == 'c' && x[i + 1] == '('){
    i += 2;

    while(i < n){
      // 1) we go after the quote
      bool ok = move_after_end_of_numeric(x, i, n, values);
      if(!ok){
        return false;
      }

      // 2) we skip the comma
      str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
      if(i < n && x[i] == ','){
        ++i;
      } else if(i < n && x[i] == ')'){
        return true;
      } else {
      // this is a problem
        return false;
      }
    }

    // if here => problem
    return false;
  }

  // then we assume it's a numeric
  bool ok = move_after_end_of_numeric(x, i, n, values);
  if(!ok){
    return false;
  }

  // there can be still some parsing issues
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  // if we're at the end of the string, we're good
  return i == n;
}

} // end anonymous namespace


AC_String RAutocomplete::suggest_path(){
  // path is a special case, because we modify the query here
  
  //
  // 1) we update the query and find out if a folder or not 
  //
  
  
  string x_clean = util::format_path(path_query);
  bool is_folder = x_clean.back() == '/';
  
  fs::path origin_path = x_clean;
  fs::path parent_path = origin_path;
  
  if(!is_folder){
    parent_path = origin_path.parent_path();
  }
  
  if(parent_path.string() == ""){
    parent_path = ".";
  }
  
  // updating the query to the right one
  path_query = is_folder ? "" : origin_path.filename().string();
  
  
  //
  // 2) we find the suggestions 
  //
  
  vector<string> all_paths;
  AC_String res;
  
  if(fs::exists(parent_path) && fs::is_directory(parent_path)){
    for(auto &p : fs::directory_iterator(parent_path)){
      const fs::path &tmp_path = p.path();
      if(fs::is_directory(tmp_path)){
        all_paths.push_back(tmp_path.filename().string() + "/");
      } else {
        all_paths.push_back(tmp_path.filename().string());
      }
    }
    
    if(all_paths.empty()){
      res.set_cause_empty("no file found in the current path");
    }
  } else {
    res.set_cause_empty("the current path does not exist");
  }
  
  if(!all_paths.empty()){
    res = AC_String(all_paths).set_finalize(AC_FINALIZE::PATH);
  }
  
  return res;
  
}

AC_String RAutocomplete::suggest_dollar_arobase(){
  
  AC_String choices;
  
  choices.set_finalize(AC_FINALIZE::DEFAULT);
  
  const AC_TYPE &type = parsed_context.type;
  ParsedVariable &raw_data = parsed_context.contextual_object;
  
  if(!raw_data.is_valid()){
    choices.set_cause_empty(raw_data.get_error());
    return choices;
  }
  
  const string &data = raw_data.get_data_name();
  
  if(type == AC_TYPE::DOLLAR){
    bool is_list = R::R_run("is.list(" + data + ") || is.environment(" + data + ")");
    if(!is_list){
      choices.set_cause_empty("The object " + dquote(data) + " is not list-like");
      return choices;
    }
    
    choices = R::R_run("names(" + data + ")");
    
  } else if(type == AC_TYPE::AROBASE){
    choices = R::R_run("slotNames(" + data + ")");
    
  }
  
  if(choices.empty()){
    if(R::length(data) <= 0){
      choices.set_cause_empty("The object is empty");
    } else {
      choices.set_cause_empty("The object does not have names");
    }
  }
  
  return choices;
}

AC_String RAutocomplete::suggest_introspection(){
  
  AC_String choices;
  
  ParsedVariable &raw_data = parsed_context.contextual_object;
  const string &data = raw_data.get_data_name();
  
  bool is_char = false;
  vector<string> all_values;
  if(!raw_data.is_valid()){
    // we check for DT
    
    // is it a variable name?
    if(is_valid_R_name(data) && context.size() > 3){
      
      // we need to re-parse to find the container, we remove the ">>"
      AutocompleteContext context_no_intro;
      context_no_intro.before_cursor = context.substr(0, context.size() - 2);
      
      AutocompleteRContext larger_context(context_no_intro);
      const string &base = larger_context.data_container;
      
      bool is_dt = !util::is_unset(base);
      if(is_dt){
        if(!R::exists(base)){
          is_dt = false;
        } else {
          is_dt = R::R_run("inherits(" + base + ", \"data.table\")");
        }
      }
      
      if(is_dt){
        string var_name = data;
        if(var_name.size() > 2 && var_name.front() == '`' && var_name.back() == '`'){
          var_name.pop_back();
          var_name.erase(var_name.begin());
        }
        
        bool var_exists = R::R_run(str::dquote(var_name) + " %in% names(" + base + ")");
        
        if(var_exists){
          const string varname = base + "[[" + str::dquote(var_name) + "]]";
          all_values = R::R_run("as.character(" + varname + ")");
          is_char = R::R_run("is.character(" + varname + ") || is.factor(" + varname + ")");
        }
        
      }
      
    }
    
    if(all_values.empty()){
      choices.set_cause_empty(raw_data.get_error());
      return choices;
    }
    
  } else {
    // this can be costly depending on the context
    all_values = R::R_run("as.character(" + data + ")");
    is_char = R::R_run("is.character(" + data + ") || is.factor(" + data + ")");
    
  }
  
  IndexInfo all_values_index = to_index(all_values, to_indexOpts().table());
  
  const vector<size_t> &first_obs = all_values_index.get_first_obs();
  const vector<size_t> &counts = all_values_index.get_table();
  
  const size_t n = first_obs.size();
  vector<string> values_unik(n);
  vector<string> labels(n);
  for(size_t i = 0 ; i < n ; ++i){
    values_unik[i] = all_values[first_obs[i]];
    labels[i] = " <" + std::to_string(counts[i]) + ">";
  }
  
  choices = std::move(values_unik);
  choices.set_meta("labels", std::move(labels));
  choices.set_meta("is_char", is_char ? "true" : "false");
  
  choices.set_finalize(AC_FINALIZE::INTROSPECTION);
  return choices;
  
}

AC_String RAutocomplete::suggest_namespace_exports(){
  
  AC_String choices;
  
  const AC_TYPE &type = parsed_context.type;
  ParsedVariable &raw_data = parsed_context.contextual_object;
  
  const string &data = raw_data.get_data_name();
  
  //
  // regular namespace exports 
  //
  
  
  if(!R::is_valid_name(data)){
    choices.set_cause_empty("The package name " + dquote(data) + " is invalid");
    return choices;
  }
  
  bool exists_ns = R::R_run("requireNamespace(package = " + dquote(data) + ", quietly = TRUE)");
  
  if(!exists_ns){
    choices.set_cause_empty("The package " + dquote(data) + " is not installed");
    return choices;
  }
  
  if(type == AC_TYPE::NAMESPACE_EXPORTS){
    choices = R::R_run("sort(getNamespaceExports(" + dquote(data) + "))");
    
  } else if(type == AC_TYPE::NAMESPACE_ALL){
    choices = R::R_run("ls(envir = asNamespace(" + dquote(data) + "))");  
  }
  
  choices.set_finalize(AC_FINALIZE::FUNCTION);
  
  return choices;
}

AC_String RAutocomplete::suggest_package(bool add_colon){
    
  const string &fun = parsed_context.function_container.get_fun_name();
  AC_String installed_packages = R::R_run("list.files(.libPaths())");
  
  if(installed_packages.empty()){
    installed_packages.set_cause_empty("No installed package found. Likely a library location issue. Is .libPaths() fine?");
    return installed_packages;
  }
  
  if(add_colon){
    for(auto &pkgs : *installed_packages.get_string_vec_ptr()){
      pkgs += "::";
    }
    
    installed_packages.set_finalize(AC_FINALIZE::PACKAGE_EXPORT);
    
    return installed_packages;
  }
  
  if(fun == "requireNamespace" && parsed_context.query_arg_pos == 0){
    installed_packages.set_finalize(AC_FINALIZE::QUOTE);
    
  } else if(str::is_string_in(fun, {"library", "require"}) && parsed_context.query_arg_pos == 0){
    installed_packages.set_finalize(AC_FINALIZE::NONE);
    
  } else {
    installed_packages.set_finalize(AC_FINALIZE::PACKAGE_EXPORT);
  }
  
  return installed_packages;
  
}

AC_String RAutocomplete::suggest_CRAN_package(){
  // we run the internet search every 50 days
  CachedData pkg_cached("CRAN_packages.txt");
  
  if(pkg_cached.days_since_last_write() > 50){
    // we invalidate the cache
    AC_String pkgs = R::R_run("available.packages()[, \"Package\"]");
    
    if(pkgs.empty()){
      pkgs.set_cause_empty("The list of available packages on CRAN could not be retrived");
      return pkgs;
    }
    
    pkg_cached.set_cached_vector(pkgs.get_string_vec());
    
    pkgs.set_finalize(AC_FINALIZE::QUOTE);
    return pkgs;
  }
  
  AC_String pkgs = pkg_cached.get_cached_vector();
  
  pkgs.set_finalize(AC_FINALIZE::QUOTE);
  return pkgs;
  
}

AC_String RAutocomplete::suggest_argument(){
  
  AC_String choices;
  
  ParsedFunction &fun_raw = parsed_context.function_container;
  
  if(fun_raw.empty()){
    choices.set_cause_empty("No argument found for `" + fun_raw.get_fun_name() + "`");
    return choices;
  }
  
  //
  // step 1: data arguments 
  //
  
  if(parsed_context.is_data_function){
    
    ParsedVariable data(parsed_context.data_container);
    if(!data.is_valid()){
      choices.set_cause_empty(data.get_error());
      return choices;
    }
    
    string fun = fun_raw.get_fun_name();
    string full_fun;
    vector<string> data_class = R::R_run("class(" + data.get_data_name() + ")");
    for(const auto &c : data_class){
      full_fun = fun + "." + c;
      if(R::R_run("isS3method(\"" + full_fun + "\")")){
        vector<string> args = R::R_run("names(formals(args(getS3method(" + dquote(fun) + ", " + dquote(c) + "))))");
        // NOTA: we always drop the first argument
        if(args.size() > 1){
          args.erase(args.begin());
          str::append_right(args, " = ");
          choices = args;
          break;
        }
      }
    }
    
    if(choices.empty()){
      choices.set_cause_empty("No argument found for \"" + full_fun + "\"");
    }
    
    choices.set_finalize(AC_FINALIZE::NONE);
    
    return choices;
  }
  
  //
  // step 2: regular arguments 
  //
  
  
  if(!fun_raw.exists()){
    choices.set_cause_empty(fun_raw.get_error());
    return choices;
  }
  
  AC_String arg_names;
  const string fun = fun_raw.get_complete_fun_name();  
  string dqfun = dquote(fun);
  
  // special cases
  if(util::vector_contains(CONTROL_FUNCTIONS, fun)){
    choices.set_cause_empty(fun + " is not a regular function");
    return choices;
  }
  
  //
  // Dispatching to the right S3 if appropriate
  //
  
  const vector<string> &args = parsed_context.previous_arg_values;
  
  bool is_S3 = false;
  bool is_S3_method = false;
  if(!args.empty() && !fun_raw.is_from_namespace()){
    // is the function an S3 method?
    is_S3_method = R::R_run("isS3method(" + dqfun + ") || isS3stdGeneric(" + dqfun + ")");
    if(is_S3_method){
      // is the first arg an existing object?
      const string first_arg = str::trim_WS(args[0]);
      if(R::is_valid_name(first_arg)){
        if(R::exists(first_arg)){
          // is the object's class from the method?
          vector<string> arg_classes = R::R_run("tryCatch(class(" + first_arg + "), error = function(e) character())");
          for(uint i = 0 ; i < arg_classes.size() ; ++i){
            const string &cl = arg_classes[i];
            if(R::R_run("!is.null(tryCatch(getS3method(" + dquote(fun) + ", " + dquote(cl) + "), error = function(e) NULL))")){
              arg_names = R::R_run("names(formals(args(getS3method(" + dquote(fun) + ", " + dquote(cl) + "))))");
              if(!arg_names.empty()){
                is_S3 = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  
  if(!is_S3){
    arg_names = R::R_run("names(formals(args(" + fun + ")))");
  }
  
  if(arg_names.empty()){
    choices.set_cause_empty("No argument found for `" + fun + "`");
    return choices;
  }
  
  vector<string> prev_args = parsed_context.previous_arg_names;
  
  arg_names = str::setdiff(arg_names.get_string_vec(), prev_args);
  
  if(arg_names.empty()){
    choices.set_cause_empty("No arguments left for `" + fun + "`");
    return choices;
  }
  
  for(auto &str : *arg_names.get_string_vec_ptr()){
    str += " = ";
  }
  
  arg_names.set_finalize(AC_FINALIZE::NONE);
  
  return arg_names;
  
}

AC_String RAutocomplete::suggest_global_env(){
  
  AC_String vars = R::R_run("base::ls(envir = .GlobalEnv, all.names = TRUE)");
  if(vars.empty()){
    vars.set_cause_empty("No variable found in the Global environment");
  } else {
    vars.set_finalize(AC_FINALIZE::POSSIBLE_FUNCTION);
  }
  
  return vars;
  
}

AC_String RAutocomplete::suggest_variables(bool add_ls_vars){
  // to add types for variables in data sets:
  // sapply(ls(), function(x) eval(str2lang(paste0("class(", x, ")")))[1])
  // or for variables in the current env.
  
  AC_String choices;
  
  // we check whether the variable is included in a data set
  const string &base = parsed_context.data_container;
  bool is_dt = !util::is_unset(base);
  if(is_dt){
    if(!R::exists(base)){
      is_dt = false;
    } else {
      is_dt = R::R_run("inherits(" + base + ", \"data.table\")");
    }
  }
  
  if(is_dt){
    AC_String vars_dt = R::R_run("names(" + base + ")");
    vars_dt.set_finalize(AC_FINALIZE::DEFAULT);
    if(vars_dt.empty()){
      choices.set_cause_empty("No variable found in the current data table");
    } else {
      choices = vars_dt;
    }
  }
  
  // Shall we include regular variables?
  if(!is_dt || add_ls_vars){
    // we use regular variables
    
    bool is_in_browser = R::R_run("getOption(\"sircon_is_in_browser\", default = 0L)");
    
    AC_String vars;
    
    if(is_in_browser){
      vars = R::R_run("getOption(\"sircon_browser_ls\", default = \"\")");
    } else {
      vars = R::R_run("setdiff(base::ls(all.names = TRUE), '.Random.seed')");
    }
    
    if(vars.empty()){
      choices.set_cause_empty("No variable found in the current environment");
    } else {
      vars.set_finalize(AC_FINALIZE::POSSIBLE_FUNCTION);
      choices.push_back(vars);
    }
    
  }
  
  return choices;
  
}

AC_String RAutocomplete::suggest_functions(bool add_ls_functions){
  
  const string &query = parsed_context.query;
  
  //
  // special behavior when developping packages:
  // 
  
  // we always want ALL the exports
  
  if(util::is_unset(package_name)){
    
    bool is_pkg = R::R_run("\"DESCRIPTION\" %in% list.files()");
    if(is_pkg){
      R::CPP_SEXP pkg = R::R_run("trimws(gsub(\"^Package: \", \"\", readLines(\"DESCRIPTION\", n = 1)))");
      package_name = static_cast<string>(pkg);
      if(package_name.empty()){
        package_name = NOT_A_PACKAGE;
      }
    } else {
      package_name = NOT_A_PACKAGE;
    }
    
  }
  
  AC_String pkg_funs;
  
  bool is_pkg = package_name != NOT_A_PACKAGE;
  if(is_pkg){
    bool is_loaded = R::R_run(str::dquote(package_name) + "%in% loadedNamespaces()");
    if(!is_loaded){
      is_pkg = false;
    }
    
    if(is_pkg){
      // we want ALL the functions, except the c++ wrappers
      pkg_funs = R::R_run("grep(\"^[^_]\", ls(envir = asNamespace(" + str::dquote(package_name) + ")), value = TRUE)");
    }
  }
  
  
  //
  // functions from the environment 
  //
  
  AC_String ls_funs;
  
  if(add_ls_functions && !R::R_run("ls()").empty()){
    ls_funs = R::R_run("ls()[sapply(ls(), function(x) exists(x, mode = \"function\"))]");
  }
  
  //
  // functions from other packages 
  //
  
  AC_String funs;
  
  const string loaded_NS = is_pkg ? ("setdiff(loadedNamespaces(), " + str::dquote(package_name) + ")") : "loadedNamespaces()";
  
  if(!query.empty() && query[0] == '.'){
    // we only show functions starting with a dot when the query starts with a dot
    // we ignore a few internal functions
    funs = R::R_run("grep(\"^[.]_\", sort(unlist(lapply(" + loaded_NS + ", function(x) getNamespaceExports(x)))), invert = TRUE, value = TRUE)");
  } else {
    // we don't show functions not starting with a letter
    funs = R::R_run("grep(\"^[[:alpha:]]\", sort(unlist(lapply(" + loaded_NS + ", function(x) getNamespaceExports(x)))), value = TRUE)");
  }
  
  AC_String all_funs;
  
  if(!pkg_funs.empty()){
    all_funs.push_back(pkg_funs);
  }
  
  if(!ls_funs.empty()){
    all_funs.push_back(ls_funs);
  }
  
  all_funs.push_back(funs).set_finalize(AC_FINALIZE::FUNCTION);
  
  return all_funs;

}

AC_String RAutocomplete::suggest_all_datasets(){
  
  CachedData data_cached(get_Rversion() + "/datasets_extensive.txt");
  
  
  
  if(data_cached.is_unset()){
    
    vector<string> all_lib_path_str = R::R_run(".libPaths()");
    vector<string> all_data_info;
    
    int index = 0;
    for(const auto &lib_path_str : all_lib_path_str){
      ++index;
      
      fs::path lib_path = lib_path_str;
      
      if(!fs::exists(lib_path)){
        continue;
      }
      
      for(auto &p : fs::directory_iterator(lib_path)){
        const fs::path &meta_path = p.path() / "Meta/data.rds";
        
        if(fs::exists(meta_path)){
          const string pkg = p.path().filename().string();
          vector<string> pkg_info = R::R_run(
            "readRDS(paste0(.libPaths()[" + std::to_string(index) + "], '/" + pkg + "/Meta/data.rds'))[, 1]"
          );
          const bool is_default = pkg == "datasets";
          
          const string suffix = ", package = \"" + pkg + "\"";
          for(const auto &s : pkg_info){
            if(!str::str_contains(s, '(')){
              if(is_default){
                all_data_info.push_back(s);
              } else {
                all_data_info.push_back(s + suffix);
              }
            }
          }
        }
      }
    }
    
    if(all_data_info.empty()){
      return AC_String();
    }
    
    util::vector_sort_unique(all_data_info);
    
    data_cached.set_cached_vector(all_data_info);
  }
  
  AC_String all_data_sets = data_cached.get_cached_vector();
  all_data_sets.set_finalize(AC_FINALIZE::NONE);
  
  return all_data_sets;

}

AC_String RAutocomplete::suggest_basic_datasets(){
  
  CachedData data_cached(get_Rversion() + "/datasets_basic.txt");
  
  if(data_cached.is_unset()){
    
    vector<string> all_dataset_names;
    vector<string> all_libpath = R::R_run(".libPaths()");
    for(const auto &libpath : all_libpath){
      
      fs::path path = fs::path{libpath + "/datasets/Meta/data.rds"};
      
      if(fs::exists(path)){
        vector<string> dataset_names = R::R_run("readRDS(" + str::dquote(util::format_path(path.string())) + ")[, 1]");
        if(!dataset_names.empty()){
          util::append(all_dataset_names, dataset_names);
        }
      }
    }
    
    if(all_dataset_names.empty()){
      return AC_String();
    }
    
    if(all_libpath.size() > 0){
      util::vector_sort_unique(all_dataset_names);
    }
    
    vector<string> clean_ds;
    for(const auto &s : all_dataset_names){
      if(!str::str_contains(s, '(')){
        clean_ds.push_back(s);
      }
    }
    
    data_cached.set_cached_vector(clean_ds);
    
  }
  
  AC_String all_data_sets = data_cached.get_cached_vector();
  
  return all_data_sets;

}

AC_String RAutocomplete::suggest_env(){
  
  AC_String choices = R::R_run("names(Sys.getenv())");
  
  choices.set_finalize(AC_FINALIZE::QUOTE);
  
  return choices;
}

AC_String RAutocomplete::suggest_tilde(){
  
  vector<string> all_nm;
  
  for(auto &pv : parsed_context.possible_tilde_data){
    all_nm = pv.names();
    if(!all_nm.empty()){
      break;
    }
  }
  
  if(all_nm.empty()){
    parsed_context.type = AC_TYPE::DEFAULT;
    return suggest_default();
  }
  
  AC_String choices(all_nm);
  
  choices.set_finalize(AC_FINALIZE::DEFAULT);
  
  return choices;
}

AC_String RAutocomplete::suggest_default(){
  // This is a big chunk since it dispatched depending on the context
  
  const AC_TYPE &type = parsed_context.type;
  const string &query = parsed_context.query;
  
  // cout << "\nParsed context:\n" << 
  //   "- query = '" << parsed_context.query << "'\n" << 
  //   "- function_container = '" << parsed_context.function_container << "'\n" << 
  //   "- data_container = '" << parsed_context.data_container << "'\n" << 
  //   "- contextual_object = '" << parsed_context.contextual_object << "'\n" << 
  //   "- query_arg_name = '" << parsed_context.query_arg_name << "'\n" << 
  //   "- query_arg_pos = '" << parsed_context.query_arg_pos << "'\n";
  
  allowed_codes = DEFAULT_CODES;
  
  AC_String choices;
  
  //
  // branch 1: specific suggestions 
  //
  
  if(type == AC_TYPE::DOLLAR || type == AC_TYPE::AROBASE){
    choices = suggest_dollar_arobase();
    allowed_codes = "";
    return choices;
    
  } else if(type == AC_TYPE::NAMESPACE_EXPORTS || type == AC_TYPE::NAMESPACE_ALL){
    choices = suggest_namespace_exports();
    return choices;
    
  } else if(type == AC_TYPE::TILDE){
    choices = suggest_tilde();
    return choices;
    
  } else if(type == AC_TYPE::INTROSPECTION){
    choices = suggest_introspection();
    return choices;
    
  }
  
  const string &fun = parsed_context.function_container.get_fun_name();
  
  if(str::is_string_in(fun, {"library", "require", "requireNamespace"}) && 
       parsed_context.query_arg_pos == 0){
    choices = suggest_package();
    allowed_codes = DEFAULT_CODES_ARG;
    return choices;
    
  } else if(fun == "install.packages" && parsed_context.query_arg_pos == 0){
    choices = suggest_CRAN_package();
    allowed_codes = DEFAULT_CODES_ARG;
    return choices;
    
  } else if(fun == "data" && parsed_context.query_arg_pos == 0){
    choices = suggest_all_datasets();
    allowed_codes = DEFAULT_CODES_ARG;
    return choices;
    
  } else if(fun == "Sys.getenv" && parsed_context.query_arg_pos == 0){
    choices = suggest_env();
    allowed_codes = DEFAULT_CODES_ARG;
    return choices;
  }
  
  //
  // branch 2: arguments
  //
  
  // FUNCTION_ARGUMENT
  // - we check always more than arguments, but arguments come first
  
  if(type == AC_TYPE::FUNCTION_ARGUMENT && fun != "."){
    
    allowed_codes = DEFAULT_CODES_ARG;
    
    // special case: dt[order(TAB)]
    bool prefer_variable = false;
    string &data = parsed_context.data_container;
    if(fun == "order" && !util::is_unset(data)){
      if(R::exists(data) && R::exists("is.data.table", R::existsOpts().mode_function())){
        if(R::R_run("is.data.table(`" + data + "`)")){
          prefer_variable = true;
        }
      }
    }
    
    if(!prefer_variable){
      choices = suggest_argument();
      if(!choices.empty() && query.empty()){
        // if no context provided, we only return the arguments (and not all variables and functions)
        return choices;
      }
    }
    
  }
  
  //
  // branch 3: regular variables
  //
  
  /* Algorithm
  * - if 0-1 letters typed
  *   * if DATA container == DT => suggest variables from the DT
  *   * else => suggest user-defined objects from LS
  * - if 2+ letters typed
  *   * add functions from the loaded namespaces (loadedNamespaces)
  * - if no available choices before function => functions
  * 
  * */
  
  AC_String vars = suggest_variables(query.size() >= 2);
  
  choices.push_back(vars);
  
  bool early_suggest = query.size() == 1 && choices.empty();
  
  if(query.size() >= 2 || early_suggest){
    // we add the functions
    AC_String funs = suggest_functions(false);
    choices.push_back(funs);
  }
  
  if(query.size() >= 3 || early_suggest){
    // adding packages + data sets suggestions
    AC_String pkgs = suggest_package(true);
    choices.push_back(pkgs);
    AC_String ds = suggest_basic_datasets();
    ds.set_finalize(AC_FINALIZE::NONE);
    choices.push_back(ds);
  }
  
  return choices;
}

StringMatch RAutocomplete::make_suggestions(const AutocompleteContext &context){
  
  in_autocomp = true;
  map_code_suggestion.clear();
  first_update = true;
  
  this->context = context.before_cursor;
  this->after_context = context.after_cursor;
  this->in_path = context.is_in_path;
  this->path_query = context.path_query;
  parsed_context = AutocompleteRContext(context);
  
  StringMatch res = update_suggestions(current_suggestion.get_code());
  
  return res;
}

StringMatch RAutocomplete::update_suggestions(const char code){
  
  if(!in_autocomp){
    return StringMatch().set_cause_no_match("unavailable");
  }
  
  // caching the current choices
  if(!first_update){
    
    if(code == current_suggestion.get_code()){
      return StringMatch().set_cause_no_match("identical");
    }
    
    if(!str::str_contains(allowed_codes, code)){
      return StringMatch().set_cause_no_match("unavailable");
    }
    
    if(!util::map_contains(map_code_suggestion, current_suggestion.get_code())){
      map_code_suggestion[current_suggestion.get_code()] = current_suggestion;
    }
    
    // if we already computed the suggestion
    if(util::map_contains(map_code_suggestion, code)){
      current_suggestion = map_code_suggestion.at(code);
      return current_suggestion.get_suggestion();
    }
    
  } else {
    first_update = false;
  }
  
  //
  // regular algorithm
  //
  
  AC_String choices;
  const string &query = parsed_context.query;
  const AC_TYPE type = MAP_CODE_TYPE.at(code);
  string cause_no_match;
  
  current_suggestion.set_code(code);
  
  if(type == AC_TYPE::DEFAULT){
    
    // path is a special case
    if(in_path){
      AC_String choices = suggest_path();
      return string_match(path_query, choices);
    }
    
    choices = suggest_default();
    return build_and_save_suggestion(query, choices);
  }
  
  if(type == AC_TYPE::FUNCTION_ARGUMENT){
    choices = suggest_argument();
    return build_and_save_suggestion(query, choices);
  }
  
  if(type == AC_TYPE::PACKAGE){
    choices = suggest_package();
    return build_and_save_suggestion(query, choices);
  }
  
  if(type == AC_TYPE::VARIABLE){
    choices = suggest_variables(query.size() >= 1);
    return build_and_save_suggestion(query, choices);
  }
  
  if(type == AC_TYPE::GLOBAL_ENV){
    choices = suggest_global_env();
    return build_and_save_suggestion(query, choices);
  }
  
  if(type == AC_TYPE::FUNCTION){
    // we always add the functions in the current env
    choices = suggest_functions(true);
    return build_and_save_suggestion(query, choices);
  }
  
  StringMatch res;
  res.set_cause_no_match("Internal error: This case should never happen, please fix. 754");
  
  return res;
}

void RAutocomplete::quit_autocomp(){ 
  
  current_suggestion.clear();
  map_code_suggestion.clear();
  in_autocomp = false;
  
}

const string FUNCTION_FULL_PAREN = " +-*/=<>|&~,;#$[]{}";

AutocompleteResult RAutocomplete::finalize_autocomplete(const str::MetaString &user_choice){
  // => add continue? ex: on pkg export tab leads to completion + trigerring new AC?
  
  if(!in_autocomp){
    // means this waa taken care of by the console directly (like path AC eg)
    quit_autocomp();
    return AutocompleteResult(user_choice);
  }
  
  string text = user_choice.get_string();
  
  const AC_FINALIZE &finalize = user_choice.has_key("finalize") ? string_to_finalize(user_choice.get_meta("finalize")) : AC_FINALIZE::DEFAULT;
  quit_autocomp();
  
  if(finalize == AC_FINALIZE::NONE){
    return AutocompleteResult(text);
  }
  
  if(finalize == AC_FINALIZE::PATH){
    return AutocompleteResult(text).set_continue(true);
  }
  
  if(finalize == AC_FINALIZE::QUOTE){
    return AutocompleteResult(dquote(text));
  }
  
  if(finalize == AC_FINALIZE::PACKAGE_EXPORT){
    if(text.back() == ':'){
      return AutocompleteResult(text).set_continue(true);
    } else {
      return AutocompleteResult(text + "::").set_continue(true);
    }
  }
  
  if(finalize == AC_FINALIZE::INTROSPECTION){
    // we need to erase the stuff before
    // iris:Species>>TAB => "setosa"
    const size_t n_del = parsed_context.contextual_object.get_expr_wide_width() + 2;
    bool is_char = user_choice.get_meta("is_char") == "true";
    
    if(is_char){
      text = dquote(text);
    }
    
    return AutocompleteResult(text).set_n_delete_left(n_del);
  }
  
  // we format the name
  string fmt_choice = R::is_valid_name(text) ? text : str::bquote(text);
  
  if(finalize == AC_FINALIZE::DEFAULT){
    return AutocompleteResult(fmt_choice);
  }
  
  if(finalize == AC_FINALIZE::POSSIBLE_FUNCTION){
    if(!R::exists(text, R::existsOpts().mode_function())){
      return AutocompleteResult(fmt_choice);
    }
  }
  
  //
  // function
  //
  
  if(after_context.empty()){
    return AutocompleteResult(fmt_choice + "()").set_shift(-1);
  }
  
  const uchar &c = after_context[0];
  
  if(c == '('){
    // we do nothing; just fill the AC, no good guess here 
    // (too many different use cases, so we're conservative)
    return AutocompleteResult(fmt_choice);
  }
  
  if(c == ')'){
    // this is kinda difficult, there are two cases:
    // bon(jo|) => bon(jour(|))
    // jo|) => jour(|)
    
    if(str::any_open_paren_before(context, '(')){
      return AutocompleteResult(fmt_choice + "()").set_shift(-1);
    } else {
      return AutocompleteResult(fmt_choice + "(");
    }
  }
  
  if(str::str_contains(FUNCTION_FULL_PAREN, c)){
    return AutocompleteResult(fmt_choice + "()").set_shift(-1);
  }
  
  return AutocompleteResult(fmt_choice + "(");
  
}


//
// AutocompleteRContext --------------------------------------------------------  
//

void move_i_to_comma_or_closing_paren(const string &x, int &i, const int n){
  // NOTA: I don't have an algorithm to fix ill formed code
  // => should it be done?
  // 
  
  int n_parenbracket_open = 0;
  
  while(i < n){
    
    const char &c = x[i];
    
    if(str::is_quote(c)){
      const char quote = c;
      ++i;
      while(i < n && !(x[i] == quote && !str::is_escaped(x, i))){
        ++i;
      }
    } else if(str::is_opening_paren(c)){
      ++n_parenbracket_open;
      
    } else if(str::is_closing_paren(c)){
      --n_parenbracket_open;
      if(n_parenbracket_open < 0){
        return;
      }
      
    } else if(n_parenbracket_open == 0 && c == ','){
      // we're good!
      break;
    }
    
    ++i;
  }
}

inline string extract_variable_name(const string &x, int &i, const int n, const int side){
  // NOTA: we accept mistakes in the 'variable" name
  // (can be a digit, eg)
  // 
  // i: STARTING point of the name
  // 
  
  string res;
  
  if(side == SIDE::LEFT){
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
    while(i >= 0 && str::is_word_char(x[i])){
      res = x[i] + res;
      --i;
    }
  } else {
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
    while(i < n && str::is_word_char(x[i])){
      res += x[i];
      ++i;
    }
  }
  
  return res;
}

void extract_function_arguments(const string &x, int &j, const int n, 
                                vector<string> &vec_arg_name, 
                                vector<string> &vec_arg_value,
                                const int cursor,
                                int &cursor_pos){
  // we extract function arguments and place them into a vector
  // 
  
  bool is_set = false;
  cursor_pos = -1;
  
  while(j < n){
    // each iteration we go at the end of the function argument
    
    string arg_name;
    string arg_value;
    
    str::move_i_to_non_WS_if_i_WS(x, j, SIDE::RIGHT);
    
    if(str::is_quote(x[j])){
      // quoted arg name
      
      const char quote = x[j];
      arg_name.push_back(quote);
      ++j;
      while(j < n && !(x[j] == quote && !str::is_escaped(x, j))){
        arg_name.push_back(x[j++]);
      }
      
      if(j < n){
        arg_name.push_back(x[j++]);
      }
    } else {
      // regular arg name
      
      while(j < n && str::is_word_char(x[j])){
        arg_name += x[j];
        ++j;
      }
    }
    
    int k_start = j; // we start at this point if there is no equal, see below
    
    str::move_i_to_non_WS_if_i_WS(x, j, SIDE::RIGHT);
    
    if(j < n && x[j] == '=' && !(j + 1 < n && x[j + 1] == '=')){
      // fun(arg = value
      ++j;
      str::move_i_to_non_WS_if_i_WS(x, j, SIDE::RIGHT);
      k_start = j;
      
    } else {
      // no equal!
      // fun(base[stuff], 
      // 
      arg_value = arg_name;
      arg_name = UNSET::STRING;
    }
    
    move_i_to_comma_or_closing_paren(x, j, n);
    
    const bool is_end_function = j >= n || (j < n && x[j] != ',');
    
    for(int k = k_start ; k < j ; ++k){
      arg_value += x[k];
    }
    
    vec_arg_name.push_back(arg_name);
    vec_arg_value.push_back(arg_value);
    
    if(!is_set && j >= cursor){
      is_set = true;
      cursor_pos = vec_arg_name.size() - 1;
    }
    
    ++j;
    
    if(is_end_function){
      break;
    }
  }
  
}


void AutocompleteRContext::set_context_from_previous_args(){
  
  const bool is_empty = previous_arg_names.empty();
  if(is_empty || util::is_unset(previous_arg_names.back())){
    // Those are the only cases where we consider the context to be an argument
    // fun(|arg
    //      ^^^ context => argument
    //
    // fun(hello, |bon
    //             ^^^ context => argument
    //      
    type = AC_TYPE::FUNCTION_ARGUMENT;
  }
  
  if(is_empty){
    query_arg_pos = 0;
    
  } else {
    
    query_arg_name = previous_arg_names.back();
    query_arg_pos = previous_arg_names.size() - 1;
    previous_arg_names.pop_back();
    previous_arg_values.pop_back();
    
  }
}

bool any_tilde_in_line(const string &x, int i, const int side){
  
  if(side == SIDE::LEFT){
    while(i >= 0 && x[i] != '~' && x[i] != '\n'){
      --i;
    }
    
    return i >= 0 && x[i] == '~';
  }
  
  // side right
  const int n = x.size();
  while(i < n && x[i] != '~' && x[i] != '\n'){
    ++i;
  }
  
  return i < n && x[i] == '~';
}

string build_line_from_context(const AutocompleteContext &context, int &cursor){
  
  const string &x = context.before_cursor;
  const string &y = context.after_cursor;
  
  string res;
  
  //
  // before
  //
  
  int i = x.size() - 1;
  
  while(i >= 0 && x[i] != '\n'){
    // we skip quotes
    if(str::is_quote(x[i])){
      char quote = x[i];
      --i;
      
      while(i >= 0 && !(x[i] == quote && !str::is_escaped(x, i))){
        --i;
      }
      
      if(i >= 0){
        --i;
      }
      
    } else {
      --i;
    }
  }
  
  if(i >= 0 && x[i] == '\n'){
    res = x.substr(i + 1);
  } else {
    res = x;
  }
  
  cursor = res.size();
  
  //
  // after 
  //
  
  const int n = y.size();
  i = 0;
  
  if(n == 0){
    return res;
  }
  
  while(i < n && y[i] != '\n'){
    // we skip quotes
    if(str::is_quote(y[i])){
      char quote = y[i];
      ++i;
      
      while(i < n && !(y[i] == quote && !str::is_escaped(y, i))){
        ++i;
      }
      
      if(i < n){
        ++i;
      }
      
    } else {
      ++i;
    }
  }
  
  // NOTA: we need to add a space to preserve the cursor position
  // and make a link between the cursor position and the position in the char
  // string.
  if(i < n && y[i] == '\n'){
    res += " " + y.substr(0, i);
  } else {
    res += " " + y;
  }
  
  return res;
}


Container::Container(const string &line, const int cursor){
  // starting from the cursor position: we go left to find the container
  //   then we catch all the arguments of the container
  
  const int n = line.size();
  int i = cursor;
  int n_paren_closed = 0;
  int n_bracket_closed = 0;
  
  any_container = false;
  
  if(i >= n){
    i = n - 1;
  }
  
  while(i >= 0){
    
    const char &c = line[i];
    if(str::is_quote(c)){
      // we just ignore the quote
      const char quote = c;
      --i;
      while(i >= 0 && !(line[i] == quote && !str::is_escaped(line, i))){
        --i;
      }
      
    } else if(c == '('){
      if(n_paren_closed > 0){
        --n_paren_closed;
        
      } else {
        // this is the function we're after, we extract it
        int j = i + 1;
        --i;
        function = ParsedFunction(line, i, n, SIDE::LEFT);
        pos_start = i + 1;
        
        // Now we move forward to extract the arguments
        extract_function_arguments(line, j, n, arg_names, arg_values, 
                                   cursor, cursor_pos);
        
        pos_end = j;
        any_container = true;
        return;
      }
      
    } else if(c == ')'){
      ++n_paren_closed;
      
    } else if(c == '['){
      if(n_bracket_closed > 0){
        --n_bracket_closed;
        
      } else {
        // NOTA: our algorithm won't work well when chaining
        // eg: base[, new := 55][, .(m = mean(x)), by = new][, |]
        // => the data container will be empty
        // not sure it's a big deal
        // 
        
        // the data container
        int j = i + 1;
        --i;
        
        if(i >= 0 && line[i] == '['){
          --i;
          function = ParsedFunction("[[");
        } else {
          function = ParsedFunction("[");
        }
        
        data_name = extract_variable_name(line, i, line.size(), SIDE::LEFT);
        pos_start = i + 1;
        
        // Now we move forward to extract the arguments, if needed
        extract_function_arguments(line, j, n, arg_names, arg_values,
                                   cursor, cursor_pos);
        pos_end = j;
        // we stop here, we don't need more information
        is_data = true;
        any_container = true;
        return;
      }
    } else if(c == ']'){
      ++n_bracket_closed;
      
    }
    
    --i;
  }
  
  
}


AutocompleteRContext::AutocompleteRContext(const AutocompleteContext &x){
  init(x);
}

void AutocompleteRContext::init(const AutocompleteContext &context){
  // we go backward + we have the guarantee that x is not within a string
  // later => add this possibility (to find string interpolations, etc)
  // 
  
  const string x = context.before_cursor;
  
  int n = x.size();
  int i = n - 1;
  
  //
  // step 1: getting the query
  //
  
  
  // EXAMPLE:
  // base$  bonjour
  // 01234567890123
  // n = 14
  // 
  
  // NOTA: we always accept spaces in the query
  while(i >= 0 && (str::is_word_char(x[i]) || x[i] == ' ')){
    --i;
  }
  
  // i out = 4   ('$')
  
  // ... but we skip the first spaces
  while(i + 1 < n && x[i + 1] == ' '){
    ++i;
  }
  
  // i out = 6   (' ')
  
  if(i < 0){
    query = x;
  } else {
    // i + 1 = 7   ('b')
    // n - i - 1 = 14 - 6 - 1 = 7
    query = x.substr(i + 1, n - i - 1);
  }
  
  
  //
  // step 2: function/data containers and arguemnts 
  //
  
  // we don't look beyond the context
  n = i + 1;
  
  // NOTA:
  // I can do the same for the data => extract he arguments
  // I should extract ghe arguments  only for the innermost
  // => account for that, write separate functions to extract the arguments
  // 
  // By default the type is a variable, unless it is within a function,
  // in which case it can also be an argument.
  // 
  
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
  
  if(i < 0){
    return;
  }
  
  // special cases
  if(x[i] == '$' || x[i] == '@'){
    type = x[i] == '$' ? AC_TYPE::DOLLAR : AC_TYPE::AROBASE;
    --i;
    contextual_object = ParsedVariable(x, i, n, SIDE::LEFT);
    return;
  } else if(x[i] == ':'){
    --i;
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
    if(i >= 0 && x[i] == ':'){
      --i;
      str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
      if(i >= 0 && x[i] == ':'){
        --i;
        str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
        type = AC_TYPE::NAMESPACE_ALL;
        
      } else {
        type = AC_TYPE::NAMESPACE_EXPORTS;
      }
      
      contextual_object = ParsedVariable(x, i, n, SIDE::LEFT);
      return;
    }
  } else if(x[i] == '>' && i - 1 >= 0 && x[i - 1] == '>'){
    // introspection
    i -= 2;
    type = AC_TYPE::INTROSPECTION;
    contextual_object = ParsedVariable(x, i, n, SIDE::LEFT);
    return;
  }
  
  bool is_tilde = any_tilde_in_line(x, i, SIDE::LEFT) || any_tilde_in_line(context.after_cursor, 0, SIDE::RIGHT);
  
  Container cont_inner(x, x.size() - 1);
  
  if(cont_inner.is_container()){
    
    //
    // step 1: the function container 
    //
    
    
    function_container = cont_inner.get_parsed_function();
    previous_arg_names = cont_inner.get_previous_arg_names();
    previous_arg_values = cont_inner.get_previous_arg_values();
    
    query_arg_name = cont_inner.get_cursor_arg_name();
    query_arg_pos = cont_inner.get_cursor_position();
    
    is_data_function = cont_inner.is_data_container();
    bool is_data_container = is_data_function;
    
    if(util::is_unset(query_arg_name)){
      if(is_data_container){
        if(function_container.get_fun_name() == "["){
          if(query_arg_pos >= 2){
            type = AC_TYPE::FUNCTION_ARGUMENT;
          }
        } else {
          // [[
          if(query_arg_pos >= 1){
            type = AC_TYPE::FUNCTION_ARGUMENT;
          }
        }
      } else {
        type = AC_TYPE::FUNCTION_ARGUMENT;
      }
    }
    
    //
    // step 2: the data container 
    //
    
    int pos = cont_inner.get_pos_start();
    
    if(is_data_container){
      data_container = cont_inner.get_data_name();
    }
    
    while(!is_data_container && pos > 0){
      
      Container new_cont(x, pos);
      if(!new_cont.is_container()){
        break;
      }
      
      if(new_cont.is_data_container()){
        is_data_container = true;
        data_container = new_cont.get_data_name();
      } else {
        pos = new_cont.get_pos_start();
      }
    }
    
    //
    // step 3: tilde 
    //
    
    if(is_tilde){
      // We need to reparse the full line
      
      int cursor = 0;
      string line = build_line_from_context(context, cursor);
      
      Container cont_inner = Container(line, cursor);
      vector<string> other_arg_values;
      
      bool is_ok = str::str_contains(cont_inner.get_cursor_arg_value(), '~');
      if(is_ok){
        other_arg_values = cont_inner.get_other_arg_values();

      } else {
        Container cont_outer(line, cont_inner.get_pos_start());
        
        is_ok = str::str_contains(cont_outer.get_cursor_arg_value(), '~');
        if(is_ok){
          other_arg_values = cont_outer.get_other_arg_values();
        }
        
      }
      
      if(is_ok && !other_arg_values.empty()){
        for(auto &arg : other_arg_values){
          ParsedVariable var(arg);
          if(var.is_full_string()){
            possible_tilde_data.push_back(var);
          }
        }
        
        if(!possible_tilde_data.empty()){
          // finally, we're good
          type = AC_TYPE::TILDE;
        }
        
      }
    }
    
  }
  
  
}

//
// ParsedFunction -------------------------------------------------------------- 
//

ParsedFunction::ParsedFunction(const string &x, int &i, const int n, const int side){
  // NOTA: we accept mistakes in the 'variable" name
  // (can be a digit, eg)
  // 
  // i: STARTING point of the name
  // 
  
  is_set = true;
  
  fun_name = extract_variable_name(x, i, n, side);
  
  if((side == SIDE::LEFT && i <= 0) || (side == SIDE::RIGHT && i >= n)){
    return;
  }
  
  if(side == SIDE::LEFT){
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
    if(x[i] == ':' && x[i - 1] == ':'){
      // namespace
      i -= 2;
      if(i >= 0 && x[i] == ':'){
        --i;
        type = TYPE::TRIPLE_COLON;
      } else {
        type = TYPE::DOUBLE_COLON;
      }
      
      if(str::is_word_char(x[i])){
        pkg = extract_variable_name(x, i, n, side);
      } else {
        type = TYPE::LOADED_FUN;
      }
      
    }
    
  } else {
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
    
    if(i + 2 < n && x[i] == ':' && x[i + 1] == ':'){
      // namespace
      i += 2;
      if(i < n && x[i] == ':'){
        ++i;
        type = TYPE::TRIPLE_COLON;
      } else {
        type = TYPE::DOUBLE_COLON;
      }
      
      if(str::is_word_char(x[i])){
        pkg = fun_name;
        fun_name = extract_variable_name(x, i, n, side);
      } else {
        type = TYPE::LOADED_FUN;
      }
      
    }
  }
  
}

bool ParsedFunction::exists() {
  
  if(type == TYPE::LOADED_FUN){
    bool fun_exists = R::exists(fun_name, R::existsOpts().mode_function());
    if(!fun_exists){
      error = "Function `" + fun_name + "` does not exist";
      return false;
    }
    return true;
  }
  
  if(str::no_nonspace_char(pkg)){
    error= "The package name `" + pkg + "` is ill formed.";
    return false;
  }
  
  bool exists_ns = R::R_run("requireNamespace(package = " + dquote(pkg) + ", quietly = TRUE)");
  
  if(!exists_ns){
    error = "The package " + dquote(pkg) + " is not installed";
    return false;
  }
  
  vector<string> all_funs;
  if(type == TYPE::DOUBLE_COLON){
    all_funs = R::R_run("sort(getNamespaceExports(" + dquote(pkg) + "))");
    
  } else if(type == TYPE::TRIPLE_COLON){
    all_funs = R::R_run("ls(envir = asNamespace(" + dquote(pkg) + "))");  
  }
  
  bool fun_exists = find(all_funs.begin(), all_funs.end(), fun_name) != all_funs.end();
  
  if(!fun_exists){
    error = "Function `" + fun_name + "` is not contained by the package `" + pkg + "`";
    return false;
  }
  
  return true;
  
}

string ParsedFunction::get_complete_fun_name() const {
  
  if(type == TYPE::LOADED_FUN){
    return fun_name;
  }
  
  if(type == TYPE::DOUBLE_COLON){
    return pkg + "::" + fun_name;
  }
  
  return pkg + ":::" + fun_name;
  
}


//
// ParsedVariable --------------------------------------------------------------
//

ParsedVariable::ParsedVariable(const string &x){
  const int n = x.size();
  int i = n - 1;
  
  init(x, i, n, SIDE::LEFT);
}

ParsedVariable::ParsedVariable(const string &x, int &i, const int n, const int side){
  init(x, i, n, side);
}

void ParsedVariable::init(const string &x, int &i, const int n, const int side){
  // i: starting position and the x[i] WILL be included in the parsed string
  
  const int i_start = i;
  
  if(side != SIDE::LEFT){
    throw util::bad_type("ParsedVariable can currently only be run on the left.");
  }
  
  if(n == 0){
    error = "empty string";
    return;
  }
  
  while(i >= 0){
    
    string expr;
    TYPE type = TYPE::ROOT;
    
    const char &c = x[i];
    
    if(c == ']'){
      if(i - 1 >= 0 && x[i - 1] == ']'){
        // double square bracket
        i -= 2;
        while(i >= 1 && !(x[i] == '[' && x[i - 1] == '[')){
          expr = x[i--] + expr;
        }
        
        str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
        
        if(expr.empty()){
          error = "parsing error: empty variable name in `[[]]` selection";
          return;
          
        } else if(i == 0){
          error = "parsing error: the double square bracket is not open properly (`" + expr + "]]`)";
          return;
          
        } else if(i == 1){
          error = "parsing error: expecting a variable before the double square bracket (`[[" + expr + "]]`)";
          return;
        }
        
        type = TYPE::DOUBLE_SQUARE_BRACKET;
        i -= 2;
        
      } else {
        
        --i;
        while(i >= 0 && x[i] != '['){
          expr = x[i--] + expr;
        }
        
        str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
        
        if(expr.empty()){
          error = "parsing error: empty variable name in `[]` selection";
          return;
          
        } else if(i == 0){
          error = "parsing error: the square bracket is not open properly (`" + expr + "]`)";
          return;
          
        } else if(i == 1){
          error = "parsing error: expecting a variable before the square bracket (`[" + expr + "]`)";
          return;
        }
        
        type = TYPE::SINGLE_SQUARE_BRACKET;
        --i;
        
      }
      
      // we save and continue
      all_expr.push_back(expr);
      all_type.push_back(type);
      continue;
      
    } else if(c == '`'){
      
      --i;
      while(i >= 0 && x[i] != '`'){
        expr = x[i--] + expr;
      }
      
      if(expr.empty()){
        error = "parsing error: empty variable name in backticks";
        return;
        
      } else if(i < 0){
        error = "parsing error: incomplete variable name (" + expr + "), backticks not open";
        return;
      }
      
    } else if(str::is_word_char(c)){
      while(i >= 0 && str::is_word_char(x[i])){
        expr = x[i--] + expr;
      }
      
    } else {
      error = "parsing error: expecting a variable name, received a non word character (" + string{c} + ")";
      return;
    }
    
    all_expr.push_back(expr);
    
    // finding the type
    str::move_i_to_non_WS_if_i_WS(x, i, SIDE::LEFT);
    bool is_root = false;
    
    if(i >= 0 && x[i] == '$'){
      type = TYPE::DOLLAR;
      
    } else if(i >= 0 && x[i] == '@'){
      type = TYPE::DOLLAR;
      
    } else {
      // root, note that it can also be a parsing error...
      is_root = true;
    }
    
    all_type.push_back(type);
    
    if(is_root){
      break;
    } else {
      --i;
    }
    
  }
  
  if(all_type.empty()){
    error = "internal error: no type found";
    return;
  }
  
  if(all_type.back() != TYPE::ROOT){
    error = "parsing error: when data is chained it must start with a root variable";
    return;
  }
  
  // we count the wide size
  int i_out = i;
  if(i_out > 0){
    // out of the previous loop, we're at the first character after the variable
    // ex: x = iris$Species|
    //       | i_out = 2
    // => we want to go to the first char of the expression
    
    ++i_out;
    str::move_i_to_non_WS_if_i_WS(x, i_out, SIDE::RIGHT);
  } else {
    i_out = 0;
  }
  
  size_t wide_width = 0;
  uint narrow_i = i_out;
  while(narrow_i <= static_cast<size_t>(i_start) && narrow_i < static_cast<size_t>(n)){
    ++wide_width;
    str::utf8::move_i_to_start_of_next_utf8_char(x, narrow_i);
  }
  
  full_expr_wide_width = wide_width;
  
  // does the data name take the full string?
  int j = i;
  str::move_i_to_non_WS_if_i_WS(x, j, SIDE::LEFT);
  _is_full_string = j < 0;
  
}



inline bool ParsedVariable::set_error(const string &msg){
  _is_valid = false;
  is_set = true;
  error = msg;
  return false;
}
  
bool ParsedVariable::is_valid(){
  // we check the validity
  
  if(!error.empty()){
    _is_valid = false;
    is_set = true;
    return false;
  }
  
  if(is_set){
    return _is_valid;
  }
  
  const int n = all_expr.size();
  
  for(int i = n - 1 ; i >= 0 ; --i){
    const string &expr = all_expr[i];
    const TYPE &type = all_type[i];
    
    if(type == TYPE::ROOT){
      
      full_expr = expr;
      
      if(is_valid_R_name(expr)){
        if(!R::exists(expr)){
          return set_error("the variable `" + expr + "` does not exist");
        }
        
      } else {
        return set_error("when data chaining, the root element must be a variable");
      }
      
      current_names = R::R_run("names(" + full_expr + ")");
      
    } else if(type == TYPE::DOLLAR || type == TYPE::AROBASE){
      
      string tag = type == TYPE::DOLLAR ? "$" : "@";
      
      // we get the current names
      string fun = type == TYPE::DOLLAR ? "names" : "slotNames";
      current_names = R::R_run(fun + "(" + full_expr + ")");
      
      if(is_R_var_in_vector(expr, current_names)){
        // OK!
      } else {
        return set_error("the value `" + tag + expr + "` does not exist");
      }
      
      full_expr += tag + expr;
      
    } else {
      // [[]] or []
      // Now the expression can be varied
      
      const bool is_dsb = type == TYPE::DOUBLE_SQUARE_BRACKET;
      
      bool is_char_vector;
      vector<string> char_values;
      vector<int> num_values;
      
      if(is_R_string_vector(expr, char_values)){
        // base[c("bon", "jour")]
        is_char_vector = true;
        
      } else if(is_R_numeric_vector(expr, num_values)) {
        // is_num_vector = true;
        
      } else if(is_valid_R_name(expr)){
        // we only take into account variables that are
        // character vectors or numeric vectors
        if(R::exists(expr)){
          if(R::R_run("is.character(" + expr + ")")){
            is_char_vector = true;
            char_values = R::R_run(expr);
            
          } else if(R::R_run("is.numeric(" + expr + ")")){
            // is_num_vector = true;
            num_values = R::R_run(expr);
            
          } else {
            return set_error("data chaining only works with numeric/character vectors (`" + expr + "` is not)");
          }
        } else {
          return set_error("the variable " + expr + " does not exist");
        }
      } else {
        return set_error("the expression `" + expr + "` cannot be deduced");
      }
      
      // OUT: either numeric vector, either character vector
      
      // now we check the values asked for are valid
      const int n_values = is_char_vector ? char_values.size() : num_values.size();
      
      if(is_dsb && n_values > 1){
        return set_error("double square brackets, [[]], accept only scalars (here it's of length " + n_values);
      }
      
      // we get the current names
      current_names = R::R_run("names(" + full_expr + ")");
      
      if(is_char_vector){
        for(const auto &v : char_values){
          if(!str::is_string_in(v, current_names)){
            return set_error("the value `" + v + "` does not exist in the data set");
          }
        }
        
      } else {
        const int n_names = current_names.size();
        for(const auto &v : num_values){
          if(v > n_names){
            return set_error("the value " + std::to_string(v) + " does not exist in the data set (length = " + std::to_string(n_names) + ")");
          }
        }
      }
      
      full_expr += is_dsb ? ("[[" + expr + "]]") : ("[" + expr + "]");
      
    }
    
  }
  
  _is_valid = true;
  is_set = true;
  
  return true;
}

vector<string> ParsedVariable::names(){
  if(!is_set){
    is_valid();
  }
  return current_names;
}

string ParsedVariable::get_data_name(){
  if(!is_set){
    is_valid();
  }
  
  return full_expr; 
}






