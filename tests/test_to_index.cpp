

#include "../to_index.hpp"
#include <iostream>
#include <stdint.h>
#include <vector>

using std::vector;

void print_vector(const vector<size_t> &x){
  const size_t n = x.size();
  std::cout << "{";
  std::cout << x[0];
  for(size_t i=1 ; i<n ; ++i){
    std::cout << ", " << x[i];
  }
  std::cout << "}\n";
}

int main(){
  
  std::cout << "Initializing the input vector\n";
  vector<size_t> x = {7, 3, 3, 3, 7, 7, 5, 6, 7, 3, 3};
  
  std::cout << "Computing the index\n";
  IndexInfo index = to_index(x);
  std::cout << "Extracting the first_obs vector\n";
  vector<size_t> first_obs = index.get_first_obs();
  
  std::cout << "Printing the vectors\n";
  print_vector(x);
  print_vector(index.get_index());
  print_vector(first_obs);
  
  std::cout << "\nNow in reverse\n";
  index = to_index(x, to_indexOpts().reverse());
  print_vector(x);
  print_vector(index.get_index());
  print_vector(index.get_first_obs());
  
  return 0;
}


