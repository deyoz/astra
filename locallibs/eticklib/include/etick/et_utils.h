/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#include <string>
#include <stdarg.h>
#include <stdexcept>

namespace EtUtils
{
std::string PrintApFormat(const char *format,
                          va_list ap,
                          const int StepSize=80);
}
