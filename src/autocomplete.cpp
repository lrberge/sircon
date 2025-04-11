
#include "autocomplete.hpp"
#include "console.hpp"

str::MetaStringVec suggest_path(string &context, const bool add_quotes){
  // Examples of contexts
  // 
  // context = '/'
  //           './bon/jo'
  //           '../'
  //           'src\\module\\'
  //           
  // Algorithm
  // 1) is the current context a folder?
  //   a) yes: we list all the files 
  //   b) no:  we go to the parent directory (if it exists) and list the files
  // 2) list all the files and return them
  // 
  // causes of a 0-length return:
  // 1. no files in the directory
  // 2. the directory does not exist
  // 
  
  string original_context = add_quotes ? context : "";
  
  //
  // step 1: normalizing the path 
  //
  
  // we need to add the trailing spaces to the modified context
  string trailing_space = "";
  int i = context.size() - 1;
  while(i >= 0 && context[i--] == ' '){
    trailing_space += " ";
  }
  
  string context_clean = util::format_path(context);
  bool is_folder = context_clean.back() == '/';
  
  fs::path origin_path = context_clean;
  fs::path parent_path = origin_path;
  
  if(!is_folder){
    parent_path = origin_path.parent_path();
  }
  
  if(parent_path.string() == ""){
    parent_path = ".";
  }
  
  //
  // step 2: modifying the context
  //
  
  if(is_folder){
    // we return everyone
    context = "";
  } else {
    context = origin_path.filename().string();
  }
  
  context += trailing_space;
  
  //
  // step 3: finding candidates 
  //
  
  
  vector<string> all_paths;
  string cause_empty;
  str::MetaStringVec res;
  
  if(fs::exists(parent_path) && fs::is_directory(parent_path)){
    
    for(auto &p : fs::directory_iterator(parent_path)){
      const fs::path &tmp_path = p.path();
      if(fs::is_directory(tmp_path)){
        all_paths.push_back(tmp_path.filename().string() + "/");
      } else {
        all_paths.push_back(tmp_path.filename().string());
      }
    }
    
    // we continue the autocomp only for folders
    const size_t n = all_paths.size();
    vector<string> add_continue(n);
    bool add_meta = false;
    for(size_t i = 0 ; i < n ; ++i){
      if(all_paths[i].back() == '/'){
        add_meta = true;
        add_continue[i] = "true";
      }
    }
    
    res = all_paths;
    if(add_meta){
      res.set_meta("continue", add_continue);
    }
    
    if(add_quotes){
      // we add quotes if the path contains spaces
      
      string prepend;
      if(context.size() < original_context.size()){
        prepend = original_context.substr(0, original_context.size() - context.size());
      }
      
      string prepend_size = std::to_string(str::utf8::count_wide_chars(prepend));
                  
      vector<string> add_right(n);
      vector<string> add_left(n);
      vector<string> add_n_del(n);
      vector<string> add_shift(n);
      
      bool any_quote = false;
      for(size_t i = 0 ; i < n ; ++i){
        if(str::str_contains(all_paths[i], ' ')){
          any_quote = true;
          add_right[i] = "\"";
          add_left[i] = "\"" + prepend;
          add_n_del[i] = prepend_size;
          if(add_continue[i] == "true"){
            add_shift[i] = "-1";
          }
        }
      }
      
      if(any_quote){
        res.set_meta("append_right", add_right);
        res.set_meta("append_left", add_left);
        res.set_meta("n_delete_left", add_n_del);
        res.set_meta("cursor_shift", add_shift);
      }
      
    }
    
    if(all_paths.empty()){
      res.set_cause_empty("no file found in the current path");
    }
  } else {
    res.set_cause_empty("the current path does not exist");
  }
  
  return res;
  
}

//
// ServerAutocomplete ---------------------------------------------------------- 
//



str::StringMatch ServerAutocomplete::suggest_path(const string &context_raw){
  
  string context = context_raw;
  str::MetaStringVec all_paths = ::suggest_path(context);
  
  return str::string_match(context, all_paths);
}


//
// ConsoleAutocomplete ---------------------------------------------------------
//


uint ConsoleAutocomplete::get_max_width(){
  uint max_width = default_max_width;
  if(max_width > pconcom->win_width){
    max_width = pconcom->win_width - 2;
  }
  return max_width;
}

