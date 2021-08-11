#pragma once

#include <string>
#include <vector>


namespace DbCpp
{
  
struct TCheckSessionsLoadSaveConsistency
{
  std::string nick;
  const char* file;
  size_t line;
  std::string text_error;
};
  
std::vector<TCheckSessionsLoadSaveConsistency> check_autonomous_sessions_load_save_consistency();

} // namespace DbCpp
