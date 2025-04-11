
#pragma once

#include <windows.h>
#ifdef TRUE
  #undef TRUE
#endif
#ifdef FALSE
  #undef FALSE
#endif
#include <string>
#include <vector>

using std::vector;
using std::string;

using uint = unsigned int;


//
// CursorInfo ------------------------------------------------------------------
//


class CursorInfo {
  CONSOLE_SCREEN_BUFFER_INFO info;
  
public:
  uint x_abs = 0;
  uint y_abs = 0;
  uint win_height = 0;
  uint win_width = 0;
  uint n_lines_below = 0;
  
  CursorInfo() = delete;
  CursorInfo(HANDLE handle_console){
    GetConsoleScreenBufferInfo(handle_console, &info);
    x_abs = info.dwCursorPosition.X;
    y_abs = info.dwCursorPosition.Y;
    win_width = info.dwSize.X - 1;
    win_height = info.dwSize.Y;
    n_lines_below = win_height > y_abs ? win_height - y_abs - 1 : 0;
  }
  
};


//
// CursorSelection -------------------------------------------------------------
//

class CursorSelection {
  uint start = 0;
  uint end = 0;
  bool is_sel = false;
  
public:
  CursorSelection() = default;
  CursorSelection(bool x): is_sel(x) {}
  
  uint range_start() const { return start; }
  uint range_end() const { return end; }
  uint range_size() const { return end - start; }
  bool is_selection() const { return is_sel; }
  
  void remove_selection(){ is_sel = false; }
  
  void select_range(uint from, uint to, bool set_both = false){
    if(set_both){
      
      is_sel = true;
      if(from == to){
        is_sel = false;
        
      } else if(from <= to){
        start = from;
        end = to;
        
      } else {
        start = to;
        end = from;
        
      }
      
    } else if(is_sel){
      // it was already a selection => only the 'to' matters
      // unless we ask to reset the two
      
      if(to < start){
        // we swap in this case:
        //  01234567
        //  bonjour|
        //     ^^^^   -> selected
        //  shift+ctrl+left
        // |bonjour
        //  ^^^       -> selected
        //  
        //  in that case cursor_str_x (= arg. `from`) is in pos 7
        //  start is in position 3 and end in position 7
        //  
        
        if(from > start){
          end = start;
          start = to;
        } else {
          start = to;
        }
        
      } else {
        
        if(from == start){
          
          if(to < end){
            // 
            // 012345678901234567
            // |bonjour les gens 
            //  ^^^^^^^^^^^^^^^^
            //  
            //  bonjour |les gens
            //           ^^^^^^^^
            // 
            // cursor_str_x = 0 ; start = 0 ; end = 17 ; to = 9
            // 
            
            start = to;
            
          } else {
            // we swap
            // 01234567
            // |bonjour
            //  ^^^
            //  we want 
            // bonjour|
            //    ^^^^
            // 
            // cursor_str_x = 0 ; start = 0 ; end = 4 ; to = 7
            // 
            start = end;
            end = to;
          }
          
        } else {
          end = to;
        }
        
      }
      
      if(end == start){
        is_sel = false;
      }
      
    } else if(from != to){
      is_sel = true;
      
      if(from < to){
        start = from;
        end = to;
      } else {
        start = to;
        end = from;
      }
    }
  }

};



//
// CommandToEvaluate -----------------------------------------------------------
//

class CommandToEvaluate {
public:
  string cmd;
  bool is_complete = false;
  bool is_parse_error = false;
  
  CommandToEvaluate() = default;
  CommandToEvaluate(string str, bool complete, bool err): cmd(str), is_complete(complete), is_parse_error(err) {};
  
  // implicit conversion to string
  operator string() const { return cmd; }
};



