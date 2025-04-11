
#include "../src/stringtools.hpp"
#include "../src/VTS.hpp"

using namespace util;
using namespace stringtools;

// NOTA:
// simplified regex:
// - | = or
// - +, *
// - ^, $
// - .
// - \\s, \\S
// 

int main(){
  
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  
  msg("delete_until");
  
  string x = "bonjour    les gens";
  
  test_eq_str(delete_until(x, "les"), " gens");
  test_eq_str(delete_until(x, " +"), "les gens");
  test_eq_str(delete_until(x, {"e", " +"}), "les gens");
  test_eq_str(delete_until(x, {"les", "gens"}), " gens");
  
  msg("delete_after");
  
  test_eq_str(delete_after(x, " "), "bonjour");
  test_eq_str(delete_after(x, {" ", "jour"}), "bon");
  
  msg("str_split");
  
  test_eq_vec_str(str_split(x, " +"), {"bonjour", "les", "gens"});
  test_eq_vec_str(str_split(x, {"our", " +"}), {"bonj", "", "les", "gens"});
  
  string ex_options = "options.color_fun.set #117755";
  test_eq_vec_str(str_split(ex_options, {".", " +"}), {"options", "color_fun", "set", "#117755"});
  
  msg("delete_pattern");
  
  test_eq_str(delete_pattern("bonjour", "jour"), "bon");
  test_eq_str(delete_pattern("bonjour les gens!", "jour"), "bon les gens!");
  test_eq_str(delete_pattern("bonjour les gens!", {"jour", "!"}), "bon les gens");
  
  msg("replace pattern");
  
  test_eq_str(str_replace("bonjour", "jour", "soir"), "bonsoir");
  
  msg("stpli at width");
  
  string m = VTS::FG_BRIGHT_BLUE + "Bonjour Bergé" + VTS::FG_BRIGHT_GREEN + " et Marlène!" + VTS::FG_DEFAULT;
  vector<string> m_7 = str_split_at_width(m, 7);
  
  std::cout << m << "\nSplit at 7 chars:\n";
  for(auto &s : m_7){
    std::cout << "\"" << s << "\"\n";
  }
  
  msg("shorten");
  
  vector<string> lb = {"laurent", "robert", "bergé"};
  vector<string> lb_short = shorten(lb, 5);
  for(auto &s : lb_short){
    std::cout << "\"" << s << "\"\n";
  }
  
  
  msg("tests perfomed successfully");
  
  return 0;
}

