    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#include "console.hpp"
#include "rlanguageserver.hpp"


int main(int argc, char **argv){
  
  RLanguageServer lgsrv(argc, argv);
  lgsrv.main_loop();
  
  return 0;
}





