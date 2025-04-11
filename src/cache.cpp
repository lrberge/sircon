
#include "cache.hpp"


// the static values
const string CachedData::SEPARATOR = "^^^";
fs::path CachedData::root_path = "";
std::map<string, CachedData::ptr_vec_vec_string> CachedData::global_cache = std::map<string, CachedData::ptr_vec_vec_string>();

CachedData::CachedData(string name, TYPE type){
  
  cache_name = name;
  cache_type = type;
  
  //
  // step 1: we look at whether the object is already cached in memory 
  //
  
  if(auto loc = global_cache.find(cache_name) ; loc != global_cache.end()){
    // the cache exists!
    cache_exists = true;
    data = global_cache.at(cache_name);
    return;
  }
  
  if(type == TYPE::ONLY_MEMORY){
    return;
  }
  
  //
  // step 2: we check whether the object exists on disk 
  //

  if(root_path.empty()){
    std::cerr << "Internal error: The current CACHE path was not set. You must set it before using CachedData. Please fix.\n";
    return;
  }

  cache_path = root_path / cache_name;
  
  if(fs::exists(cache_path)){
    
    // the date 
    date_modified = fs::last_write_time(cache_path);
    
    // we read the file
    std::ifstream cache_in{cache_path};
    if(!cache_in.is_open()){
      // NOTA: LATER, make it silent (the user does not need to know)
      std::cerr << "Could not read the existing cache file at:\n'" << cache_path << "'\n";
      return;
    }
    
    cache_exists = true;
    
    // NOTA:
    // - we assume the data is well formatted
    
    // we initialize the data
    data = std::make_shared<vec_vec_string>();
    data->push_back(vector<string>());
    vector<string> *pvec = &(data->at(0));
    
    std::string line;
    while(std::getline(cache_in, line)){
      
      if(line == SEPARATOR){
        // we go to the next vector
        data->push_back(vector<string>());
        pvec = &(data->back());
      } else {
        pvec->push_back(line);
      }
      
    }
    
    // we remove the last empty line
    pvec->pop_back();
    
    cache_in.close();
    
    // we save the data in the global cache
    global_cache[cache_name] = data;
  }
  
}


void CachedData::write_cache(){
  
  //
  // 1) we make sure the folder exists
  //
  
  string err;
  int status = util::create_parent_path(cache_path, 3, util::DoCheck(err));
  
  if(status != 0){
    // LATER: Make it silent
    util::error_msg("Could not write the cache at:\n'", cache_path);
    util::error_msg(err);
    return;
  }
  
  //
  // 2) we write 
  //
  
  
  std::ofstream cache_out{cache_path};
  
  if(!cache_out.is_open()){
    // NOTA: LATER, make it silent (the user does not need to know)
    
    std::cerr << "Could not write the cache at:\n'" << cache_path << "'\n";
    return;
  }
  
  for(size_t index = 0 ; index < data->size() ; ++index){
    
    const vector<string> &vec = data->at(index);
    const size_t n = vec.size();
    
    for(size_t i = 0 ; i < n ; ++i){
      cache_out << vec[i] << "\n";
    }
    
    if(index + 1 < data->size()){
      cache_out << SEPARATOR << "\n";
    }
    
  }
  
  cache_out << "\n";
  
  cache_out.close();
  
  date_modified = fs::last_write_time(cache_path);
  
}



double CachedData::seconds_since_last_write(){
    
  if(is_unset()){
    return 1e10;
  }
  
  if(cache_type == TYPE::ONLY_MEMORY){
    return 0;
  }
  
  fs::path dummy_path = root_path / "dummy_time";
  std::ofstream dummy{dummy_path};
  dummy.close();
  
  double seconds = std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(dummy_path) - date_modified).count();
  
  return seconds;
};

double CachedData::hours_since_last_write(){
  double seconds = seconds_since_last_write();
  return seconds / 3600.0;
};

double CachedData::days_since_last_write(){
  double seconds = seconds_since_last_write();
  return seconds / 3600.0 / 24.0;
};

vector<string> CachedData::get_cached_vector(size_t index){
  
  if(!cache_exists){
    std::cerr << "Error: trying to access a cache that does not yet exists.\n";
  }
  
  if(index >= data->size()){
    std::cerr << "Error when getting the cache for '" << cache_name << 
      "'.\nRequesting vector in position " << index << ", but there are only " << data->size() <<
      " vectors in the cache.\n";
  }
  
  return data->at(index);
}

CachedData& CachedData::set_cached_vector(const vector<string> &x){
  
  data = std::make_shared<vec_vec_string>();
  data->push_back(x);
  
  cache_exists = true;
  
  if(cache_type == TYPE::ON_DISK){
    write_cache();
  }
  
  return *this;
}

CachedData& CachedData::set_cached_vectors(std::initializer_list<vector<string>> vecs, SIZE size){
  
  data = std::make_shared<vec_vec_string>(vecs);
  
  if(size == SIZE::ALL_EQUAL && data->size() > 1){
    const size_t n_vecs = data->size();
    const size_t s0 = data->at(0).size();
    for(size_t i = 1 ; i < n_vecs ; ++i){
      size_t si = data->at(0).size();
      if(s0 != si){
        std::cerr << "Error when caching '" << cache_name << 
          "', the vectors to be cached must be of the same size. " <<
          "\nPROBLEM: vector in position 0 is of size " << s0 << 
          " while vector in position " << i << " is of size" << si << ".\n";
        
        return *this;
      }
    }
  }
  
  cache_exists = true;
  
  if(cache_type == TYPE::ON_DISK){
    write_cache();
  }
  
  return *this;
}


