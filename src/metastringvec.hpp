
#pragma once

#include "util.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <memory>

using std::string;
using std::vector;

namespace stringtools {
  
using vec_str = vector<string>;
using ptr_vec_str = std::shared_ptr<vec_str>;
using vec_vec_str = vector<vec_str>;

template<typename T>
inline size_t find_pos_first_match(const vector<T> &vec, const T value){
  const size_t n = vec.size();
  for(size_t i = 0 ; i < n; ++i){
    if(vec[i] == value){
      return i;
    }
  }
  
  return n;
}

inline bool all_true(const vector<bool> &x, const bool on_empty = true){
  
  if(x.empty()){
    return on_empty;
  }
  
  for(auto v : x){
    if(!v){
      return false;
    }
  }
  
  return true;
}

inline bool any_false(const vector<bool> &x, const bool on_empty = false){
  
  if(x.empty()){
    return on_empty;
  }
  
  for(auto v : x){
    if(!v){
      return true;
    }
  }
  
  return false;
}

//
// Meta ------------------------------------------------------------------------ 
//

// NOTA: initialization with a single string => value for all the elements
// with a vector => 1-to-1 match between vector value and element id
class Meta {
  vec_vec_str all_vecs;
  size_t n = 0;
  vec_str all_keys;
  vector<size_t> selection;
  vector<bool> is_flex;
  vector<bool> is_inherit;
  
  void error_empty(const vec_str &x){
    if(x.empty()){
      throw std::range_error("The vector in input cannot be empty");
    }
  }
  
  void error_position(const size_t obs) const {
    if(obs >= n && any_false(is_flex, true)){
      throw util::bad_type("The position of the observation requested (",
                       obs, ") is larger than the number of observations (",
                       n, ").");
    }
  }
  
public:
  
  enum class SCALAR_TYPE {
    INHERIT,
    NO_INHERIT,
  };
  
  //
  // initializations 
  //
  
  
  Meta() = default;
  
  Meta(size_t n_): n(n_){}
  
  // string initialization
  Meta(const string x, SCALAR_TYPE stype = SCALAR_TYPE::INHERIT, size_t n_ = 1): n(n_){
    all_vecs.push_back(vec_str{n, x});
    all_keys.push_back("");
    is_flex.push_back(true);
    is_inherit.push_back(stype == SCALAR_TYPE::INHERIT);
  }
  
  Meta(const string key, const string x, SCALAR_TYPE stype = SCALAR_TYPE::INHERIT, 
       size_t n_ = 1): n(n_){
    all_vecs.push_back(vec_str{n, x});
    all_keys.push_back(key);
    is_flex.push_back(true);
    is_inherit.push_back(stype == SCALAR_TYPE::INHERIT);
  }
  
  // string vector initialization
  Meta(const vec_str &x){
    error_empty(x);
    n = x.size();
    all_vecs.push_back(x);
    all_keys.push_back("");
    is_flex.push_back(false);
    is_inherit.push_back(false);
  }
  
  Meta(const string key, const vec_str &x){
    error_empty(x);
    n = x.size();
    all_vecs.push_back(x);
    all_keys.push_back(key);
    is_flex.push_back(false);
    is_inherit.push_back(false);
  }
  
  
  //
  // functions 
  //
  
  Meta& resize(size_t n_new){
    // we only set n to the new size. should not work if n != 1
     
    if(n == 0){
      n = n_new;
    } else if(n_new == n){
      // nothing
    } else if(all_true(is_flex)){
      n = n_new;
    } else {
      throw util::index_pblm("The current resize is invalid: old size = ", n, 
                             ", new size = ", n_new, ".\n", 
                             "This is not possible with non inherited scalars.");
    }
    
    return *this;
  }
  
  Meta& clear(){
    selection.clear();
    n = 0;
    all_vecs.clear();
    all_keys.clear();
    is_flex.clear();
    is_inherit.clear();
    
    return *this;
  }
  
  Meta& push_back(const string x, SCALAR_TYPE stype = SCALAR_TYPE::INHERIT){
    
    all_vecs.push_back(vec_str{1, x});
    all_keys.push_back("");
    is_flex.push_back(true);
    is_inherit.push_back(stype == SCALAR_TYPE::INHERIT);
    
    return *this;
  }
  
