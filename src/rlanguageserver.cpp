    //=========================================================================//
   //            Author: Laurent R. Bergé, University of Bordeaux             //
  //             Copyright (C) 2025-present, Laurent R. Bergé                //
 //              MIT License (see project_root/LICENSE)                     //
//=========================================================================//

/*
* - [ ] automalically find the locations of R
* - [ ] check the DLL and the R version match
* - [ ] add a proper CmdParsed, with indent
* - [ ] run: avoid long jump
* - [ ] run: disable custom error handlers
* - [ ] catch the errors when running
* - [ ] implement autocomplete
* - [ ] on exit: offer different implementations
* - [ ] add options
* - [ ] add the timing between two runs (it should be here and not in console ???? or not?)
* - [ ] add when running for more than 1s:
*       running... elapsed 00:00s
*       Emit sound when over? Type bip
* => the color of the bip letters highlight when the user writes bip
* - [ ] autocompletes gets out only when we change the type of completion
*       base$peta| => suggest Petal.Length
*       base$p| => still suggest while deleting
*       base$| => still same suggests
*       base| => now we stop because we change the type of completion
*       => we should also stop when adding non-word non-space characters
*       
* - [ ] allow less spelling mistakes when the autocomplete is looser => 
*       too many false positive, not like a path where we know the answer is there
*       => avoir plus de controle la dessus
*       => de même pour rechercher les strings au sein des mots et pas au depart
* - [ ] dispatch S3 methods to the right function using the first argument
* - [ ] in string_match: create argument MatchingOptions
* - [ ] add method void StringMatch::add(StringMatch) to add matches together, these matches
*       may have used different options
* - [ ] cache costly autocomlete choices (available packages/functions)
* */

/* AUTOCOMPLETE
* 
* - [ ] order functions
* - [ ] cache functions
* - [ ] ??? remove S3 functions ???
* - [ ] cache packages in memory
* - [ ] cache packages on disk
* - [ ] implement string
* - [ ] implement string interpolation
* - [ ] implement clean_suggestions()
* - [ ] implement update_suggestions(char c)
* - [ ] NSE for given function arguments based on an argument's value
* - [ ] implement var:| => leads to values from this vector. IF var is a VECTOR
* 
* */


#include "rlanguageserver.hpp"

namespace fs = std::filesystem;

RLanguageServer *prlgsrv;

// we need to cache the command
static string current_cmd;

