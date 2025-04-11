
#pragma once

#include "util.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
#include <iostream>

namespace fs = std::filesystem;
using std::string;
using std::vector;


class CachedData {
  
public:
  // public types
  
  enum class TYPE {
    ON_DISK,
    ONLY_MEMORY,
  };
  
  enum class SIZE {
    ALL_EQUAL,
    UNEQUAL,
  };
  
private:
  
  using vec_string = vector<string>;
  using vec_vec_string = vector<vec_string>;
  using ptr_vec_vec_string = std::shared_ptr<vec_vec_string>;
  
  // static values
  static std::map<string, ptr_vec_vec_string> global_cache;
  static const string SEPARATOR;
  
  string cache_name;
  fs::path cache_path;
  std::shared_ptr< vec_vec_string > data;
  fs::file_time_type date_modified;
  
  bool cache_exists = false;
  TYPE cache_type = TYPE::ON_DISK;
  
  void write_cache();
  
public:
  
  static fs::path root_path;
  
  CachedData(string cache_name, TYPE cache_type = TYPE::ON_DISK);
  
  bool is_unset(){ return !cache_exists; }
  
  bool exists(){ return cache_exists; }
  
  double seconds_since_last_write();
  double hours_since_last_write();
  double days_since_last_write();
  
  vector<string> get_cached_vector(size_t index = 0);
  
  CachedData& set_cached_vector(const vector<string> &);
  CachedData& set_cached_vectors(std::initializer_list<vector<string>> vecs, 
                                 SIZE size = SIZE::ALL_EQUAL);

};

