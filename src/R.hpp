

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>

#include "constants.hpp"
#include "util.hpp"

using std::string;
using std::vector;

namespace R {

//
// R data structures -----------------------------------------------------------
//


// Rstartup.h

// NOTA: since this is c code, I should use "typedef enum" (I cannot rm the typdef as in c++)
// not sure if replacing the "#define" below with "using" is allowed, for the same reason
// 
#define R_SIZE_T std::size_t

extern "C" {

typedef int R_len_t;

typedef enum { FALSE = 0, TRUE /*, MAYBE */ } Rboolean;

typedef enum {RGui, RTerm, LinkDLL} UImode;

#define RSTART_VERSION 1 /* version 1 introduced in R 4.2.0 */

/* Startup Actions */
typedef enum {
  SA_NORESTORE,/* = 0 */
  SA_RESTORE,
  SA_DEFAULT,/* was === SA_RESTORE */
  SA_NOSAVE,
  SA_SAVE,
  SA_SAVEASK,
  SA_SUICIDE
} SA_TYPE;

typedef struct {
  Rboolean R_Quiet;
  Rboolean R_NoEcho;
  Rboolean R_Interactive;
  Rboolean R_Verbose;
  Rboolean LoadSiteFile;
  Rboolean LoadInitFile;
  Rboolean DebugInitFile;
  SA_TYPE RestoreAction;
  SA_TYPE SaveAction;
  R_SIZE_T vsize;
  R_SIZE_T nsize;
  R_SIZE_T max_vsize;
  R_SIZE_T max_nsize;
  R_SIZE_T ppsize;
  Rboolean NoRenviron : 16;
  int RstartVersion : 16;
  int nconnections;
  char *rhome;
  char *home; 
  int (*ReadConsole) (const char *, unsigned char *, int, int);
  void (*WriteConsole) (const char *, int);
  void (*CallBack) (void);   
  void (*ShowMessage) (const char *);
  int (*YesNoCancel) (const char *);
  void (*Busy) (int);
  UImode CharacterMode;
  void (*WriteConsoleEx) (const char *, int, int);
  Rboolean EmitEmbeddedUTF8;
  void (*CleanUp)(SA_TYPE, int, int);
  void (*ClearerrConsole)(void);
  void (*FlushConsole)(void);
  void (*ResetConsole) (void);
  void (*Suicide) (const char *s);
   
} structRstart;

typedef structRstart *Rstart;


// Defn.h

// We will need to manipulate SEXP, so we need to define the type

typedef unsigned int SEXPTYPE;

struct sxpinfo_struct {
  SEXPTYPE type      :  5;
  unsigned int scalar:  1;
  unsigned int obj   :  1;
  unsigned int alt   :  1;
  unsigned int gp    : 16;
  unsigned int mark  :  1;
  unsigned int debug :  1;
  unsigned int trace :  1;
  unsigned int spare :  1;
  unsigned int gcgen :  1;
  unsigned int gccls :  3;
  unsigned int named : 16;
  unsigned int extra : 16;
};

struct primsxp_struct {
  int offset;
};

struct symsxp_struct {
  struct SEXPREC *pname;
  struct SEXPREC *value;
  struct SEXPREC *internal;
};

struct listsxp_struct {
  struct SEXPREC *carval;
  struct SEXPREC *cdrval;
  struct SEXPREC *tagval;
};

struct envsxp_struct {
  struct SEXPREC *frame;
  struct SEXPREC *enclos;
  struct SEXPREC *hashtab;
};

struct closxp_struct {
  struct SEXPREC *formals;
  struct SEXPREC *body;
  struct SEXPREC *env;
};

struct promsxp_struct {
  struct SEXPREC *value;
  struct SEXPREC *expr;
  struct SEXPREC *env;
};

typedef struct SEXPREC {
  struct sxpinfo_struct sxpinfo;
  struct SEXPREC *attrib;
  struct SEXPREC *gengc_next_node, *gengc_prev_node;
  union {
    struct primsxp_struct primsxp;
    struct symsxp_struct symsxp;
    struct listsxp_struct listsxp;
    struct envsxp_struct envsxp;
    struct closxp_struct closxp;
    struct promsxp_struct promsxp;
  } u;
} SEXPREC;


// include/Rinternals.h
typedef struct SEXPREC *SEXP;

// include/R_ext/Parse.h

typedef enum {
  PARSE_NULL,
  PARSE_OK,
  PARSE_INCOMPLETE,
  PARSE_ERROR,
  PARSE_EOF
} ParseStatus;

// include/Rinternals.h
typedef enum {
    CE_NATIVE = 0,
    CE_UTF8   = 1,
    CE_LATIN1 = 2,
    CE_BYTES  = 3,
    CE_SYMBOL = 5,
    CE_ANY    =99
} cetype_t;

typedef ptrdiff_t R_xlen_t;

} // extern "C"

constexpr unsigned int NILSXP     = 0;
constexpr unsigned int SYMSXP     = 1;
constexpr unsigned int LISTSXP    = 2;
constexpr unsigned int CLOSXP     = 3;
constexpr unsigned int ENVSXP     = 4;
constexpr unsigned int PROMSXP    = 5;
constexpr unsigned int LANGSXP    = 6;
constexpr unsigned int SPECIALSXP = 7;
constexpr unsigned int BUILTINSXP = 8;
constexpr unsigned int CHARSXP    = 9;
constexpr unsigned int LGLSXP     = 10;
constexpr unsigned int INTSXP     = 13;
constexpr unsigned int REALSXP    = 14;
constexpr unsigned int CPLXSXP    = 15;
constexpr unsigned int STRSXP     = 16;
constexpr unsigned int DOTSXP     = 17;
constexpr unsigned int ANYSXP     = 18;
constexpr unsigned int VECSXP     = 19;
constexpr unsigned int EXPRSXP    = 20;
constexpr unsigned int BCODESXP   = 21;
constexpr unsigned int EXTPTRSXP  = 22;
constexpr unsigned int WEAKREFSXP = 23;
constexpr unsigned int RAWSXP     = 24;
constexpr unsigned int OBJSXP     = 25;
constexpr unsigned int S4SXP      = 25;
constexpr unsigned int NEWSXP     = 30;
constexpr unsigned int FREESXP    = 31;
constexpr unsigned int FUNSXP     = 99;

//
// R functions to load --------------------------------------------------------- 
//

#define RAPI extern __cdecl

// Rembedded.h
RAPI char *(*getDLLVersion)(void);
RAPI char *(*getRUser)(void);
RAPI char *(*get_R_HOME)(void);
RAPI int (*Rf_initEmbeddedR)(int argc, char *argv[]);
RAPI void (*Rf_endEmbeddedR)(int fatal);
RAPI void (*R_setStartTime)(void);
RAPI int (*R_DefParams)(Rstart);
RAPI int (*R_DefParamsEx)(Rstart, int);
RAPI void (*R_SetParams)(Rstart);
RAPI void (*freeRUser)(char *);
RAPI void (*free_R_HOME)(char *);
RAPI void (*R_set_command_line_arguments)(int argc, char **argv);
RAPI void (*readconsolecfg)(void);
RAPI void (*setup_Rmainloop)(void);
RAPI int (*Rf_initialize_R)(int ac, char **av);
RAPI void (*R_ProcessEvents)(void);
// RAPI void (**R_PolledEvents)(void);

RAPI int (*GA_initapp)(int, char **);
RAPI int (*GA_peekevent)(void);

// main/gram.c
RAPI SEXP (*R_ParseVector)(SEXP text, int n, ParseStatus *status, SEXP srcfile);

// main/context.c
RAPI SEXP (*R_tryEval)(SEXP e, SEXP env, int *ErrorOccurred);

// include/Rinternals.h
RAPI SEXP (*Rf_mkCharCE)(const char *, cetype_t);
RAPI SEXP (*Rf_mkString)(const char *);
RAPI const char *(*Rf_translateCharUTF8)(SEXP);
RAPI SEXP (*STRING_ELT)(SEXP, R_xlen_t);
RAPI SEXP (*Rf_protect)(SEXP);
RAPI void (*Rf_unprotect)(int);
RAPI SEXP (*VECTOR_ELT)(SEXP x, R_xlen_t i);
RAPI R_len_t (*Rf_length)(SEXP);
RAPI SEXP (*Rf_ScalarString)(SEXP);
RAPI SEXP (*Rf_ScalarInteger)(int);
RAPI SEXP (*Rf_ScalarLogical)(int);
RAPI SEXP (*Rf_ScalarReal)(double);
RAPI int *(*INTEGER)(SEXP);
RAPI double *(*REAL)(SEXP);

// main/print.c
RAPI void (*Rf_PrintValue)(SEXP);

// main/main.c
RAPI void (*run_Rmainloop)();

#undef RAPI

//
// R objects to load ----------------------------------------------------------- 
//

//
// variables

extern SEXP R_NilValue;
extern SEXP R_GlobalEnv;

//
// pointers to variables

extern int *pR_SignalHandlers;
extern int *pUserBreak;
extern int *pCharacterMode;



//
// CPP_SEXP -------------------------------------------------------------------- 
//

string show_sexptype(SEXP x);

inline SEXPTYPE TYPEOF(SEXP x){
  return x->sxpinfo.type;
}

// this class takes care of conversions between R types and C types
class CPP_SEXP {
  SEXP Rexpr = R_NilValue;
  string error_message = UNSET::STRING;
  
