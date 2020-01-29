//Этот файл нужно подрубать гденить в .h файлах, где нет NICKNAME, NICKTRACE
#define _NO_TEST_H_
#include "slogger.h"

#define LogTrace1 LogTrace(1,"",__FILE__,__LINE__)
#define LogTrace5 LogTrace(12,"",__FILE__,__LINE__)
#define LogError_ LogError("",__FILE__,__LINE__)

#undef _NO_TEST_H_
