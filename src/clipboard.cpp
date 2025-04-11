

#include "clipboard.hpp"

// RAII: ressource acquisition is initialization
// => you implement the necessary cleanup in the destructor
//

namespace simpleclipboard {

class RaiiOpenClipboard {
  bool error = false;
public:
  RaiiOpenClipboard(){
    if(!OpenClipboard(nullptr)){
      error = true;
    }
  }
  
  ~RaiiOpenClipboard(){
    CloseClipboard();
  }
  
  bool is_error(){ return error; };
  
};

class RaiiAllocHandle {
  HGLOBAL handle;
public:
  
  explicit RaiiAllocHandle(size_t n_bytes){
    this->handle = GlobalAlloc(GMEM_MOVEABLE, n_bytes);
  }
  
  ~RaiiAllocHandle(){
    GlobalFree(handle);
  }
  
  bool is_error() { return handle == nullptr; };
  HGLOBAL get_handle() { return handle; };
  
};

class RaiiLockHandle {
  HANDLE handle;
  LPVOID handle_lock;
public:
  
  explicit RaiiLockHandle(HANDLE handle){
    this->handle = handle;
    handle_lock = GlobalLock(handle);
  }
  
  explicit RaiiLockHandle(RaiiAllocHandle &allocated_handle){
    this->handle = allocated_handle.get_handle();
    handle_lock = GlobalLock(handle);
  }
  
  ~RaiiLockHandle(){
    GlobalUnlock(handle);
  }
  
  bool is_error() { return handle_lock == nullptr; };
  const char* char_pointer() { return static_cast<const char*>(handle_lock); };
  const wchar_t* wide_char_pointer() { return static_cast<const wchar_t*>(handle_lock); };
  LPVOID raw_pointer() { return handle_lock; };
  
};


string get_clipboard(){
  
  RaiiOpenClipboard cb_open;
  if(cb_open.is_error()){
    return "";
  }
  
  // we favor CF_UNICODETEXT over CF_TEXT
  
  bool is_unicode = true;
  HANDLE handle = GetClipboardData(CF_UNICODETEXT);
  if(handle == nullptr){
    is_unicode = false;
    // we try raw text
    handle = GetClipboardData(CF_TEXT);
    if(handle == nullptr){
      return "";
    }
  }

  RaiiLockHandle cb_lock(handle);
  if(cb_lock.is_error()){
    return "";
  }
  
  string str;
  if(is_unicode){
    wstring wstr(cb_lock.wide_char_pointer());
    
    bool is_error = false;
    str = stringtools::utf16_to_utf8(wstr, is_error);
    if(is_error){
      return "";
    }
  } else {
    str = string{cb_lock.char_pointer()};
  }

  return str;
}


void set_clipboard(string str){
  
  if(str.empty()){
    return;
  }
  
  RaiiOpenClipboard cb_open;
  if(cb_open.is_error()){
    return;
  }
  
  EmptyClipboard();
  
  // we fill both CF_TEXT and CF_UNICODETEXT
  
  std::vector<int> formats = {CF_TEXT, CF_UNICODETEXT};
  int n = 2;
  bool is_error = false;
  wstring wstr = stringtools::utf8_to_utf16(str, is_error);
  if(is_error){
    n = 1;
  }
  
  for(int i=0 ; i<n ; ++i){
    
    int fmt = formats[i];
    
    size_t byte_size = 0;
    void *data_ptr = nullptr;
    if(fmt == CF_UNICODETEXT){
      byte_size = (wstr.size() + 1) * 2;
      data_ptr = &wstr[0];
    } else {
      byte_size = str.size() + 1;
      data_ptr = &str[0];
    }
    
    RaiiAllocHandle handle_alloc(byte_size);
    if(handle_alloc.is_error()){
      return;
    }
    
    RaiiLockHandle cb_lock(handle_alloc);
    if(cb_lock.is_error()){
      return;
    }
    
    memcpy(cb_lock.raw_pointer(), data_ptr, byte_size);
    
    SetClipboardData(fmt, handle_alloc.get_handle());  
  }
  
}

} // namespace simpleclipboard

