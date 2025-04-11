    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

#include "history.hpp"
#include "console.hpp"


//
// ConsoleCommandSummary -------------------------------------------------------
//


ConsoleCommandSummary::ConsoleCommandSummary(ConsoleCommand *pconcom): type(TYPE::SET) {
  
  // we trim WS for one liners
  all_lines = pconcom->all_lines;
  all_lines_fmt = pconcom->all_lines_fmt;
  
  all_ending_quotes = pconcom->all_ending_quotes;
  cursor_str_x = pconcom->cursor_str_x;
  cursor_str_y = pconcom->cursor_str_y;
  hash = pconcom->hash();
  
  selection = pconcom->selection;
  
  string str;
  const uint n = min(all_lines.size(), 5);
  for(uint i=0 ; i<n ; ++i){
    if(i > 0){
      str += "\\n";
    }
    str += all_lines[i].str();
  }
  
  command_short = str;
}

ConsoleCommandSummary::ConsoleCommandSummary(const vector<string> &x): 
  type(TYPE::SET),
  all_lines(vector<str::string_utf8>()), 
  all_lines_fmt(vector<string>()),
  all_ending_quotes(vector<char>())
  {
  
  uint n = x.size();
  for(auto &xi : x){
    all_lines.push_back(str::string_utf8(xi));
    all_lines_fmt.push_back(UNSET::STRING);
    all_ending_quotes.push_back(NOT_A_QUOTE);
  }
  
  cursor_str_x = all_lines[0].size();
  
  hash = str::hash_string_vector(x);
  
  string str(x[0]);
  n = min(all_lines.size(), 5);
  for(uint i=1 ; i<n ; ++i){
    str += "\\n" + all_lines[i].str();
  }
  
  command_short = str;
}


//
// ConsoleHistory --------------------------------------------------------------
//


#define CONSOLE_HISTORY 0

fs::path ConsoleHistory::hist_path = UNSET::PATH;

ConsoleHistory::ConsoleHistory(ConsoleCommand *pcon, const string program_name){
  
  pconcom = pcon;
  
  if(util::is_unset(program_name)){
    return;
  }
  
  // we try to load the history from the disk
  hist_path = get_path_to_program_history(program_name);
  if(!fs::exists(hist_path)){
    // nothing
    return;
  }
  
  // we read the file
  std::ifstream hist_file(hist_path);
  if(!hist_file.is_open()){
    std::cerr << "Could not read the existing history located at:\n'" << hist_path << "'\n";
    return;
  }
  
  //
  // step 1: read all the lines 
  //
  
  std::string line;
  std::vector<std::string> all_cmd;
  vector<ConsoleCommandSummary> all_past_commands;
  vector<size_t> all_hash;
  while(std::getline(hist_file, line)){
    if(line.empty()){
      // nothing
    } else if(line.back() == '\\'){
      line.pop_back();
      all_cmd.push_back(line);
      
    } else {
      all_cmd.push_back(line);
      ConsoleCommandSummary cmd_sum(all_cmd);
      
      all_hash.push_back(cmd_sum.hash);
      
      all_past_commands.push_back(cmd_sum);
      all_cmd.clear();
    }
  }
  
  if(all_past_commands.empty()){
    hist_file.close();
    return;
  }
  
  //
  // step 2: remove duplicates
  //
  
  // note that the first values of the vector past_commands are in
  // fact the oldest ones => we respect that here (we start from the end)
  // 
  const uint MAX_HIST_ENTRIES = 80;
  IndexInfo index = to_index(all_hash, to_indexOpts().reverse());
  
  const vector<size_t> first_obs = index.get_first_obs();
  const uint start = min(first_obs.size() - 1, MAX_HIST_ENTRIES - 1);
  uint j = 0;
  for(int i=start ; i>=0 ; --i){
    ConsoleCommandSummary &cmd_sum = all_past_commands[first_obs[i]];
    past_commands.push_back(cmd_sum);
    cmd_index[cmd_sum.hash] = j++;
  }
  
  hist_file.close();
  
  //
  // step 3: rewrite the history 
  //
  
  write_history();
  
}

void ConsoleHistory::write_history(){
  
  if(util::is_unset(hist_path)){
    return;
  }
  
  std::ofstream hist_file_out(hist_path);
  if(!hist_file_out.is_open()){
    std::cerr << "Could not wite into the existing history located at:\n'" << hist_path << "'\n";
    return;
  }
  
  const uint n = past_commands.size();
  for(uint i=0 ; i<n ; ++i){
    vector<str::string_utf8> cmd_utf8(past_commands[i].all_lines);
    const uint nlines = cmd_utf8.size();
    for(uint j=0 ; j+1<nlines ; ++j){
      hist_file_out << cmd_utf8[j].str() << "\\\n";
    }
    hist_file_out << cmd_utf8[nlines - 1].str() << "\n";
  }
  hist_file_out << "\n";
  
  hist_file_out.close();
}