  Meta& push_back(const vec_str &x){
    
    // we check the size of the input
    const size_t n_x = x.size();
    if(n > 0 && n_x != n){
      throw std::range_error("The vector to be pushed back must be of the same length as the current Meta data or of length 1. This is not the case.");
    }
    
    if(n == 0){
      resize(n_x);
    }
    
    all_vecs.push_back(x);
    all_keys.push_back("");
    is_flex.push_back(false);
    is_inherit.push_back(false);

    return *this;
  }
  
  // we cannot push back with names
  Meta& set(const string key, const string x, SCALAR_TYPE stype = SCALAR_TYPE::INHERIT){
    
    size_t pos = find_pos_first_match(all_keys, key);
    if(pos < all_vecs.size()){
      // found
      all_vecs[pos] = vec_str{1, x};
      all_keys[pos] = key;
      is_flex[pos] = true;
      is_inherit[pos] = stype == SCALAR_TYPE::INHERIT;
    } else {
      all_vecs.push_back(vec_str{1, x});
      all_keys.push_back(key);
      is_flex.push_back(true);
      is_inherit.push_back(stype == SCALAR_TYPE::INHERIT);
    }
    
    return *this;
  }
  
  Meta& set(const string key, const vec_str &x){
    
    const size_t n_x = x.size();
    if(n == 0){
      n = n_x;
    } else if(n != n_x){
      throw util::index_pblm("Size of the vector to be set (",
                             n_x, ") differs from the existing size (",
                             n, "). This is not possible for non inherited scalars.");
    }
    
    const size_t pos = find_pos_first_match(all_keys, key);
    if(pos < all_keys.size()){
      // found
      all_vecs[pos] = x;
      all_keys[pos] = key;
      is_flex[pos] = false;
      is_inherit[pos] = false;
    } else {
      all_vecs.push_back(x);
      all_keys.push_back(key);
      is_flex.push_back(false);
      is_inherit.push_back(false);
    }
    
    return *this;
  }
  
  Meta& rm_key(const string key, const bool check = false){
    
    size_t pos = find_pos_first_match(all_keys, key);
    if(pos < all_vecs.size()){
      // found
      all_vecs.erase(all_vecs.begin() + pos);
      all_keys.erase(all_keys.begin() + pos);
      is_flex.erase(is_flex.begin() + pos);
      is_inherit.erase(is_inherit.begin() + pos);
      
    } else if(check){
      throw std::out_of_range(util::txt("The key `", key, "` does not exist among ", 
                                        all_keys.size(), "keys."));
    }
    
    return *this;
  }
  
  Meta& set_names(const vec_str key){
    if(key.size() != all_vecs.size()){
      throw std::range_error(util::txt("The length of the keys (", key.size(), 
                                       ") does not match the number of existing vectors (", 
                                       all_vecs.size(), ")."));
    }
    
    all_keys = key;
    
    return *this;
  }
  
  //
  // access 
  //
  
  Meta at(const size_t i) const {
    
    if(i >= n && any_false(is_flex)){
      throw std::out_of_range(util::txt("The index to select (",
                                        i, ") is larger than the number of observations (",
                                        n, ")."));
    }
    
    Meta res;
    
    int k = 0;
    for(auto &v : all_vecs){
      if(is_flex[k]){
        res.set(all_keys.at(k++), v[0]);
      } else {
        res.set(all_keys.at(k++), v[i]);
      }
    }
    
    return res;
  }
  
  const vec_str& get_vector(const string key) const {
    const size_t k = find_pos_first_match(all_keys, key);
    if(k >= all_keys.size()){
      throw std::range_error(util::txt("The key `", key, "`does not exist."));
    }
    
    return all_vecs.at(k);
  }
  
  vec_str& get_vector(const string key){
    const size_t k = find_pos_first_match(all_keys, key);
    if(k >= all_keys.size()){
      throw std::range_error(util::txt("The key `", key, "`does not exist."));
    }
    
    return all_vecs.at(k);
  }
  
  const vec_str& get_vector(const size_t k) const {
    if(k >= all_vecs.size()){
      throw std::range_error(util::txt("The position ",  k, 
                                       " does not fit the number of elements (",
                                       all_vecs.size(), ")."));
    }
    
    return all_vecs[k];
  }
  