  bool is_error() const {
    if(!util::is_unset(error_message)){
      std::cout << "The R expression led to an error, implicit conversions lead to default values\n";
      // line below to be modified to stg more meaningful
      return true;
    }
    
    return false;
  }
  
public:
  void set_error(const string &err_msg){
    error_message = err_msg;
  }
  
  void set_SEXP(SEXP x){
    Rexpr = x;
  }
  
  //
  // utilities
  //
  
  uint size() const { return Rf_length(Rexpr); }
  bool empty() const { return size() == 0; }
  
  //
  // definition of implicit conversions
  //
  
  // NOTA: R will pop errors if we mess up here => no need to be too careful
  // hmmm: second thoughts: there's this long jump issue/custom error handlers thing
  // => if I'm not sure that the code I'm running is 100% safe, I should be more 
  // careful.
  // 
  
  operator SEXP() const {
    if(is_error()){
      throw std::runtime_error("BUG: error when evaluating an R expression that shouldn't have failed");
    }
    return Rexpr;
  }
  
  operator const char *() const {
    if(is_error()){
      throw std::runtime_error("BUG: error when evaluating an R expression that shouldn't have failed");
    }
    const SEXP &Rstr = Rexpr;
    return Rf_translateCharUTF8(STRING_ELT(Rstr, 0));
  }
  