// To catch interrupts
bool R_is_running = false;
bool t_interrupt_running = false;
void check_interrupt(ConsoleCommand *pconcom){
  t_interrupt_running = true;
  while(R_is_running){
    wchar_t unicode = 0;
    pconcom->read_line(ReadOptions().raw_key(&unicode).no_block());
    
    if(unicode == KEYS::ESC){
      *R::pUserBreak = 1;
      R_is_running = false;
      pconcom->insert_newline_if_needed_to_be_leftmost();
      std::cout << VTS::FG_BRIGHT_RED << "#> User interrupt" << VTS::RESET_FG_BG;
      break;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  t_interrupt_running = false;
}

void input_hook(){
  if(R::GA_peekevent()){
    R::R_ProcessEvents();
  }
}


static bool browser_ls_sent = false;
static bool command_just_sent = false;
static bool do_print = true;

static bool stack_startup_msg = true;
static vector<string> startup_msg;
static vector<int> startup_msg_type;

static bool do_pretty_int = false;
static vector<string> pretty_ints;


inline string add_commas(const string &x){
  // NOTA: the ints we deal with here start with a space
  // we don't add a comma to 1234
  
  const int n = x.size();
  int i = 0;
  str::move_i_to_non_WS_if_i_WS(x, i, SIDE::RIGHT);
  int n_digits = n - i;
  
  if(n_digits <= 4){
    return x;
  }
  
  string res = x;
  int current_pos = 3;
  while(n_digits - current_pos > 0){
    res.insert(res.begin() + n - current_pos, ',');
    current_pos += 3;
  }
  
  return res;
}

void flush_pretty_ints(){
  
  if(pretty_ints.empty()){
    return;
  }
  
  if(do_pretty_int){
    
    prlgsrv->concom.write_output("[1]", 0);
    
    for(size_t i = 1 ; i < pretty_ints.size() ; ++i){
      prlgsrv->concom.write_output(add_commas(pretty_ints[i]), 0);
    }
    
  } else {
    for(const auto &s : pretty_ints){
      prlgsrv->concom.write_output(s, 0);
    }
  }
  
  pretty_ints.clear();
}

inline bool str_is_int(const char *buf, int len){
  // a simple vector of int is:
  // [1] 1234 5678
  //    ^^^^^
  // R sends each element underlined in turn
  // 
  
  if(len < 2 || buf[0] != ' '){
    return false;
  }
  
  int i = 1;
  while(i < len && buf[i] == ' '){
    ++i;
  }
  
  if(i == len || !str::is_digit(buf[i])){
    return false;
  }
  
  while(i < len && str::is_digit(buf[i])){
    ++i;
  }
  
  return i == len;
}

void cb_write_console(const char *buf, int len, int otype){
  
  if(stack_startup_msg){
    string msg{buf};
    if(str::starts_with(msg, "During startup")){
      do_print = false;
      return;
    }
    
    if(do_print){
      startup_msg.push_back(msg);
      startup_msg_type.push_back(otype);
    }
    
    return;
  }
  
  if(do_print){
    
    if(do_pretty_int && otype != 0){
      do_pretty_int = false;
    }
    
    if(do_pretty_int){
      
      if(command_just_sent){
        
        command_just_sent = false;
        
        if(len == 3 && buf[0] == '[' && buf[1] == '1' && buf[2] == ']'){
          pretty_ints.push_back(buf);
          return;
          
        }
        
        do_pretty_int = false;
        
      } else {
        
        if(pretty_ints.back()[0] ==  '\n'){
          // we're past the end => no pretty int
          
        } else if(len == 1 && buf[0] == '\n'){
          // last item
          pretty_ints.push_back(buf);
          return;
          
        } else if(str_is_int(buf, len)){
          if(pretty_ints.size() < 4){
            pretty_ints.push_back(buf);
            return;
          }
        }
        
        do_pretty_int = false;
        flush_pretty_ints();
      }
      
    }
    
    do_pretty_int = false;
    prlgsrv->concom.write_output(buf, otype);
    
  }
}

void print_startup_msg(){
  
  if(!startup_msg.empty()){
    cout << "\n";
  }
  
  for(size_t i = 0 ; i < startup_msg.size() ; ++i){
    if(startup_msg_type[i] == 1){
      cout << VTS::FG_BRIGHT_RED << startup_msg[i] << VTS::FG_DEFAULT;
    } else {
      cout << startup_msg[i];
    }
  }
}


int cb_read_console(const char *prompt, unsigned char *buf, int len_in, int addtohistory){
  
  R_is_running = false;
  ConsoleCommand *pconcom = &(prlgsrv->concom);
  const bool is_new_call = current_cmd.empty();
  
  flush_pretty_ints();
  
  //
  // step 1: receiving the command from the user 
  //
  
  bool is_in_browser = false;
  
  if(is_new_call){
    bool is_command = addtohistory > 0;
    string hist_name = prompt;
    
    if(is_command){
      is_in_browser = hist_name.size() > 9 && hist_name.substr(0, 6) == "Browse";
      if(is_in_browser){
        // we get the function name
        const char *str = R::R_run("as.character(sys.call()[[1]])[1]");
        hist_name = str;
        
        // we save the variables for the AC
        
        if(!browser_ls_sent){
          const string code_to_run = "options(sircon_browser_ls = base::ls(all.names = TRUE))";
          const size_t n = code_to_run.size();
          for(size_t i=0 ; i<n ; ++i){
            buf[i] = code_to_run[i];
          }
          buf[n] = '\n';
          buf[n + 1] = '\0';
          browser_ls_sent = true;
          return 1;
        }
        
        R::R_run("options(sircon_is_in_browser = TRUE)");
        
      } else {
        // default history name
        hist_name = "main";
        R::R_run("options(sircon_is_in_browser = FALSE)");
      }
    }
    
    // we find out if there was an error in the previous command
    bool was_error = R::R_run("getOption(\"sircon_is_error\", default = 0L)");
    
    // NOTA: R_process_event is run while reading input in the main thread
    CommandToEvaluate full_cmd = prlgsrv->concom.read_command(is_command, hist_name, prompt, was_error);
    
    current_cmd = full_cmd.cmd;
    if(is_in_browser && str::trim_WS(current_cmd) == "q()"){
      // we correct a common mistake within browser
      current_cmd = "Q";
    }
    
    if(is_command){
      browser_ls_sent = false;
      R::R_run("options(sircon_is_error = 1L)");
      if(!full_cmd.is_parse_error){
        current_cmd = current_cmd + " \n options(sircon_is_error = 0L)";
      }
    }
  }
  
  //
  // step 2: sending the command to R 
  //
  
  // I need to do that properly, with buffering
  const size_t n = current_cmd.size();
  size_t len = len_in;
  if(n + 1 < len){
    for(size_t i=0 ; i<n ; ++i){
      buf[i] = current_cmd[i];
    }
    buf[n] = '\n';
    buf[n + 1] = '\0';
    
    current_cmd.clear();
  } else {
    
    // we do not cut in the middle of a multibyte character
    uint new_len = len - 10;
    while(new_len + 1 < len && !stringtools::utf8::is_starting_byte(current_cmd[new_len])){
      ++new_len;
    }
    // out: we're at a starting byte, so new_len - 1 is the end of a (possibly) MB char
    
    for(size_t i=0 ; i<new_len ; ++i){
      buf[i] = current_cmd[i];
    }
    buf[new_len] = '\0';
    
    auto start = current_cmd.begin();
    current_cmd.erase(start, start + new_len - 1);
  }
  
  //
  // step 3: waiting functions 
  //
  
  R_is_running = true;
  if(!t_interrupt_running){
    std::thread t_interrupt(check_interrupt, pconcom);
    t_interrupt.detach();
  }
  
  command_just_sent = true;
  do_pretty_int = prlgsrv->concom.get_program_option("pretty_int").get_logical();
  
  return 1;
}

void cb_callback(void){
  /* called during i/o, eval, graphics in ProcessEvents */
  // cout << "[callback]";
}

void cb_busy_cursor(int which){
  /* set a busy cursor ... if which = 1, unset if which = 0 */
}

void cb_show_msg(const char *buf){
  cout << "[ShowMessage]";
}

int cb_ask_yes_no(const char *buf){
  //    yes:  1
  //     no: -1
  // cancel:  0
  
  // cout << "[YesNoCancel]";
  
  ReadOptions opts;
  opts.choices({"y", "n", "c"}).n_char(1).to_lower();
  
  string res = prlgsrv->concom.read_line(opts);
  if(res == "y"){
    return 1;
  } else if(res == "n"){
    return -1;
  }
  
  return 0;
}



void load_functions_dll(std::map<string, HMODULE> dll_handles){
  
  HMODULE &lib_R = dll_handles["R"];
  HMODULE &lib_GA = dll_handles["Rgraphapp"];
  #define LOAD_FUNCTION(name) \
    R::name = extract_dll_function< decltype(R::name) >(lib_R, #name);
  
  LOAD_FUNCTION(Rf_endEmbeddedR)
  LOAD_FUNCTION(R_set_command_line_arguments)
  LOAD_FUNCTION(R_setStartTime)
  LOAD_FUNCTION(R_DefParams)
  LOAD_FUNCTION(R_SetParams)
  LOAD_FUNCTION(readconsolecfg)
  LOAD_FUNCTION(setup_Rmainloop)
  LOAD_FUNCTION(R_ParseVector)
  LOAD_FUNCTION(R_tryEval)
  LOAD_FUNCTION(Rf_protect)
  LOAD_FUNCTION(Rf_unprotect)
  LOAD_FUNCTION(Rf_ScalarString)
  LOAD_FUNCTION(Rf_ScalarInteger)
  LOAD_FUNCTION(Rf_ScalarLogical)
  LOAD_FUNCTION(Rf_ScalarReal)
  LOAD_FUNCTION(Rf_mkCharCE)
  LOAD_FUNCTION(Rf_mkString)
  LOAD_FUNCTION(VECTOR_ELT)
  LOAD_FUNCTION(Rf_length)
  LOAD_FUNCTION(Rf_PrintValue)
  LOAD_FUNCTION(run_Rmainloop)
  LOAD_FUNCTION(R_ProcessEvents)
  // LOAD_FUNCTION(R_PolledEvents)
  
  LOAD_FUNCTION(Rf_translateCharUTF8)
  LOAD_FUNCTION(STRING_ELT)
  
  LOAD_FUNCTION(INTEGER)
  LOAD_FUNCTION(REAL)
  
  //
  // GraphApp 
  //
  
  R::GA_initapp = extract_dll_function<decltype(R::GA_initapp)>(lib_GA, "GA_initapp");
  R::GA_peekevent = extract_dll_function<decltype(R::GA_peekevent)>(lib_GA, "GA_peekevent");
  
}


void load_variable_dll(HMODULE lib_R){
  
  #define LOAD_VARIABLE(name) \
    R::name = extract_dll_variable< decltype(R::name) >(lib_R, #name);
  
  LOAD_VARIABLE(R_NilValue)
  LOAD_VARIABLE(R_GlobalEnv)
}

//
// special functions -----------------------------------------------------------
//


void sf_pretty_int(ConsoleCommand *pconcom, const vector<ParsedArg> &all_args){
  
  const ParsedArg arg = all_args.at(0);
  
  bool is_pretty_int = arg.get_logical();
  if(arg.is_default()){
    // we switch the current values
    is_pretty_int = pconcom->get_program_option("pretty_int").get_logical();
    is_pretty_int = !is_pretty_int;
  }
  
  pconcom->set_program_option("pretty_int", is_pretty_int);
  
}

//
// options --------------------------------------------------------------------- 
//

void hook_set_prompt_in_R(ConsoleCommand *pconcom, LanguageServer *plang, ParsedArg *x){
  
  RLanguageServer *prlang = dynamic_cast<RLanguageServer*>(plang);
  if(prlang->init_ok){
    // only when r is running
    R::R_run("options(prompt = " + str::dquote(x->get_string()) + ")");
    
    pconcom->set_command_to_send("# setting the new prompt");
  }
  
}


//
// RLanguageServer -------------------------------------------------------------
//

ConsoleCommand RLanguageServer::concom = ConsoleCommand();

RLanguageServer::RLanguageServer(int argc, char **argv){
  this->argc = argc;
  this->argv = argv;
  
  prlgsrv = this;
  
  // options
  const options_fmt_t all_options_fmt = {
    {"R_path", argtype::PATH().path_must_exist().path_must_end_with_filename("R.exe")},
    {"freeform", 
     argtype::PATH()
     .path_must_exist()
     .path_must_end_with_filename("R.exe")
     .freeform_key_must_start_with("R")},
     {"pretty_int", argtype::LOGICAL("true")},
     // prompt
     {"prompt.color",    argtype::COLOR("hot_pink")},
     {"prompt.main",     argtype::STRING("> ").set_hook(&concom, this, hook_set_prompt_in_R)},
     {"prompt.continue", argtype::STRING("+ ")},
     // shortcuts
     {"shortcut.enter",     argtype::SHORTCUT("<if: line_matches: \"{_cursor_}\"> <newline> <newline> <delete: line_before> <move_y: up> <move_x: rightmost> <endif>")},
     {"shortcut.alt+enter", argtype::SHORTCUT("<if: empty> <move_y: up> <endif> <move_y: bottom> <move_x: rightmost> <insert: \" |>\"> <newline>")},
  };
  
  // special functions
  std::map<string, SpecialFunctionInfo> all_special_funs = {
    {"pretty_int", SpecialFunctionInfo(&concom, sf_pretty_int, {argtype::LOGICAL("true")})},
  };
  
  concom.initialize("sircon", ConsoleCommand::opts().set_options_format(all_options_fmt));
  
  concom.setup_lgsrv(this);
  concom.setup_Run_While_Reading(&input_hook);
  concom.setup_language_keywords({"TRUE", "FALSE"});
  concom.setup_language_controls({"if", "else", "while", "for", "next", "continue", "in"});
  concom.setup_ignored_hist_cmd({"q()"});
  concom.setup_special_functions(all_special_funs);
  concom.setup_inline_comment("#");
  
  std::shared_ptr<RAutocomplete> pautocomp = std::make_shared<RAutocomplete>();
  concom.setup_srvautocomp(pautocomp);
  
}

void RLanguageServer::init_R(){
  
  init_ok = false;
  
  //
  // step 0: finding R's path 
  //
  
  ParsedArg Rexe_opt = concom.get_program_option("R_path");
  if(Rexe_opt.is_unset()){
    // we try to find out the path
    string default_path = run_shell_command("where.exe", "R.exe");
    
    string err;
    Rexe_opt = ParsedArg(
      default_path, 
      argtype::PATH().path_must_exist().path_must_end_with_filename("R.exe"),
      util::DoCheck(err)
    );
    
    if(Rexe_opt.is_unset()){
      
      util::error_msg(
        "The default path to the R executable could not be found automatically. ",
        "Is R.exe on the PATH?\n",
        "\nIn any case, you can set the path to the executable in sircon's options:",
        "\nIn the file located at: ", str::dquote(concom.get_path_options()), "\n",
        "add the line:\n",
        "R_path = PATH/TO/R/BINARY"
      );
      return;
    }
    
    util::info_msg("R.exe found at: ", Rexe_opt.get_path(), 
                   "\nWe're setting it as your default R version.\n",
                   "\n",
                   "If this is not your favourite version, you can give a path to another binary.",
                   "\nHow to? In the global option file located at: \n",
                   str::dquote(concom.get_path_options()), "\n",
                   "edit the line:\n",
                   "R_path = PATH/TO/YOUR/FAVOURITE/R/BINARY");
    
    concom.set_program_option("R_path", Rexe_opt.get_path().string(), 
                              ConsoleCommand::op_write_t::GLOBAL);
    
  }
  
  fs::path path_Rexe = Rexe_opt.get_path();
  fs::path path_Rbin = path_Rexe.parent_path();
  
  if(!fs::exists(path_Rbin / "R.dll")){
    if(fs::exists(path_Rbin / "x64" / "R.dll")){
      path_Rbin = path_Rbin / "x64";
    } else {
      util::msg("R's binary folder containing the DLLs could not be located.",
                "It should be located at ", path_Rbin, ", or ", path_Rbin / "x64", 
                " but it does not seem to be there. Could you report to sircon's maintainer?");
      return;
    }
  }
  
  fs::path path_Rhome = path_Rbin.parent_path();
  
  if(!fs::exists(path_Rhome / "bin")){
    if(fs::exists(path_Rhome.parent_path() / "bin")){
      path_Rhome = path_Rhome.parent_path();
    } else {
      util::msg("R's home folder could not be located.",
                "It should be located at ", path_Rhome, ", or ", path_Rhome.parent_path(), 
                " but it does not seem to be there. Could you report to sircon's maintainer?");
      return;
    }
  }
  
  // example Rbin: "C:\\Users\\lrberge\\APPS\\R-4.4.1\\bin\\x64\\"
  
  //
  // step 1: loading the DLL
  //
  
  vector<string> all_dlls = {"R.dll", "Rgraphapp.dll", "Rblas.dll", "Riconv.dll", "Rlapack.dll"};
  std::map<string, HMODULE> dll_handles;
  // cout << "Loading the DLL:\n";
  for(auto &dll : all_dlls){
    string dll_path = (path_Rbin / dll).string();
    
    // we load the R dll
    // the options are necessary
    HMODULE lib_handle = LoadLibraryExA(dll_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    
    if(lib_handle == NULL){
      cout << "error when loading the DLL '" << dll_path << "'\n";
      throw std::system_error(GetLastError(), std::system_category(), "");
    } else {
      // cout << " - " << dll << " loaded\n";
    }
    
    string key = dll.substr(0, dll.size() - 4);
    dll_handles[key] = lib_handle;
  }
  
  HMODULE &lib_R = dll_handles["R"];
  
  load_functions_dll(dll_handles);
  
  //
  // step 2: startup options 
  //
  
  // No signal handlers => we do differently
  // cout << "Setting signal handler\n";
  R::pR_SignalHandlers = extract_dll_pointer<int>(lib_R, "R_SignalHandlers");
  *R::pR_SignalHandlers = 0;
  
  // cout << "Setting up R-init:\n";
  R::structRstart rstart;
  R::Rstart prstart = &rstart;
  memset(&rstart, 0, sizeof(rstart));
  
  string *pRhome = new string(path_Rhome.string());
  char *RHome = const_cast<char*>(pRhome->c_str());
  const char *pEnvUser = getenv("USERPROFILE");
  string *p_path_r_user;
  if(pEnvUser){
    p_path_r_user = new string(string(pEnvUser) + "/Documents");
  } else {
    exit_R();
    return;
  }
  char *RUser = const_cast<char*>(p_path_r_user->c_str());
  
  // needed (sets a global variable)
  // cout << "- R_setStartTime\n";
  R::R_setStartTime();
  
  // initializes prstart => we try R_DefParamsEx
  R::R_DefParamsEx = extract_dll_function< decltype(R::R_DefParamsEx) >(lib_R, "R_DefParamsEx", false);
  if(R::R_DefParamsEx){
    R::R_DefParamsEx(prstart, 1);
  } else {
    R::R_DefParams(prstart);
  }
  
  //
  // startup params
  //
  
  // paths
  prstart->rhome = RHome;
  prstart->home = RUser;
  
  // R startup message
  prstart->R_Quiet = R::TRUE;
  prstart->R_NoEcho = R::FALSE;
  prstart->R_Interactive = R::TRUE;
  prstart->R_Verbose = R::FALSE;
  
  // files loaded at startup
  prstart->LoadSiteFile = is_vanilla ? R::FALSE : R::TRUE;     // global
  prstart->LoadInitFile = is_vanilla ? R::FALSE : R::TRUE;     // Rprofile
  prstart->DebugInitFile = R::FALSE;
  
  // text
  prstart->CharacterMode = R::RGui;
  prstart->EmitEmbeddedUTF8 = R::FALSE;
  
  // hooks
  prstart->ReadConsole = cb_read_console;
  prstart->WriteConsole = NULL; 
  prstart->WriteConsoleEx = cb_write_console;
  prstart->CallBack = cb_callback;
  prstart->ShowMessage = cb_show_msg;
  prstart->YesNoCancel = cb_ask_yes_no;
  prstart->Busy = cb_busy_cursor;

  // other parameters
  prstart->RestoreAction = R::SA_NORESTORE;
  prstart->SaveAction = R::SA_NOSAVE;
  
  // we MUST run it
  // cout << "- R_SetParams\n";
  R::R_SetParams(prstart);
  
  // set command line arguments
  // cout << "R_set_command_line_arguments\n";
  // const char *argv[] = {"sircon", "--interactive"};
  // int argc = sizeof(argv) / sizeof(char *);
  // cout << "argc = " << argc;
  // argc = sizeof(argv) / sizeof(argv[0]);
  // cout << ", new argc = " << argc << endl;
  
  // R_set_command_line_arguments(argc, const_cast<char**>(argv));
  R::R_set_command_line_arguments(argc, argv);

  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
  
  // don't know what that does
  // cout << "- GA_initapp\n";
  R::GA_initapp(0, 0);
  
  // needed, it sets all kind of stuff
  // cout << "- readconsolecfg\n";
  R::readconsolecfg();
  
  // we need this to avoid having the default code page
  // this will trigger a warning if the system CP is not UTF8
  // => we hide this warning
  // 
  _putenv("LANG=en_US.UTF-8");
  _putenv("LC_ALL=en_US.UTF-8");
  
  // as in RStudio: we set if to LinkDLL, then back to RGui
  R::pCharacterMode = extract_dll_pointer<int>(lib_R, "CharacterMode");
  *R::pCharacterMode = R::LinkDLL;
  
  // absolutely needed, kind of the thing that launches R
  // cout << "- setup_Rmainloop\n";
  stack_startup_msg = true;
  R::setup_Rmainloop();
  stack_startup_msg = false;
  do_print = true;
  
  //
  // step 3: variables 
  //
  
  // we must load the variables at the end
  
  // loading a few variables (notably the global_env)
  load_variable_dll(lib_R);
  
  // we load a few pointers
  R::pUserBreak = extract_dll_pointer<int>(lib_R, "UserBreak");
  
  // cout << "=> initialization successfull\n";
  
  // welcome message
  R::R_run("cat(\"This is Sircon with \", \"R \", R.version$major, \".\", R.version$minor, \". Welcome. \", sep = \"\")");
  print_startup_msg();
  
  // default options
  R::R_run("options(deparse.max.lines = 5)");
  R::R_run("options(warnPartialMatchArgs = TRUE)");
  R::R_run("options(warnPartialMatchAttr = TRUE)");
  R::R_run("options(warnPartialMatchDollar = TRUE)");
  R::R_run("if(getOption(\"repos\") == \"@CRAN@\") options(repos = \"https://cran.rstudio.com\")");
  
  init_ok = true;
  
  // setting the prompt if needed
  const string new_prompt = concom.get_program_option("prompt.main").get_string();
  if(new_prompt != "> "){
    R::R_run("options(prompt = " + str::dquote(new_prompt) + ")");
  }
  
}

void RLanguageServer::exit_R(){
  
  // exit R
  util::msg("R::Rf_endEmbeddedR(0)");
  R::Rf_endEmbeddedR(0);
  
  // unload DLLs
  util::msg("Unloading the DLLs");
  for(auto const &x : dll_handles){
    FreeLibrary(x.second);
  }
  
  dll_handles.clear();
  util::msg("... done");
  
  
  // Later:
  // display screen with choice of R version and vanilla mode or not
}


CmdParsed RLanguageServer::parse_command(const string &cmd){
  // Later:
  // - improve the parsing so that you can provide parser errors early and precisely
  R::Protector protect;
  R::SEXP cmd_sexp = protect.add(R::Rf_ScalarString(R::Rf_mkCharCE(cmd.c_str(), R::CE_UTF8)));
  
  R::ParseStatus status;
  R::R_ParseVector(cmd_sexp, -1, &status, R::R_NilValue);
  
  CmdParsed res;
  if(status == R::ParseStatus::PARSE_ERROR){
    res.is_error = true;
  } else if(status == R::ParseStatus::PARSE_INCOMPLETE){
    res.is_continuation = true;
  } else if(status == R::ParseStatus::PARSE_OK){
    res.is_continuation = false;
  }
  
  return res;
}


void RLanguageServer::main_loop(){
  
  do_restart = true;
  while(do_restart){
    do_restart = false;
    run_repl();
  }
  
}


void RLanguageServer::run_repl(){
  
  init_R();
  
  if(!init_ok){
    return;
  }
  
  // setting the right width
  uint width = concom.window_width();
  resize_window_width(width);
  
  R::run_Rmainloop();
  
  util::msg("Out of the R main loop");
  
  exit_R();
}

void RLanguageServer::resize_window_width(uint width){
  string cmd = "options(width = " + std::to_string(width) + ")";
  R::R_run(cmd);
}

