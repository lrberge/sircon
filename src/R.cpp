
#include "R.hpp"

namespace R {
  
//
// Functions to be loaded from the DLL -----------------------------------------
//

#define RAPI __cdecl

// Rembedded.h
RAPI char *(*getDLLVersion)(void) = nullptr;
RAPI char *(*getRUser)(void) = nullptr;
RAPI char *(*get_R_HOME)(void) = nullptr;
RAPI int (*Rf_initEmbeddedR)(int argc, char *argv[]) = nullptr;
RAPI void (*Rf_endEmbeddedR)(int fatal) = nullptr;
RAPI void (*R_setStartTime)(void) = nullptr;
RAPI int (*R_DefParams)(Rstart) = nullptr;
RAPI int (*R_DefParamsEx)(Rstart, int) = nullptr;
RAPI void (*R_SetParams)(Rstart) = nullptr;
RAPI void (*freeRUser)(char *) = nullptr;
RAPI void (*free_R_HOME)(char *) = nullptr;
RAPI void (*R_set_command_line_arguments)(int argc, char **argv) = nullptr;
RAPI void (*readconsolecfg)(void) = nullptr;
RAPI void (*setup_Rmainloop)(void) = nullptr;
RAPI int (*Rf_initialize_R)(int ac, char **av) = nullptr;
RAPI void (*R_ProcessEvents)(void) = nullptr;
// RAPI void (**R_PolledEvents)(void);

RAPI int (*GA_initapp)(int, char **) = nullptr;
RAPI int (*GA_peekevent)(void) = nullptr;

// main/gram.c
RAPI SEXP (*R_ParseVector)(SEXP text, int n, ParseStatus *status, SEXP srcfile) = nullptr;

// main/context.c
RAPI SEXP (*R_tryEval)(SEXP e, SEXP env, int *ErrorOccurred) = nullptr;

// include/Rinternals.h
RAPI SEXP (*Rf_mkCharCE)(const char *, cetype_t) = nullptr;
RAPI SEXP (*Rf_mkString)(const char *) = nullptr;
RAPI const char *(*Rf_translateCharUTF8)(SEXP) = nullptr;
RAPI SEXP (*STRING_ELT)(SEXP, R_xlen_t) = nullptr;
RAPI SEXP (*Rf_protect)(SEXP) = nullptr;
RAPI void (*Rf_unprotect)(int) = nullptr;
RAPI SEXP (*VECTOR_ELT)(SEXP x, R_xlen_t i) = nullptr;
RAPI R_len_t (*Rf_length)(SEXP) = nullptr;
RAPI SEXP (*Rf_ScalarString)(SEXP) = nullptr;
RAPI SEXP (*Rf_ScalarInteger)(int) = nullptr;
RAPI SEXP (*Rf_ScalarLogical)(int) = nullptr;
RAPI SEXP (*Rf_ScalarReal)(double) = nullptr;
RAPI int *(*INTEGER)(SEXP) = nullptr;
RAPI double *(*REAL)(SEXP) = nullptr;

// main/print.c
RAPI void (*Rf_PrintValue)(SEXP) = nullptr;

// main/main.c
RAPI void (*run_Rmainloop)() = nullptr;

#undef RAPI

//
// R objects to load from DLL -------------------------------------------------- 
//

//
// variables

SEXP R_NilValue = nullptr;
SEXP R_GlobalEnv = nullptr;

//
// pointers to variables

int *pR_SignalHandlers = nullptr;
int *pUserBreak = nullptr;
int *pCharacterMode = nullptr;


//
// Functions ------------------------------------------------------------------- 
//

string show_sexptype(SEXP x){
  switch(static_cast<int>(TYPEOF(x))) {
    case NILSXP:     return "NILSXP";
    case SYMSXP:     return "SYMSXP";
    case LISTSXP:    return "LISTSXP";
    case CLOSXP:     return "CLOSXP";
    case ENVSXP:     return "ENVSXP";
    case PROMSXP:    return "PROMSXP";
    case LANGSXP:    return "LANGSXP";
    case SPECIALSXP: return "SPECIALSXP";
    case BUILTINSXP: return "BUILTINSXP";
    case CHARSXP:    return "CHARSXP";
    case LGLSXP:     return "LGLSXP";
    case INTSXP:     return "INTSXP";
    case REALSXP:    return "REALSXP";
    case CPLXSXP:    return "CPLXSXP";
    case STRSXP:     return "STRSXP";
    case DOTSXP:     return "DOTSXP";
    case ANYSXP:     return "ANYSXP";
    case VECSXP:     return "VECSXP";
    case EXPRSXP:    return "EXPRSXP";
    case BCODESXP:   return "BCODESXP";
    case EXTPTRSXP:  return "EXTPTRSXP";
    case WEAKREFSXP: return "WEAKREFSXP";
    case RAWSXP:     return "RAWSXP";
    case S4SXP:      return "S4SXP";
    case NEWSXP:     return "NEWSXP";
    case FREESXP:    return "FREESXP";
    case FUNSXP:     return "FUNSXP";
    default:         return "UNKNOWN:ERROR";
  }
}

CPP_SEXP R_run(string x){
  // we run this silently
  // TODO:
  // - avoid top long jump when evaluating
  // - disable custom error handlers that can mess things up if error
  //
  
  // std::cout << "Command run by R: '" << x << "'\n";
  
  CPP_SEXP res;
  
  Protector protect;
  SEXP cmd = protect.add(Rf_ScalarString(Rf_mkCharCE(x.c_str(), CE_UTF8)));
  
  ParseStatus status;
  SEXP parse_result = protect.add(R_ParseVector(cmd, -1, &status, R_NilValue));
  
  SEXP result = R_NilValue;
  if(status == ParseStatus::PARSE_OK){
    
    int err = 0;
    result = protect.add(result);
    if(TYPEOF(parse_result) == EXPRSXP){
      int n = Rf_length(parse_result);
      for(int i=0 ; i <n && err == 0 ; ++i){
        SEXP el = VECTOR_ELT(parse_result, i);
        result = R_tryEval(el, R_GlobalEnv, &err);
      }
    } else {
      result = R_tryEval(parse_result, R_GlobalEnv, &err);
    }
    
    // we return the last result => if error, that's fine
    // maybe we should catch the errors?
    
    if(err != 0){
      string err_msg = "R_run(\"" + x + "\")" + "\nError at run time.";
      res.set_error(err_msg);
      util::error_msg("Internal error: ", err_msg);
    } else {
      res.set_SEXP(result);
    }
    
  } else {
    string err_msg = "R_run(\"" + x + "\")" + "\nParsing error.";
    res.set_error(err_msg);
    util::error_msg("Internal error: ", err_msg);
  }
  
  return res;
}


SEXP R_run_sexp(string x){
  // we run this silently
  // TODO:
  // - avoid top long jump when evaluating
  // - disable custom error handlers that can mess things up if error
  //
  
  // std::cout << "Command run by R: '" << x << "'\n";
  
  Protector protect;
  SEXP cmd = protect.add(Rf_ScalarString(Rf_mkCharCE(x.c_str(), CE_UTF8)));
  
  ParseStatus status;
  SEXP parse_result = protect.add(R_ParseVector(cmd, -1, &status, R_NilValue));
  
  SEXP result = R_NilValue;
  if(status == ParseStatus::PARSE_OK){
    
    int err = 0;
    result = protect.add(result);
    if(TYPEOF(parse_result) == EXPRSXP){
      int n = Rf_length(parse_result);
      for(int i=0 ; i <n && err == 0 ; ++i){
        SEXP el = VECTOR_ELT(parse_result, i);
        result = R_tryEval(el, R_GlobalEnv, &err);
      }
    } else {
      result = R_tryEval(parse_result, R_GlobalEnv, &err);
    }
    
    // we return the last result => if error, that's fine
    // maybe we should catch the errors?
    
    if(err != 0){
      std::cout << "R_run(\"" + x + "\")" + "\nError at run time.";
    }
    
  } else {
    std::cout << "R_run(\"" + x + "\")" + "\nParsing error.";
  }
  
  return result;
}


} // namespace R
  
