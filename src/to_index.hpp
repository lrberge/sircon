
#pragma once

#include "util.hpp"

#include <string>
#include <vector>
using std::vector;

using util::bad_type;
using util::msg;

class IndexInfo {
  vector<size_t> index;
  vector<size_t> first_obs;
  vector<size_t> table;
public:
  IndexInfo(vector<size_t> index_in, vector<size_t> first_obs_in, 
            vector<size_t> table_count): 
    index(index_in), first_obs(first_obs_in), table(table_count) {}
  
  vector<size_t> get_index() const {
    return index;
  }
  
  vector<size_t> get_first_obs() const {
    return first_obs;
  }
  
  vector<size_t> get_table() const {
    return table;
  }
};

class to_indexOpts {
  bool do_reverse;
  bool do_table;
  
public:
  
  to_indexOpts& reverse() {
    do_reverse = true;
    return *this;
  }
  
  to_indexOpts& table() {
    do_table = true;
    return *this;
  }
  
  bool is_reverse() const { return do_reverse; }
  
  bool is_table() const { return do_table; }
  
};

IndexInfo to_index(const vector<size_t> &x, const to_indexOpts opts = to_indexOpts());

IndexInfo to_index(const vector<std::string> &x, const to_indexOpts opts = to_indexOpts());






