#ifndef __XP_TEST_UTILS_H__
#define __XP_TEST_UTILS_H__

#ifdef __cplusplus

#include <string>
#include "sirenaproc.h"

#define TEST_AWK    "ˆ‘Œ"
#define TEST_OPR    "’…‘’_’_0"
#define TEST_PUL    "’…‘’_0"
#define TEST_PASSWD "€‹œ"

void initTest();

void clear_base(int oc=1, int cc=1);
void prepare_base(int oc=1, int cc=1); 
// prepare_base assumes that GOS '”' and SFE 'Œ‚' exist !!!

void commit_base(void);
void rollback_base(void);

void InitXmlCtxt();
void ShutXmlCtxt();

class XpFail
{
  std::string Msg;
public:
  XpFail(const char *msg):Msg(msg?msg:""){};
  std::string getMsg()
  {
    return Msg;
  };
};

int xp_testing_sirena(const char *request, char *answer);

#endif /* __cplusplus */

#endif /* __XP_TEST_UTILS_H__ */