void ConsoleAutocomplete::set_matches(StringMatch x, bool in_path){
  
  reset();
  
  is_active = true;
  all_matches = x;
  n_matches = x.size();
  is_empty = x.empty();
  n_display = 1;
  this->in_path = in_path;
  
  uint max_width = get_max_width();
  
  if(!is_empty){
    
    n_display = n_matches < max_height ? n_matches : max_height;
    
    const uint n = all_matches.size();
    lines_fmt.resize(n);
    
    // we don't highlight for starting letters because it is trivial
    // we only highlight when non starting matches can be found
    // ie this matching starts at 2+ letters
    const uint n_target = all_matches.get_target_size_wide();
    const bool add_hightlight = n_target >= 2;
    
    // suggestions can have VTS sequences, 
    // if so, we add two spaces before
    // 
    const bool has_vts = all_matches.has_meta("vts");
    vector<string> all_vts = has_vts ?  all_matches.get_meta_vector("vts") : vector<string>();
    
    if(has_vts){
      // we add two spaces in front of the command, in case they are formatted
      max_width -= 2;
    }
    
    uint largest_width = 0;
    for(uint i = 0 ; i < n ; ++i){
      str::string_utf8 tmp = str::shorten(all_matches.string_at(i), max_width);
      if(tmp.size() > largest_width){
        largest_width = tmp.size();
      }
      
      lines_fmt[i] = tmp.str();
    }
    
    bool has_labels = all_matches.has_meta("labels");
    vector<string> all_labels = has_labels ?  all_matches.get_meta_vector("labels") : vector<string>();
    
    if(has_labels){
      uint label_size = str::max_size(all_labels);
      if(label_size + largest_width > max_width){
        // 3/4 sugg, 1/4 label
        uint quarter_width = 0.25 * max_width;
        if(quarter_width > label_size){
          // no need to shorten the labels
          largest_width = max_width - label_size;
          str::shorten_inplace(lines_fmt, largest_width);
          
        } else if(quarter_width <= 4){
          // we drop the labels
          has_labels = false;
          
        } else {
          largest_width = max_width - quarter_width;
          str::shorten_inplace(lines_fmt, largest_width);
          str::shorten_inplace(all_labels, quarter_width);
          
        }
      }
      
      if(has_labels){
        str::right_fill_with_space_inplace(all_labels);
      }
      
    }
    
    // we set them at the same width
    for(uint i = 0 ; i < n ; ++i){
      
      str::string_utf8 tmp = lines_fmt[i];
      
      if(tmp.size() < largest_width){
        const uint origin_size = tmp.size();
        for(uint j = 0 ; j < (largest_width - origin_size) ; ++j){
          tmp.push_back(' ');
        }
      }
      
      // we add highlighting of the match
      if(add_hightlight){
        const str::MatchInfo &match_info = all_matches.match_info_at(i);
        const uint n_words = match_info.size();
        for(int i = n_words - 1 ; i >= 0 ; --i){
          const uint start = match_info.start_at(i);
          const uint end = match_info.end_at(i);
          if(start < tmp.size()){
            // case when we have trimmed the result
            const uint end_fixed = min(end, tmp.size());
            tmp.insert(end_fixed, VTS::BOLD_NOT);
            tmp.insert(start, VTS::BOLD);
          }
        }
        
      }
      
      if(has_labels){
        tmp.push_back(all_labels[i]);
      }
      
      if(has_vts){
        tmp.insert(0, "  " + all_vts[i]);
        tmp.push_back(VTS::RESET_FG_BG);
      }
      
      lines_fmt[i] = tmp.str();
    }
    
  } else {
    // empty
    
    string cause_no_match = all_matches.get_cause_no_match();
    if(cause_no_match.empty()){
      cause_no_match = "no suggestion, sorry";
    }
    
    lines_fmt = str::str_split(cause_no_match, "\n");
    str::shorten_inplace(lines_fmt, get_max_width());
    
    n_display = lines_fmt.size();
    str::right_fill_with_space_inplace(lines_fmt);
  }
  
}

void ConsoleAutocomplete::move(int side){
  
  if(is_empty){
    return;
  }
  
  if(side == SIDE::TOP){
    if(index == 0){
      return;
    }
    
    index = 0;
  } else if(side == SIDE::UP){
    if(index == 0){
      return;
    }
    
    --index;
  } else if(side == SIDE::BOTTOM){
    if(index >= all_matches.size() - 1){
      return;
    }
    
    index = all_matches.size() - 1;
  } else {
    if(index >= all_matches.size() - 1){
      return;
    }
    
    ++index;
  }
  
  display();
}

