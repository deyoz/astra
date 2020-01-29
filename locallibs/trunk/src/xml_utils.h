#ifndef __XML_UTILS_H1__
#define __XML_UTILS_H1__
#include <libxml/tree.h>

void XmlNodeTrace(int Level, const char *nickname, const char *filename,
                  int line, const xmlNodePtr m, int format=1);
void XmlDocTrace(int Level, const char *nickname, const char *filename,
                 int line, const xmlDocPtr doc);
#endif /* __XML_UTILS_H1__ */
