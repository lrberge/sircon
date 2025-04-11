
#include "../src/util.hpp"
#include "../src/shellrun.hpp"


int main(){
  
  string p = run_shell_command("where.exe", "R.exe");
  
  util::msg("Path = '", p, "'");
  
  return 0;
}