void ConsoleAutocomplete::reset(){
  // we clear the old AC display
  if(is_active){
    cout << VTS::CURSOR_HIDE;
    for(uint i=0 ; i<n_display ; ++i){
      cout << VTS::CURSOR_DOWN << VTS::CLEAR_LINE;
    }
    cout << VTS::cursor_up(n_display);
    pconcom->print_command();
    cout << VTS::CURSOR_REVEAL;
  }
  
  all_matches = StringMatch{};
  lines_fmt.clear();
  n_matches = 0;
  index = 0;
  screen_start = 0;
  n_display = 0;
}

void ConsoleAutocomplete::clear(){
  reset();
  
  // we refresh the current command
  pconcom->print_command(true);
  
  is_active = false;
}

vector<string> ConsoleAutocomplete::gen_scrollbar(uint screen_start, uint n_display){
  // NOTA: n_display <= max_height is ALWAYS true
  vector<string> res(n_display, "");
  
  if(n_matches <= max_height){
    // we should never end here but just in case
    return res;
  }
  
  /* 
  * Implementation
  * 
  * We take an example, we go through all the cases
  * 
  * max_height = 5
  * n_matches = 9
  * ~~~~~: curent screen
  * X: solid scroll bar
  * -: blank scroll bar (hole)
  * digit: index of the match
  * 
  * 012345678
  * ~~~~~
  * X----
  * 
  * 012345678
  *  ~~~~~
  *  -X---
  * 
  * 012345678
  *   ~~~~~
  *   --X--
  * 
  * 012345678
  *    ~~~~~
  *    ---X-
  * 
  * 012345678
  *     ~~~~~
  *     ----X
  * 
  * Total number of screen_start positions possible:
  * n_ss = n_matches - max_height + 1
  * 
  * Screen start positions, excluding the extremes:
  * n_ss_middle = n_ss - 2
  * 
  * Number of blanks in the scroll bar
  * n_blanks = n_matches - max_height
  * if(n_blanks > max_height - 1) n_blanks = max_height - 1
  * 
  * Number of solid blocks in the scroll bar
  * n_solid = max_height - n_blanks
  * 
  * Number of possible starting positions of the solid block in the scroll bar
  * n_solid_start = n_blanks + 1
  * 
  * Number of possible starting positions of the solid block in the scroll bar, excluding th extremes
  * n_solid_start_middle = n_solid_start - 2
  * 
  * How to get the starting position of the solid block in the scroll bar from the screen start, when not at the extremes?
  * - 1) screen_start: [1, n_ss - 2], or [1, n_ss_middle]
  * - 2) starting positions of the solid block: [1, n_solid_start - 2], or [1, n_solid_start_middle]
  * 
  * We want to map 1) to 2). Let ss be the current screen_start, not at an extreme, then:
  * double x = static_cast<double>(ss) / static_cast<double>(n_ss_middle) * static_cast<double>(n_solid_start_middle)
  * solid_start = ceiling(x)
  * 
  * 
  * */
  
  uint n_blanks = n_matches - max_height;
  if(n_blanks >= max_height){
    n_blanks = max_height - 1;
  }
  
  const uint bar_length = max_height - n_blanks;
  
  const uint n_ss = n_matches - max_height + 1;
  const uint n_ss_middle = n_ss >= 2 ? n_ss - 2 : 0;
  
  // const uint n_solid = max_height - n_blanks;
  const uint n_solid_start = n_blanks + 1;
  const uint n_solid_start_middle = n_solid_start - 2;
  
  //
  // step 1: scroll bar start
  //
  
  uint solid_start = 0;
  if(screen_start == 0){
    solid_start = 0;
  } else if(screen_start + max_height == n_matches){
    solid_start = max_height - bar_length;
  } else {
    
    double x = static_cast<double>(screen_start) / static_cast<double>(n_ss_middle) * static_cast<double>(n_solid_start_middle);
    solid_start = std::ceil(x);
  }
  
  //
  // step 2: printing
  //
  
  const string color_scrollbar = pconcom->opt_color("autocomp_scrollbar");
  const string color_text_bg = pconcom->opt_color("autocomp_text_bg");
  
  uint bar_done = 0;
  for(uint i=0 ; i<n_display ; ++i){
    if(i >= solid_start && bar_done < bar_length){
      ++bar_done;
      res[i] = color_scrollbar + " " + VTS::RESET_FG_BG;
    } else {
      res[i] = color_text_bg + " " + VTS::RESET_FG_BG;
    }
  }
  
  return res;
}

