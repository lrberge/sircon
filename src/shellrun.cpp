

#include "shellrun.hpp"

namespace {

constexpr unsigned int BUFSIZE = 4096;

string read_out_handle(HANDLE handle){
  DWORD n_read;
  CHAR buffer[BUFSIZE];
  bool success;
  
  // unknown output size, we need a buffer
  string output;
  string tmp;
  int i = 0;
  while(true){
    ++i;
    success = ReadFile(handle, buffer, BUFSIZE, &n_read, NULL);
    if(!success || n_read == 0){
      break;
    }
    
    
    tmp.append(buffer, n_read);
    output += tmp;
    
    if(n_read == 2 && buffer[0] == '>' && buffer[1] == ' '){
      break;
    }
    
    const size_t &n = output.size();
    if(n > 2){
      string end = output.substr(n - 3, 3);
      if(end == "@@@"){
        output.erase(output.begin() + n - 4 - 1, output.end());
        break;
      }
      
    }
    
    tmp = "";
    
  }
  
  if(!output.empty() && output.back() == '\n'){
    output.erase(output.end() - 1);
    if(!output.empty() && output.back() == '\r'){
      output.erase(output.end() - 1);
    }
  }
  
  return output;
}

} // end anonymous namespace




string run_shell_command(const string &cmd, const string &args){
  // returns an unset string if it fails
  
  string res = UNSET::STRING;
  
  //
  // step 1: create a pipe to the handles 
  //
  
  // The hanldes
  HANDLE handle_IN_R = nullptr;
  HANDLE handle_IN_W = nullptr;
  HANDLE handle_OUT_R = nullptr;
  HANDLE handle_OUT_W = nullptr;
  
  // security attributes that need to be set upon pipe creation
  SECURITY_ATTRIBUTES secattr;
  
  secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
  secattr.bInheritHandle = 1;
  secattr.lpSecurityDescriptor = nullptr;
  
  // Create a pipe for the child process's STDOUT.
  if(!CreatePipe(&handle_OUT_R, &handle_OUT_W, &secattr, 0)){
    string funname = "run_shell_command(\"" + cmd + "\", \"" + args + "\"): \n";
    util::error_msg(funname, "Error when creating the STDOUT pipe");
    return res;
  }
  
  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if(!SetHandleInformation(handle_OUT_R, HANDLE_FLAG_INHERIT, 0)){
    string funname = "run_shell_command(\"" + cmd + "\", \"" + args + "\"): \n";
    util::error_msg(funname, "Error when ensuring non handle inheritance in STDOUT/read");
    return res;
  } 

  // Create a pipe for the child process's STDIN. 
  if(!CreatePipe(&handle_IN_R, &handle_IN_W, &secattr, 0)){
    string funname = "run_shell_command(\"" + cmd + "\", \"" + args + "\"): \n";
    util::error_msg(funname, "Error when creating the STDIN pipe");
    return res;
  }
  
  // Ensure the write handle to the pipe for STDIN is not inherited.
  if(!SetHandleInformation(handle_IN_W, HANDLE_FLAG_INHERIT, 0)){
    string funname = "run_shell_command(\"" + cmd + "\", \"" + args + "\"): \n";
    util::error_msg(funname, "Error when ensuring non handle inheritance in STDIN/write");
    return res;
  }
  
  
  //
  // step 2: creating the child process
  //
  
  std::string cmd_pipe = cmd + " " + args;
  std::vector<char> cmdBuf(cmd_pipe.size() + 1, '\0');
  cmd_pipe.copy(&(cmdBuf[0]), cmd_pipe.size());
  
  PROCESS_INFORMATION proc_info;
  STARTUPINFO startup;
  BOOL success;
  
  // ZeroMemory: fills a block of memory with 0s
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
  
  // we setup the startup information (important)
  ZeroMemory(&startup, sizeof(STARTUPINFO));
  
  startup.cb = sizeof(STARTUPINFO);
  startup.hStdError = handle_OUT_W;
  startup.hStdOutput = handle_OUT_W;
  startup.hStdInput = handle_IN_R;
  startup.dwFlags |= STARTF_USESTDHANDLES;
  
  success = CreateProcess(
    nullptr,        // No module name (use command line)
    cmdBuf.data(),  // Command
    nullptr,        // Process handle not inheritable
    nullptr,        // Thread handle not inheritable
    1,              // Set handle inheritance to FALSE
    CREATE_DEFAULT_ERROR_MODE,             // No creation flags
    nullptr,       // Use parent's environment block
    nullptr,       // Use parent's starting directory
    &startup,    // Pointer to STARTUPINFO structure
    &proc_info   // Pointer to PROCESS_INFORMATION structure
  );
  
  if(!success){
    string funname = "run_shell_command(\"" + cmd + "\", \"" + args + "\"): \n";
    util::error_msg(funname, "Error when creating the child process\n");
    
    CloseHandle(handle_OUT_W);
    CloseHandle(handle_IN_R);
    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);
    
    return res;
  }
  
  // we close the handles to stdin/out which are no longer needed in the child process
  CloseHandle(handle_OUT_W);
  CloseHandle(handle_IN_R);
  
  //
  // step 4: getting the stdout
  //
  
  res = read_out_handle(handle_OUT_R);
  
  CloseHandle(proc_info.hProcess);
  CloseHandle(proc_info.hThread);
  
  return res;
}