void ConsoleHistory::append_history_line(){
  
  if(!is_main()){
    return;
  }
  
  if(util::is_unset(hist_path)){
    return;
  }
  
  if(past_commands.empty()){
    return;
  }
  
  vector<str::string_utf8> cmd_utf8(past_commands.back().all_lines);
  const uint nlines = cmd_utf8.size();
  if(nlines > 8){
    // there's no point in saving that kind of stuff in a history, 
    // => this is just polllution
    return;
  }
  
  if(util::vector_contains(ignored_cmd, cmd_utf8.at(0).str())){
    return;
  }
  
  std::ofstream hist_file_out(hist_path, std::ios::app);
  if(!hist_file_out.is_open()){
    std::cerr << "Could not write into the existing history located at:\n'" << hist_path << "'\n";
    return;
  }
  
  for(uint j=0 ; j+1<nlines ; ++j){
    hist_file_out << cmd_utf8[j].str() << "\\\n";
  }
  hist_file_out << cmd_utf8[nlines - 1].str() << "\n";
  
  hist_file_out.close();
}

void ConsoleHistory::navigate(int direction, bool &any_update, bool any_action){
  
  bool is_up = direction == SIDE::UP;
  bool is_down = !is_up;
  if(is_up){
    if(past_commands.empty() && tmp_commands.empty()){
      // nothing because there is no history
      return;
    }
  } else {
    if(index == 0){
      // we're already at the bottom
      return;
    }
  }
  
  
  //
  // step 1: saving the current command if needed
  //
  
  if(is_fresh){
    // we save the current line, that is, we initialize lookup so that lookup[0].idx leads to stg OK
    
    if(tmp_commands.empty()){
      tmp_commands.emplace_back(pconcom);
    } else {
      // case when a previous command was discarded and saved to a tmp history
      // so that it can be retrieved if needed
      tmp_commands.insert(tmp_commands.begin(), pconcom);
      // cout << "fresh hist\ntmp_commands.size = " << tmp_commands.size() << "\n";
      // cout << "lookup size = " << lookup.size() << "\n";
    }
    
    is_fresh = false;
  } else {
    // we save the line **if it was modified**
    if(any_action && pconcom->hash() != current_cmd.hash){
      if(lookup[index].is_tmp){
        // we are already in a tmp => we replace
        const uint &idx = lookup[index].idx;
        tmp_commands[idx] = pconcom;
      } else {
        // we're not in a tmp, we need to create a tmp
        tmp_commands.emplace_back(pconcom);
        lookup.insert(lookup.begin() + index, HistoryLookup(true, tmp_commands.size() - 1));
        if(is_up){
          ++index;
          // before insertion
          // a b c d
          //       ^ here
          // after insertion
          // a b c e d
          //       ^ here
          // we need to move up so that 'index + 1' refers to the first after 'd'
          // note that if we move down this is not the case becase 'index - 1' refers to 'c'
        }
      }
    }
  }
  
  
  //
  // step 2: moving 
  //
  
  if(is_down){
    // this is simple, we just navigate down the lookup table that was already set
    // cout << "down in lookup table\n";
    --index;
    // cout << "index = " << index << "\n";
    HistoryLookup &info = lookup[index];
    current_cmd = info.is_tmp ? tmp_commands[info.idx] : past_commands[info.idx];
    
  } else {
    // we need to update the lookup table
    if(index + 1 < lookup.size()){
      // cout << "fetch in lookup table" << endl;
      // we pick the command in the lookup table
      ++index;
      HistoryLookup &info = lookup[index];
      current_cmd = info.is_tmp ? tmp_commands[info.idx] : past_commands[info.idx];
      
    } else if(!past_commands.empty()){
      const uint hist_size = past_commands.size();
      if(util::is_unset(index_hist_max) || index_hist_max + 1 < hist_size){
        // we fetch the command in the history
        // we ignore commands that cannot fit the console height
        // cout << "fetch in history" << endl;
        
        bool reasonable_height = false;
        
        while(!reasonable_height){
          if(util::is_unset(index_hist_max)){
            index_hist_max = 0;
          } else {
            ++index_hist_max;
          }
          
          if(index_hist_max == hist_size){
            // we return
            return;
          }
          
          // cout << "index to fetch the history = " << hist_size - index_hist_max - 1 << endl;
          // cout << "hist_size = " <<  hist_size << ", index_hist_max = " << index_hist_max << endl;
          
          current_cmd = past_commands[hist_size - index_hist_max - 1];
          if(current_cmd.all_lines.size() + 1 < pconcom->win_height){
            reasonable_height = true;
          }
        }
        
        ++index;
        lookup.push_back(HistoryLookup(false, hist_size - index_hist_max - 1));
        
        // cout << "command fetched = '" << current_cmd.collect() << "'";
        
      } else {
        // nothing: we're at the top
        return;
      }
    }
  }
  
  
  // we start at y = 0 => because it is not possible to reposition the cursor to lines below reliably
  if(current_cmd.cursor_str_y > 0){
    // if we move the cursor vertically, we reset the x cursor
    current_cmd.cursor_str_y = 0;
    current_cmd.cursor_str_x = 0;
  }
  
  // we remove the selection
  current_cmd.selection = false;
  
  pconcom->copy_cmd(current_cmd);
  pconcom->pcmdstate->clear(current_cmd);
  
  any_update = true;
  
}


