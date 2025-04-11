

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <filesystem>
#include <windows.h>


using std::string;
using std::vector;
namespace fs = std::filesystem;

HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);

int main(){
  
  std::cout << "\033[94mTo quit this program, press q\033[39m\n";
  
  // console
  DWORD n_events = 100, n_events_read = 0;
  std::vector<INPUT_RECORD> inputs_read(n_events);
  _KEY_EVENT_RECORD key_in;
  
  fs::path path = "debug.txt";
  fs::file_time_type last_write;
  size_t line_nb = 0;
  int n_sleeps = 0;
  
  while(true){
    
    WaitForSingleObject(handle_in, 5);
    
    GetNumberOfConsoleInputEvents(handle_in, &n_events);
    if(n_events > 0){
      ReadConsoleInputW(handle_in, &inputs_read[0], n_events, &n_events_read);
      
      if(n_events_read > 0 && inputs_read[0].EventType == KEY_EVENT){
        key_in = inputs_read[0].Event.KeyEvent;
        
        if(key_in.bKeyDown){
          const char &ascii = key_in.uChar.AsciiChar;
          if(ascii == 'q'){
            std::cout << "exiting\n";
          }
          
          break;
        }
        
      }
    }
    
    ++n_sleeps;
    
    if(fs::exists(path)){
      fs::file_time_type write_time = fs::last_write_time(path);
      if(write_time > last_write){
        // update 
        
        if(n_sleeps > 5 && line_nb > 0){
          std::cout << "---- \n";
        }
        
        n_sleeps = 0;
        
        std::ifstream file_in(path);
        if(file_in.is_open()){
          size_t i = 0;
          string line;
          while(std::getline(file_in, line)){
            ++i;
            if(i > line_nb){
              line_nb = i;
              std::cout << "\033[33m" << line << "\033[39m\n";
            }
          }
          
          file_in.close();
        } else {
          std::cout << "THE FILE IS NOT OPEN\n\n";
        }
        
      }
      
      last_write = write_time;
    }
    
  }
  
  
  
  
  
  return 0;
}










