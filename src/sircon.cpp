
#include "console.hpp"
#include "rlanguageserver.hpp"


int main(int argc, char **argv){
  
  RLanguageServer lgsrv(argc, argv);
  lgsrv.main_loop();
  
  return 0;
}