  vec_str& get_vector(const size_t k){
    if(k >= all_vecs.size()){
      throw std::range_error(util::txt("The position ", k, 
                                       " does not fit the number of elements (",
                                       all_vecs.size(), ")."));
    }
    
    return all_vecs[k];
  }
  
  vec_str get_vector_copy(string key) const {
    vec_str res = get_vector(key);
    return res;
  }
  
  vec_str get_vector_copy(size_t k) const {
    vec_str res = get_vector(k);
    return res;
  }
  
  string get_vector_at(const string key, const size_t obs = 0) const {
    error_position(obs);
    const vec_str &vec = get_vector(key);
    
    // inheritance
    if(vec.size() == 1){
      return vec[0];
    }
    
    return vec[obs];
  }
  
  string get_vector_at(const size_t k, const size_t obs = 0) const {
    error_position(obs);
    const vec_str &vec = get_vector(k);
    
    if(vec.size() == 1){
      return vec[0];
    }
    
    return vec[obs];
  }
  
  vec_str get_row(const size_t obs) const {
    error_position(obs);
    vec_str res;
    for(auto &vec : all_vecs){
      if(vec.size() == 1){
        res.push_back(vec[0]);
      } else {
        res.push_back(vec[obs]);
      }
    }
    
    return res;
  }
  
  vec_str get_keys() const {
    return all_keys;
  }
  
  size_t get_n_obs() const {
    return n;
  }
  
  size_t get_n_vectors() const {
    return all_vecs.size();
  }
  
  bool empty() const {
    return n == 0;
  }
  
  bool unset() const {
    return n == 0 && all_vecs.empty();
  }
  
  bool all_flex() const {
    return all_true(is_flex, true);
  }
  
  
  //
  // selection 
  //
  
  
  template<typename T>
  Meta& select(const vector<T> &sel){
    // should I allow empty selections?
    // yes, it's more robust
    
    static_assert(std::is_convertible_v<T, size_t>, 
      "In `select`, values for selection must be represented by unsigned integers.");
    
    int k = 0;
    for(auto &vec : all_vecs){
      if(is_flex[k++]) {
        // nothing => remains flex
      } else {
        
        if(n != vec.size()){
          throw util::bad_type("Internal error: the length of the internal vector, ", vec.size(), 
                               ",  and that of the internal size, ", n, ",  differ.");
        }
        
        vec_str new_vec;
        for(const auto &i: sel){
          
          if(i >= n || i < 0){
            throw util::index_pblm("When processing the selection, the selection ID (",
                                   i, ") is invalid given the number of observations ", 
                                   n, ".");
          }
          
          new_vec.push_back(vec[i]);
        }
        vec = std::move(new_vec);
      }
    }
    
    n = sel.size();
    selection.clear();
    
    return *this;
  }
  
  //
  // binding 
  //
  
  Meta& bind(const Meta& x){
    
    if(x.unset()){
      return *this;
      
    } else if(unset()){
      if(this != &x){
        *this = x;
      }
      
      return *this;
    }
    
    // general information
    size_t K_left = all_vecs.size(), K_right = x.all_vecs.size();
    size_t n_left = n, n_right = x.n;
    vector<bool> done_right(K_right);
    
    for(size_t k = 0 ; k < K_left ; ++k){
      const size_t pos_right = find_pos_first_match(x.all_keys, all_keys[k]);
      vec_str &vec_left = all_vecs[k];
      
      if(pos_right >= K_right){
        // no match found
        if(is_flex[k]){
          if(is_inherit[k]){
            // we inherit the flex status => we do nothing
          } else {
            // we need to resize
            vec_left = vec_str(n_left, vec_left.at(0)); 
            //         rep(vec_left, n_left)
            is_flex[k] = false;
            vec_left.insert(vec_left.end(), n_right, ""); 
            //       .push_back(vec_right)
          }
        } else {
          vec_left.insert(vec_left.end(), n_right, "");
        }
      } else {
        // found
        done_right[pos_right] = true;
        const vec_str &vec_right = x.all_vecs.at(pos_right);
        const bool flex_left = is_flex[k], flex_right = x.is_flex[pos_right];
        if(flex_left && flex_right && vec_left[0] == vec_right[0]){
          // nothing to do, that's fine!
          //
        } else {
          is_flex[k] = false;
          
          if(flex_left){
            vec_left = vec_str(n_left, vec_left.at(0));
          }
          
          if(flex_right){
            vec_left.insert(vec_left.end(), n_right, vec_right[0]);
          } else {
            vec_left.insert(vec_left.end(), vec_right.begin(), vec_right.end());
          }
        }
        
      }
    }
    
    
    // the remaining vectors
    for(size_t k = 0 ; k < K_right ; ++k){
      if(done_right[k]){
        continue;
      }
      
      const vec_str &vec_right = x.all_vecs.at(k);
      
      if(x.is_flex[k] && x.is_inherit[k]){
        all_vecs.push_back(vec_right);
        all_keys.push_back(x.all_keys[k]);
        is_flex.push_back(true);
        is_inherit.push_back(true);
        
      } else {
        vec_str vec(n_left, "");
        if(x.is_flex[k]){
          vec_str new_vec(n_right, vec_right[0]);
          vec.insert(vec.end(), new_vec.begin(), new_vec.end());
        } else {
          vec.insert(vec.end(), vec_right.begin(), vec_right.end());
        }
        
        all_vecs.push_back(std::move(vec));
        
        all_keys.push_back(x.all_keys[k]);
        is_flex.push_back(false);
        is_inherit.push_back(false);
        
      }
      
    }
    
    n = n_left + n_right;
    selection.clear();
    
    return *this;
    
  }
  