void ConsoleHistory::add_command(bool is_tmp){
  // past_commands: 
  // - list of past commands. 
  // - the incoming command ALWAYS ends up last even if it already exists
  // - the bookeeping is performed with cmd_index which stores the hash
  //   of all past commands (cmd_index and past_commands have the same length)
  //
  
  uint hist_size = past_commands.size();
  
  // does the command exist already?
  // NOTA: we trim WS for one liners
  string full_command = pconcom->all_lines.size() > 1 ? pconcom->collect() : str::trim_WS(pconcom->collect());
  
  if(str::no_nonspace_char(full_command)){
    // we don't save empty lines
    return;
  }
  
  // we reset the command cursors and save the command
  ConsoleCommandSummary cmd_sum(pconcom);
  // we place the cursor always last
  cmd_sum.cursor_str_x = cmd_sum.all_lines[0].size();
  cmd_sum.cursor_str_y = 0;
  
  index = 0;
  index_hist_max = UNSET::UINT;
  
  is_fresh = true;
  tmp_commands.clear();
  lookup = vector<HistoryLookup>{HistoryLookup()};
  
  if(is_tmp){
    // we don't save to the past commands
    tmp_commands.push_back(cmd_sum);
    lookup.push_back(HistoryLookup(true, 1));
    
  } else {
    // we remove past entries and save to past commands
    
    uint64_t cmd_hash = str::hash_string(full_command);
    auto it = cmd_index.find(cmd_hash);
    if(it == cmd_index.end()){
      // does not exist
      cmd_index[cmd_hash] = hist_size;
      ++hist_size;
    } else {
      // exist already: we clean it up
      uint i = cmd_index[cmd_hash];
      past_commands.erase(past_commands.begin() + i);
      cmd_index[cmd_hash] = hist_size - 1;
      // we update the index of posterior values
      for(auto &x: cmd_index){
        if(x.second > i){
          x.second -= 1;
        }
      }
    }
    
    past_commands.push_back(cmd_sum);
  }
  
  
}



//
// CommandState ---------------------------------------------------------------- 
//


void CommandState::add_state(){
  // we clear the commands 
  
  if(!record_state){
    return;
  }
  
  time_t t_now = util::now();
  
  ConsoleCommandSummary cmd(pconcom);
  
  if(n_undo > 0){
    // we clear the saved states past the undo index
    all_states.erase(all_states.end() - n_undo);
    all_states.push_back(cmd);
    n_undo = 0;
    
  } else {
    
    ConsoleCommandSummary &last_cmd = all_states.back();
    if(cmd.current_line() == last_cmd.current_line()){
      // cursor move
      last_cmd = cmd;
      
    } else if(util::timediff_ms(t_now, time_last_add) < 150){
      // consecutive letters are added "at once"
      last_cmd = cmd;
      
    } else {
      all_states.push_back(cmd);
      
    }
    
  }
  
  time_last_add = t_now;
  
}


void CommandState::set_state(const ConsoleCommandSummary &cmd){
  
  const uint y_origin = pconcom->cursor_str_y;
  const uint y_destination = cmd.cursor_str_y;
  const bool print_all_lines = y_origin != y_destination;
  
  if(print_all_lines){
    pconcom->clear_display_all_lines();
  }
  
  pconcom->copy_cmd(cmd);
  
  record_state = false;
  
  if(print_all_lines){
    
    if(y_destination > y_origin){
      std::cout << VTS::cursor_down(y_destination - y_origin);
    } else {
      std::cout << VTS::cursor_up(y_origin - y_destination);
    }
    
    pconcom->print_command(true);
  } else {
    pconcom->print_command();
  }
  
  record_state = true;
}


void CommandState::undo(){
  
  pconcom->quit_autocomp();
  
  const int n = all_states.size();
  
  if(n_undo >= n - 1){
    // nothing 
    return;
  }
  
  ++n_undo;
  const ConsoleCommandSummary &cmd = all_states[n - 1 - n_undo];
  
  set_state(cmd);
  
}

void CommandState::redo(){
  
  pconcom->quit_autocomp();
  
  const int n = all_states.size();
  
  if(n_undo <= 0){
    // nothing 
    return;
  }
  
  --n_undo;
  const ConsoleCommandSummary &cmd = all_states[n - 1 - n_undo];
  
  set_state(cmd);
  
}

void CommandState::clear(){
  
  all_states.clear();
  all_states.push_back(empty_cmd);
  time_last_add = time_t();
  n_undo = 0;
  
}


void CommandState::clear(const ConsoleCommandSummary &cmd){
  
  all_states.clear();
  all_states.push_back(cmd);
  time_last_add = time_t();
  n_undo = 0;
  
}
