#ifndef _ASTRA_EDI_HANDLER_H
#define _ASTRA_EDI_HANDLER_H
#include <string>

struct tlg_info
{
#warning tlg id is not integer!
  int id;
  std::string text;
  std::string sender;

  int proc_attempt;
};

void handle_edi_tlg(const tlg_info &tlg);

#endif /* _ASTRA_EDI_HANDLER_H */