void ConsoleAutocomplete::display(bool init){
  is_active = true;
  
  // we refresh the current command
  pconcom->print_command(true);
  
  // we print the autocomplete box, below the cursor
  
  std::cout << VTS::CURSOR_HIDE;
  
  CursorInfo info(pconcom->handle_out);
  
  // clearing
  if(init){
    uint i = 0;
    while(i < n_display){
      ++i;
      if(i <= info.n_lines_below){
        std::cout << VTS::CURSOR_DOWN << VTS::CLEAR_LINE;
      } else {
        std::cout << endl;
      }
    }
    std::cout << VTS::cursor_up(n_display);
  }
  
  // we place the box at the right location
  const uint ntarget = all_matches.get_target_size_wide();  
  const uint current_prompt_len = pconcom->prompt_size(pconcom->cursor_str_y == 0);
  
  // const uint x_autocomp = pconcom->cursor_str_x - ntarget + current_prompt_len;
  uint x_autocomp = pconcom->cursor_term_x;
  if(ntarget > x_autocomp - current_prompt_len){
    // we are dealing with a long line, target starting on the line above
    x_autocomp = 0;
    
  } else {
    x_autocomp -= ntarget;
    
  }
  
  // now we adjust x_autocomp based on the lenght of the suggestions
  // we need to find the width first
  uint sugg_width = str::size_no_vts(lines_fmt.at(0));
  const uint &w = pconcom->win_width;
  if(x_autocomp + sugg_width > w){
    if(sugg_width + 2 > w){
      util::error_msg("Internal error: AC width larger than the current window size:\n",
                      "sugg_width = ", sugg_width, "\n",
                      "w = ", w);
      str::shorten_inplace(lines_fmt, w / 2);
      x_autocomp = 0;
    } else {
      x_autocomp = w - sugg_width - 2;
    }
  }
  
  const string text_color = pconcom->opt_color("autocomp_text_bg") + pconcom->opt_color("autocomp_text_fg");
  
  //
  // branch 1: no suggestions 
  //
  
  if(is_empty){
    
    for(const auto &cause : lines_fmt){
      std::cout << VTS::cursor_move_at_x(x_autocomp);
      std::cout << VTS::CURSOR_DOWN << text_color << cause << VTS::RESET_FG_BG;
    }
    
    std::cout << VTS::cursor_up(lines_fmt.size());
    std::cout << VTS::cursor_move_at_x(pconcom->cursor_term_x);
    
    std::cout << VTS::CURSOR_REVEAL;
    return;
  }
  
  //
  // branch 2: suggestions 
  //
  
  // we ensure the selected is at the right position (following the moves)
  if(index < screen_start){
    screen_start = index;
  } else if(screen_start + max_height - 1 < index){
    screen_start = index - max_height + 1;
  }
  
  // the selected line cannot be the first/last on screen unless it is the global first/last
  if(n_display < n_matches){
    if(index > 0 && screen_start == index){
      // we place the screen start above
      --screen_start;
    } else if(index < n_matches - 1 && index == (screen_start + max_height - 1)){
      ++screen_start;
    }
  }
  
  vector<string> scrollbar = gen_scrollbar(screen_start, n_display);
  
  const string selection_color = pconcom->opt_color("autocomp_selection_bg") + pconcom->opt_color("autocomp_selection_fg");
  uint j = 0;
  for(uint i=screen_start ; i<(screen_start + n_display) ; ++i){
    std::cout << VTS::cursor_move_at_x(x_autocomp);
    std::cout << VTS::CURSOR_DOWN << (i == index ? selection_color : text_color) << lines_fmt[i] << VTS::RESET_FG_BG << scrollbar[j++] << VTS::CLEAR_RIGHT;
  }
  
  // we reset the cursor
  std::cout << VTS::cursor_up(n_display);
  std::cout << VTS::cursor_move_at_x(pconcom->cursor_term_x);
  
  std::cout << VTS::CURSOR_REVEAL;
  
}

AutocompleteResult ConsoleAutocomplete::get_selection() const {
    
  if(all_matches.empty()){
    return AutocompleteResult(NO_STRING_MATCH);
  }
  
  const str::MetaString user_choice = all_matches.at(index);
  AutocompleteResult res = pconcom->pserver_ac->finalize_autocomplete(user_choice);
  
  return res;
}