  operator string() const {
    if(is_error()){
      return UNSET::STRING;
    }
    const SEXP &Rstr = Rexpr;
    const char *cstr = Rf_translateCharUTF8(STRING_ELT(Rstr, 0));
    string res(cstr);
    return res;
  }
  
  operator vector<string>() const {
    if(is_error()){
      return UNSET::STRING_VECTOR;
    }
    const SEXP &Rstr = Rexpr;
    vector<string> res;
    
    uint n = Rf_length(Rstr);
    // std::cout << "converting to string, n = " << n << "\n";
    
    for(uint i=0 ; i<n ; ++i){
      const char *cstr = Rf_translateCharUTF8(STRING_ELT(Rstr, i));
      // std::cout << "- " << cstr << "\n";
      res.push_back(cstr);
    }
    
    return res;
  }
  
  operator bool() const {
    if(is_error()){
      return false;
      // throw std::runtime_error("BUG: error when evaluating an R expression that shouldn't have failed");
    }
    
    bool res = false;
    if(TYPEOF(Rexpr) == INTSXP || TYPEOF(Rexpr) == LGLSXP){
      res = INTEGER(Rexpr)[0] != 0;
    } else if(TYPEOF(Rexpr) == REALSXP){
      res = REAL(Rexpr)[0] != 0;
    } else {
      std::cout << "CPP_SEXP: Error when converting to bool.\nType " << show_sexptype(Rexpr) << " is not supported.\n";
    }
    
    return res;
  }
  
