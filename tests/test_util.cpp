
#include "../src/util.hpp"
#include<thread>

using namespace util;


int main(){
  
  msg("foramat_path");
  
  string path_weird = "bon\\\\jour/\\les\\gens";
  test_eq_str(format_path(path_weird), "bon/jour/les/gens");
  
  msg("Time");
  
  util::time_t tx = std::chrono::system_clock::now();
  
  std::this_thread::sleep_for(util::duration_ms_t(500));
  
  util::time_t ty = std::chrono::system_clock::now();
  
  msg("I slept for 0.5s, here are the timings obtained with the functions:");
  std::cout << timediff_us(tx, ty) << " us\n";
  std::cout << timediff_ms(tx, ty) << " ms\n";
  std::cout << timediff_sec(ty, tx) << " sec\n";
  
  msg("Time up to now:");
  std::cout << elapsed_us(tx) << " us\n";
  std::cout << elapsed_ms(tx) << " ms\n";
  std::cout << elapsed_sec(tx) << " sec\n";
  
  msg("all tests performed successfully");
  
  return 0;
}

