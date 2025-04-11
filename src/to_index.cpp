

#include "to_index.hpp"
#include <stdint.h>
#include <cmath>
#include <iostream>
#include <cstring>

/* NOTA
* This algorithm, which is pretty quick, is taken from indexthis
* 
* This current implementation is quick and dirty but fits my current needs.
* If I need more, maybe improve and make it cleaner, more general?
* 
* so far I just need it for a single uint64_t vector (easy) so no need to bother
* 
* Later => put everything in 64 bits correctly
* */

namespace {

inline int power_of_two(double x){
  return std::ceil(std::log2(x + 1));
}

inline uint32_t hash_single(size_t x, int shifter){
  uint32_t y[2];
  std::memcpy(y, &x, sizeof(y));
  uint32_t value = y[0] + y[1];
  return ((3141592653U * value) >> (32 - shifter));
}

} // end anonymous namespace

IndexInfo to_index(const vector<size_t> &x, const to_indexOpts opts){
  
  const bool reverse = opts.is_reverse();
  const bool add_table = opts.is_table();
  
  const int64_t n = x.size();
  
  int shifter = power_of_two(2.0 * n + 1.0);
  if(shifter < 8) shifter = 8;
  size_t larger_n = std::pow(2, shifter);
  
  vector<int32_t> hashed_obs_vec(larger_n + 1, -1);
  
  std::vector<size_t> index(n);
  std::vector<size_t> vec_first_obs;
  std::vector<size_t> table;
  
  size_t g = 0;
  uint32_t id = 0;
  int32_t obs = 0;
  
  int32_t i = reverse ? n - 1 : 0;
  while(i >= 0 && i < n){
    
    id = hash_single(x[i], shifter);
    
    bool does_exist = false;
    while(hashed_obs_vec[id] != -1){
    
      obs = hashed_obs_vec[id];
      if(x[obs] == x[i]){
        index[i] = index[obs];
        does_exist = true;
        break;
      } else {
        ++id;
        if(id > larger_n){
          id %= larger_n;
        }
      }
    }

    if(!does_exist){
      hashed_obs_vec[id] = i;
      index[i] = g++;
      vec_first_obs.push_back(i);
    }
    
    if(add_table){
      if(does_exist){
        ++table[index[i]];
      } else {
        table.push_back(1);
      }
    }
    
    if(reverse){
      --i;
    } else {
      ++i;
    }
  }
  
  return IndexInfo(index, vec_first_obs, table);
}


IndexInfo to_index(const vector<std::string> &x, const to_indexOpts opts){
  // we hash the string ex ante
  
  vector<size_t> x_size_t(x.size());
  for(size_t i = 0 ; i < x.size() ; ++i){
    x_size_t[i] = std::hash<std::string>{}(x[i]);
  }
  return to_index(x_size_t, opts);
}