  operator int() const {
    if(is_error()){
      throw std::runtime_error("BUG: error when evaluating an R expression that shouldn't have failed");
    }
    const SEXP &Rvec = Rexpr;
    
    int res = 0;
    if(TYPEOF(Rvec) == INTSXP || TYPEOF(Rexpr) == LGLSXP){
      res = INTEGER(Rvec)[0];
    } else if(TYPEOF(Rvec) == REALSXP){
      res = std::round(REAL(Rvec)[0]);
    } else {
      std::cout << "CPP_SEXP: Error when converting to int.\nType " << show_sexptype(Rexpr) << " is not supported.\n";
    }
    
    return res;
  }
  
  operator vector<int>() const {
    if(is_error()){
      throw std::runtime_error("BUG: error when evaluating an R expression that shouldn't have failed");
    }
    const SEXP &Rvec = Rexpr;
    uint n = Rf_length(Rvec);
    vector<int> res;
    const bool is_int = TYPEOF(Rvec) == INTSXP || TYPEOF(Rexpr) == LGLSXP;
    
    if(!is_int && TYPEOF(Rvec) != REALSXP){
      std::cout << "CPP_SEXP: Error when converting to vector<int>.\nType " << show_sexptype(Rexpr) << " is not supported.\n";
    }
    
    for(uint i=0 ; i<n ; ++i){
      if(is_int){
        res.push_back(INTEGER(Rvec)[i]);
      } else {
        res.push_back(std::round(REAL(Rvec)[i]));
      }
    }
    
    return res;
  }
  
};

//
// Protector ------------------------------------------------------------------- 
//

class Protector {
private:
  unsigned int n = 0;
public:
  SEXP add(SEXP x){
    ++n;
    return Rf_protect(x);
  }
  
  ~Protector(){
    Rf_unprotect(n);
  }
  
};

//
// functions ------------------------------------------------------------------- 
//

CPP_SEXP R_run(string x);
SEXP R_run_sexp(string x);

//
// inline ----------------------------------------------------------------------
//

class existsOpts {
  bool _is_no_inherits = false;
  bool _is_function = false;
public:
  existsOpts() = default;
  
  existsOpts& mode_function(){
    _is_function = true;
    return *this;
  }
  
  existsOpts& dont_inherit(){
    _is_no_inherits = true;
    return *this;
  }
  
  bool is_function() const { return _is_function; }
  bool is_no_inherits() const { return _is_no_inherits; }
  
};

inline bool exists(std::string x, const existsOpts opts = existsOpts()){
  
  if(x.empty()){
    return false;
  }
  
  string suffix;
  if(opts.is_function()){
    suffix += ", mode = \"function\"";
  }
  if(opts.is_no_inherits()){
    suffix += ", inherits = FALSE";
  }
  
  string command = "exists(\"" + x + "\"" + suffix + ")";
  R::CPP_SEXP exists_r = R::R_run(command);
  if(exists_r.empty() || !static_cast<bool>(exists_r)){
    return false;
  } 
  
  return true;
}

inline int length(std::string x){
  if(!exists(x)){
    return -1;
  }
  
  return static_cast<int>(R::R_run("length(" + x + ")[1]"));
}

const string R_INVALID_CHARACTER = " +-*=<>!?|&$@[]{}()'~,;:/\\#";
const string R_INVALID_FIRST_CHARACTER = "_0123456789" + R_INVALID_CHARACTER;

inline bool is_valid_name(const string &x){
  // We have to do it that way beacuse R considers A LOT of weird unicode characters as valid 
  // in names
  
  if(x.empty()){
    return false;
  }
  
  if(R_INVALID_FIRST_CHARACTER.find(x[0]) != string::npos){
    return false;
  }
  
  for(uint i=1 ; i<x.size() ; ++i){
    if(R_INVALID_CHARACTER.find(x[i]) != string::npos){
      return false;
    }
  }
  
  return true;
}


} // namespace R