  bool is_key(const string &key) const {
    size_t pos = find_pos_first_match(all_keys, key);
    return pos < all_keys.size();
  }
  
  bool is_key(const string &&key) const {
    size_t pos = find_pos_first_match(all_keys, key);
    return pos < all_keys.size();
  }
  
  
};

//
// MetaStringVec --------------------------------------------------------------- 
//


class MetaStringVec {
  
  using MSV = MetaStringVec;
  
  ptr_vec_str psvec = std::make_shared<vec_str>();
  std::shared_ptr<Meta> pmeta = std::make_shared<Meta>();
  
  string cause_empty;
  vector<size_t> selection;
  
  // a few functions (mostly internal versions of templated functions)
  void set_meta_internal(const string &key, const vec_str &x){
    const size_t n = x.size();
    if(n != psvec->size()){
      throw util::index_pblm("The number of elements in to be set in meta (",
                             n, ") is different from the current number ",
                             "of observations in the string vector (", 
                             psvec->size(), ").");
    }
    pmeta->set(key, x);
  }
  
public:

  MetaStringVec() = default;
  
  MetaStringVec(const vec_str &x){
    psvec = std::make_shared<vec_str>(x);
  }
  
  MSV& set_meta(Meta &&m){
    
    const size_t meta_obs = m.get_n_obs();
    
    if(psvec->empty()){
      if(meta_obs != 0){
        throw util::index_pblm("The number of observations of the meta information (",
                               meta_obs, ") cannot be greater than 0 ",
                               "when the string vector is empty.");
      }
      
    } else if(meta_obs != 0 && meta_obs != psvec->size()){
      throw util::index_pblm("The number of observations of the meta information (",
                             meta_obs, ") does not match the number of observations of the vector (", 
                             psvec->size(), ").");
    }
    
    pmeta = std::make_shared<Meta>(m);
    
    return *this;
  }
  
  MSV& set_meta(Meta &m){
    set_meta(std::forward<Meta>(m));
    return *this;
  }
  
  MetaStringVec(vec_str &x, Meta &m){
    psvec = std::make_shared<vec_str>(x);
    set_meta(m);
  }
  
  //
  // setters
  //
  
  MSV& set_string_vector(const vec_str &x){
    
    if(x.size() != psvec->size() && !pmeta->empty()){
      throw util::index_pblm(
        "The length of the new vector, ", x.size(), 
        ", does not match the length of the existing vector (", psvec->size(), ").",
        "\nThis is only possible when Meta is not set or contains only inherited scalars.");
    }
    
    psvec = std::make_shared<vec_str>(x);
    return *this;
  }
  
  MSV& set_string_vector(const vec_str &&x){
    
    if(x.size() != psvec->size() && !pmeta->empty()){
      throw util::index_pblm(
        "The length of the new vector, ", x.size(), 
        ", does not match the length of the existing vector (", psvec->size(), ").",
        "\nThis is only possible when Meta is not set or contains only inherited scalars.");
    }
    
    psvec = std::make_shared<vec_str>(x);
    return *this;
  }
  
