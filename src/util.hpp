    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <chrono>
  
using std::string;
using std::vector;

namespace fs = std::filesystem;

namespace UNSET {
  using time_t = std::chrono::time_point<std::chrono::system_clock>;
  
  const string STRING = "!__UNSET__!";
  const vector<string> STRING_VECTOR = {UNSET::STRING};
  const fs::path PATH = UNSET::STRING;
  const unsigned int UINT = (1 << 31);
  const time_t TIME = time_t();
}


namespace util {

using time_t         = std::chrono::time_point<std::chrono::system_clock>;
using duration_us_t  = std::chrono::microseconds;
using duration_ms_t  = std::chrono::milliseconds;
using duration_sec_t = std::chrono::seconds;

const string FG_ERROR = "\033[91m";
const string FG_INFO = "\033[94m";
const string FG_DEBUG = "\033[33m";
const string FG_DEBUG_TITLE = "\033[38;2;218;112;214m";
const string FG_DEFAULT = "\033[39m";

//
// unset
//


inline bool is_unset(const string &x){
  return x == UNSET::STRING;
}

inline bool is_unset(const vector<string> &x){
  return x.size() == 1 && x[0] == UNSET::STRING;
}

inline bool is_unset(const fs::path &x){
  return x == UNSET::PATH;
}

inline bool is_unset(const unsigned int &x){
  return x == UNSET::UINT;
}

inline bool is_unset(const time_t &x){
  return x == UNSET::TIME;
}
  
//
// check 
//

class DoCheck {
  bool _check = true;
  string *perror = nullptr;
  bool _is_set_error = false;
  
public:
  DoCheck(){};
  DoCheck(bool check): _check(check){};
  DoCheck(string &err): _check(true), perror(&err), _is_set_error(true){};
  
  bool is_check() const { return _check; }
  
  bool is_set_error() const {
    return _is_set_error;
  }
  