  MSV& push_back(MSV& x){
    
    // is x == this?
    bool is_same = this == &x;
    
    // 1) the cause of emptyness, we keep the last one only
    if(!is_same && !x.cause_empty.empty()){
      cause_empty = x.cause_empty;
    }
    
    // 2) dealing with empty elements
    if(empty()){
      if(x.empty()){
        return *this;
      }
      
      // we avoid the default copy constructor (bc of cause_empty)
      psvec = x.psvec;
      pmeta = x.pmeta;
      
      return *this;
    }
    
    // 3) we resize the meta information
    if(!empty()){
      pmeta->resize(psvec->size());
    }
    
    if(!is_same && !x.empty()){
      x.pmeta->resize(x.psvec->size());
    }
    
    // 4) we grow the string vectors
    psvec->insert(psvec->end(), x.psvec->begin(), x.psvec->end());
    
    // 5) we grow the meta information
    pmeta->bind(*x.pmeta);
    
    return *this;
  }
  
  MSV& set_meta(const string &key, const vec_str &x){
    set_meta_internal(key, x);
    return *this;
  }
  
  MSV& set_meta(const string &key, const string &x, const Meta::SCALAR_TYPE stype = Meta::SCALAR_TYPE::INHERIT){
    pmeta->set(key, x, stype);
    return *this;
  }
  
  MSV& set_meta(const string &key, const char *px, const Meta::SCALAR_TYPE stype = Meta::SCALAR_TYPE::INHERIT){
    pmeta->set(key, string{px}, stype);
    return *this;
  }
  
  MSV& set_meta(const string &key, const bool &x, const Meta::SCALAR_TYPE stype = Meta::SCALAR_TYPE::INHERIT){
    pmeta->set(key, x ? "true" : "false", stype);
    return *this;
  }
  
  MSV& set_meta(const string &key, const int &x, const Meta::SCALAR_TYPE stype = Meta::SCALAR_TYPE::INHERIT){
    pmeta->set(key, std::to_string(x), stype);
    return *this;
  }
  
  MSV& rm_meta_key(const string &key, const bool check = false){
    pmeta->rm_key(key, check);
    return *this;
  }
  
  MSV& set_cause_empty(const string &x){
    cause_empty = x;
    return *this;
  }
  
  MSV& set_cause_empty(const string &&x){
    cause_empty = x;
    return *this;
  }
  
  string get_cause_empty() const {
    return cause_empty;
  }
  
  MSV& clear(){
    psvec = std::make_shared<vec_str>();
    pmeta = std::make_shared<Meta>();
    cause_empty.clear();
    selection.clear();
    
    return *this;
  }
  
  MSV& clear_meta(){
    pmeta = std::make_shared<Meta>();
    return *this;
  }
  
  //
  // getters 
  //
  
  size_t size() const {
    return psvec->size();
  }
  
  bool empty() const {
    return psvec->empty();
  }
  
  ptr_vec_str get_string_vec_ptr() const {
    return psvec;
  }
  
  const vec_str& get_string_vec() const {
    return *psvec;
  }
  
  const string string_at(size_t i) const {
    if(i >= psvec->size()){
      throw util::index_pblm("The index selected (", 
                             i, ") is larger than the number of observations (", 
                             psvec->size(), ").");
    }
    
    return psvec->at(i);
  }
  
  const std::shared_ptr<Meta> get_meta_ptr() const {
    return pmeta;
  }
  
  bool has_meta() const {
    return !pmeta->unset();
  }
  
  bool has_meta(const string &key) const {
    // alias to is_meta_key
    return pmeta->is_key(key);
  }
  
  Meta& get_meta() const {
    return *pmeta;
  }
  
  Meta& get_meta(){
    if(!pmeta){
      pmeta = std::make_shared<Meta>();
    }
    return *pmeta;
  }
  
  bool is_meta_key(const string &key) const {
    return pmeta->is_key(key);
  }
  
  bool is_meta_key(const string &&key) const {
    return pmeta->is_key(key);
  }
  
  vec_str meta(const string key) const {
    if(!is_meta_key(key)){
      throw util::index_pblm("The key `", key, "` is invalid.");
    }
    
    return pmeta->get_vector(key);
  }
  
  string meta_at(const string key, const size_t i) const {
    if(!is_meta_key(key)){
      throw util::index_pblm("The key `", key, "` is invalid.");
    }
    
    if(i >= psvec->size()){
      throw util::index_pblm("The position ", i, " is larger ",
                             "than the number of observations ", psvec->size(), ".");
    }
    
    return pmeta->get_vector_at(key, i);
  }
  
  string meta_at(const string key, const size_t i, const char* the_default) const {
    if(!is_meta_key(key)){
      return string{the_default};
    }
    
    if(i >= psvec->size()){
      throw util::index_pblm("The position ", i, " is larger ",
                             "than the number of observations ", psvec->size(), ".");
    }
    
    return pmeta->get_vector_at(key, i);
  }
  
  template<typename T>
  T meta_at(const string key, const size_t i, const T the_default) const {
    if(!is_meta_key(key)){
      return the_default;
    }
    
    if(i >= psvec->size()){
      throw util::index_pblm("The position ", i, " is larger ",
                             "than the number of observations ", psvec->size(), ".");
    }
    
    return static_cast<T>(pmeta->get_vector_at(key, i));
  }
  
  MSV at(const size_t i) const {
    
    const size_t n = psvec->size();
    if(i >= n){
      throw util::index_pblm("The selection ID (", i, ") is larger than ",
                             "the number of observations (", n, ").");
    }
    
    MSV res;
    ptr_vec_str psvec_new = std::make_shared<vec_str>(1, psvec->at(i));
    res.psvec = psvec_new;
    
    res.set_meta(pmeta->at(i));
    
    return res;
  }
  
  //
  // selection 
  //
  
  template<typename T>
  MSV& select(const vector<T>& sel){
    
    if(sel.empty()){
      clear();
      return *this;
    }
    
    static_assert(std::is_convertible_v<T, size_t>, 
                  "In `select`, values for selection must be represented by unsigned integers.");
    
    // meta side
    pmeta->select(sel);
    
    // string vector side
    ptr_vec_str pvec_new = std::make_shared<vec_str>();
    const size_t n = psvec->size();
    
    for(auto &i : sel){
      if(i >= n || i < 0){
        throw util::index_pblm("When processing the selection, the selection ID (",
                               i, ") is invalid given the number of observations ", n, ".");
      }
      
      pvec_new->push_back(psvec->at(i));
    }
    
    selection.clear();
    psvec = pvec_new;
    
    return *this;
  }
  
  
  //
  // copy 
  //
  
  MetaStringVec copy() const {
    MetaStringVec new_obj;
    
    *new_obj.psvec = *psvec;
    *new_obj.pmeta = *pmeta;
    
    new_obj.cause_empty = cause_empty;
    new_obj.selection = selection;
    
    return new_obj;
  }
  
  
};


//
// MetaString ------------------------------------------------------------------ 
//


class MetaString {
  string str;
  vec_str meta_vec;
  vec_str all_keys;

public:
  
  MetaString() = delete;
  
  MetaString(const MetaStringVec &x): str(x.string_at(0)){
    if(x.has_meta()){
      const Meta &info = x.get_meta();
      all_keys = info.get_keys();
      meta_vec = info.get_row(0);
    }
  }
  
  MetaString(const string s, const vec_str mv, const vec_str key):
    str(s), meta_vec(mv), all_keys(key){
    // checks
    if(meta_vec.size() != all_keys.size()){
      throw util::index_pblm("The length of the meta values (", meta_vec.size(), 
                             ") must match the length of their names (", 
                             all_keys.size(), ").");
    }
  }
  
  MetaString(const string &s): str(s) {}
  
  bool has_key(const string &key) const {
    size_t pos = find_pos_first_match(all_keys, key);
    return pos < meta_vec.size();
  }
  
  string get_meta(const string key) const {
    size_t pos = find_pos_first_match(all_keys, key);
    if(pos >= meta_vec.size()){
      throw util::index_pblm("The key `", key, "` is invalid.");
    }
    
    return meta_vec[pos];
  }
  
  string get_string() const {
    return str;
  }
  
  MetaString& set_string(const string &x) {
    str = x;
    return *this;
  }
  
};



} // namespace stringtools