  void set_error(const string &err) const {
    *perror = err;
  }
  
};

//
// time 
//

inline time_t now(){
  return std::chrono::system_clock::now();
}

inline double timediff_us(time_t tx, time_t ty){
  if(tx > ty){
    std::swap(tx, ty);
  }
  
  const duration_us_t nb_us = std::chrono::duration_cast<duration_us_t>(ty - tx);
  
  return nb_us.count();
}

inline double timediff_ms(time_t tx, time_t ty){
  const double nb_us = timediff_us(tx, ty);
  return nb_us / 1000;
}

inline double timediff_sec(time_t tx, time_t ty){
  const double nb_us = timediff_us(tx, ty);
  return nb_us / 1000000;
}

inline double elapsed_us(time_t tx){
  return timediff_us(tx, now());
}

inline double elapsed_ms(time_t tx){
  return timediff_ms(tx, now());
}

inline double elapsed_sec(time_t tx){
  return timediff_sec(tx, now());
}


//
// general utilities
//

template<typename T>
struct remove_cv_ref {
  using type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
};


template<typename T1, typename T2>
inline bool map_contains(const std::map<T1, T2> &x, T1 key){
  // there is no check on the parameters, no error handling
  // => only for internal use
  auto search = x.find(key);
  return search != x.end();
}

template<typename T>
inline bool map_contains(const std::map<string, T> &x, const char *key){
  // there is no check on the parameters, no error handling
  // => only for internal use
  auto search = x.find(string{key});
  return search != x.end();
}

template<typename T>
inline vector<string> map_names(const std::map<string, T> &x){
  vector<string> res;
  for(auto it = x.begin() ; it != x.end() ; ++it){
    res.push_back(it->first);
  }
  
  return res;
}

template<typename T1, typename T2>
inline void map_add_entries(std::map<T1, T2> &x, const std::map<T1, T2> &y){
  for(const auto &it : y){
    x[it.first] = it.second;
  }
}

template<typename T>
inline bool vector_contains(const vector<T> &x, const T &value){
  auto search = std::find(x.begin(), x.end(), value);
  return search != x.end();
}

template<typename T>
inline void vector_sort_unique(vector<T> &x){
  // slow
  std::sort(x.begin(), x.end());
  auto last = std::unique(x.begin(), x.end());
  x.erase(last, x.end());
}

template<typename T>
inline void append(vector<T> &x, const vector<T> &y){
  x.insert(x.end(), y.begin(), y.end());
}

template<typename T>
inline size_t which(const vector<T> &x, const T &value){
  
  for(size_t i = 0 ; i < x.size() ; ++i){
    if(x[i] == value){
      return i;
    }
  }
  
  return x.size();
}

inline string format_path(const string &x){
  size_t i = 0;
  const size_t n = x.size();
  string res;
  while(i < n){
    if(x[i] == '\\' || x[i] == '/'){
      while(i < n && (x[i] == '\\' || x[i] == '/')){
        ++i;
      }
      
      res.push_back('/');
    } else {
      res.push_back(x[i++]);
    }
  }
  
  // we remove trailing spaces
  while(!res.empty() && res.back() == ' '){
    res.pop_back();
  }
  
  return res;
}

inline string format_path(const fs::path x){
  return format_path(x.string());
}

int create_parent_path(const fs::path &p, int max_depth, DoCheck opt = DoCheck(false));


inline bool ends_with_backslash(const string &x){
  int i = static_cast<int>(x.size()) - 1;
  while(i >= 0 && x[i] == ' '){
    --i;
  }
  
  return i >= 0 &&  x[i] == '\\';
}

inline void trim_ending_backslash(string &x){
  
  if(x.empty()){
    return;
  }
  
  int i = static_cast<int>(x.size()) - 1;
  while(i >= 0 && x[i] == ' '){
    --i;
  }
  
  if(i >= 0 && x[i] == '\\'){
    --i;
  }
  
  if(i >= 0){
    if(i < static_cast<int>(x.size()) - 1){
      x.erase(x.begin() + i + 1, x.end());
    }
  } else {
    x.clear();
  }
  
}

inline bool get_full_line(std::ifstream &file_in, string &line, int &line_nb_start, int &line_nb_end){
  // we extract several continued lines from a file
  // lines can span multiple lines with a backslash at the end
  // line_nb_start and line_nb_end must start at 0
  
  // NOTA: this function is to be used in a while, as in:
  // while(util::get_full_line(options_file_in, line, line_nb, line_nb_end)
  // 
  
  if(!std::getline(file_in, line)){
    return false;
  }
  
  line_nb_start = line_nb_end + 1;
  line_nb_end = line_nb_start;
  
  if(!ends_with_backslash(line)){
    // regular line
    return true;
  }
  
  // continuation line
  
  string full_line = line;
  while(true){
    trim_ending_backslash(full_line);
    
    if(std::getline(file_in, line)){
      full_line += line;
      ++line_nb_end;
    } else {
      break;
    }
    
    if(!ends_with_backslash(line)){
      break;
    }
  }
  
  line = std::move(full_line);
  return true;
}

inline void merge_extra_lines_if_needed(std::ifstream &file_in, string &line, int &line_nb_end){
  // we add extra lines with the line ends with a backslash
  // 
  
  if(!ends_with_backslash(line)){
    return;
  }
  
  // line ends with a back slash
  trim_ending_backslash(line);
  
  string extra_lines;
  int line_nb_start = 0;
  get_full_line(file_in, extra_lines, line_nb_start, line_nb_end);
  
  line = line + " " + extra_lines;
}

inline void merge_extra_lines_if_needed(std::ifstream &file_in, string &line){
  // we add extra lines with the line ends with a backslash
  // 
  
  int line_nb_end = 0;
  merge_extra_lines_if_needed(file_in, line, line_nb_end);
}

inline bool get_full_line(std::ifstream &file_in, string &line, int &line_nb_end){
  int line_nb_start = 0;
  
  return get_full_line(file_in, line, line_nb_start, line_nb_end);
}

inline bool get_full_line(std::ifstream &file_in, string &line){
  int line_nb_start = 0;
  int line_nb_end = 0;
  
  return get_full_line(file_in, line, line_nb_start, line_nb_end);
}


//
// text formatting 
//

  
string txt(const vector<string>& x);
string txt(vector<string>& x);

template<typename T>
string txt(T&& x){
  
  using Tclean = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
  
  if constexpr (std::is_same_v<Tclean, fs::path>){
    return "\"" + format_path(x) + "\"";
    
  } else if constexpr (std::is_convertible_v<Tclean, std::string> or std::is_convertible_v<Tclean, std::string_view>) {
    return x;
    
  } else if constexpr (std::is_pointer_v<Tclean>){
    return std::to_string(reinterpret_cast<uintptr_t>(x));
    
  } else {
    return std::to_string(x);
  }
}

template<typename T>
string vector_to_string(vector<T>&& x){
  if(x.empty()){
    return "[empty]";
  }
  
  using Tclean = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
  
  string res;
  int iter = 0;
  
  for(auto &value : x){
    
    if(iter++ > 0){
      res += ", ";
    }
    
    if constexpr (std::is_same_v<Tclean, std::string>){
      res += "\"" + value + "\"";
      
    } else if constexpr (std::is_convertible_v<Tclean, std::string> or std::is_convertible_v<Tclean, std::string_view>) {
      res += "\"" + static_cast<std::string>(value) + "\"";
      
    } else if constexpr (std::is_pointer_v<Tclean>){
      res += std::to_string(reinterpret_cast<uintptr_t>(value));
      
    } else {
      res += std::to_string(value);
    }
  }
  
  return res;
}

template<typename T>
string txt(vector<T>&& x){
  return vector_to_string<T>(std::forward<vector<T>&&>(x));
}

template<typename T>
string txt(vector<T>& x){
  return vector_to_string<T>(std::forward<vector<T>&&>(x));
}

template<typename T>
string txt(const vector<T>& x){
  return vector_to_string<T>(std::forward<vector<T>&&>(x));
}

template<typename T1, typename T2>
string txt(T1&& x, T2&& y){
  return (txt(x) + txt(y));
}

template<typename T1, typename T2, typename... T_rest>
string txt(T1&& x, T2&& y, T_rest... rest){
  return txt(x) + txt(y, rest...);
}

inline string txt(vector<string>& x){
  return vector_to_string<std::string>(std::forward<vector<string>>(x));
}

//
// message 
//

namespace internal {
  static int debug_depth = 0;
}

enum class DEBUG_TYPE {
  NONE,
  MSG,
  PIPE
};

extern DEBUG_TYPE _debug;

void write_debug(const string &x);

void next_debug_type();
inline void set_debug_to_file(){ _debug = DEBUG_TYPE::PIPE; }
inline void set_debug_to_msg(){ _debug = DEBUG_TYPE::MSG; }

template<typename... T_all>
void msg(T_all&&... all){
  string t = txt(all...);
  std::cout << t << "\n";
}

// the class and the debug function share the same name 
// so they're easy to locate and remove
template<typename... T_all>
void debug_msg(T_all&&... all){
  
  if(_debug == DEBUG_TYPE::NONE){
    return;
  }
  
  string t = txt(all...);
  
  if(internal::debug_depth > 0){
    t = string(internal::debug_depth, '=') + " " + t;
  }
  
  if(_debug == DEBUG_TYPE::PIPE){
    write_debug(t);
  } else {
    std::cout << FG_DEBUG << t << FG_DEFAULT << "\n";
  }
  
}

class Debug_Msg {
public:

  template<typename... T_all>
  Debug_Msg(T_all&&... all){
    ++internal::debug_depth;
    
    if(_debug != DEBUG_TYPE::NONE){
      string t = txt(all...);
      debug_msg(FG_DEBUG_TITLE + t + FG_DEFAULT);
    }
  }
  
  ~Debug_Msg(){
    --internal::debug_depth;
  }
};

template<typename... T_all>
void error_msg(T_all&&... all){
  // print an error message in red
  string t = txt(all...);
  
  std::cerr << FG_ERROR << t << FG_DEFAULT << "\n";
}

template<typename... T_all>
void info_msg(T_all&&... all){
  // print an error message in red
  string t = txt(all...);
  
  std::cout << FG_INFO << t << FG_DEFAULT << "\n";
}

//
// errors 
//

class internal_error : public std::logic_error {
  public:
    template<typename... T_all>
    explicit internal_error(T_all... all): std::logic_error(txt(all...)){}
  };

class bad_type : public std::runtime_error {
public:
  template<typename... T_all>
  explicit bad_type(T_all... all): std::runtime_error(txt(all...)){}
};

class index_pblm : public std::out_of_range {
public:
  template<typename... T_all>
  explicit index_pblm(T_all... all): std::out_of_range(txt(all...)){}
};

template<typename T> 
inline bool is_out_of_bounds(vector<T> vec, size_t i, string fun_name){
  if(vec.empty()){
    std::cerr << "Error: In " << fun_name << " the vector is empty and cannot be accessed\n";
    return true;
  }
  
  if(i >= vec.size()){
    std::cerr << "Error: In " << fun_name << " the index to access (" << i;
    std::cerr << ") is larger than the vector size (" << vec.size() << ")\n";
    return true;
  }
  
  return false;
}

//
// OnExit
//

// RAII to activate something on exit
class OnExit {
  std::function<void(void)> fun;
public:
  OnExit(std::function<void(void)> fun_input): fun(fun_input) {}
  ~OnExit(){
    fun();
  }
};

//
// testing 
//

static int test_nb = 0;

template<typename T1, typename T2>
void test_eq(const T1 &x, const T2 &y){
  ++test_nb;
  if(x != y){
    throw bad_type("TEST ", test_nb, ": \"", x, "\" != \"", y, "\"");
  }
}

inline void test_eq_str(const string &x, const string &y){
  test_eq<string, string>(x, y);
}

inline void test_eq_vec_str(vector<string> x, vector<string> y){
  if(x.size() != y.size()){
    throw bad_type("The vectors `x` and `y` are not of the same size:\n",
                   "x = ", x, "\ny = ", y);
  }
  
  for(size_t i = 0 ; i < x.size() ; ++i){
    if(x[i] != y[i]){
      throw bad_type("The vectors `x` and `y` have different elements in position ", i, ":\n",
        "x = ", x, "\ny = ", y);
    }
  }
}
  

} // end namepsace util


