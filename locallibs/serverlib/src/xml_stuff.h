#ifndef _XML_STUFF_
#define _XML_STUFF_

#include <libxml/tree.h>
#include <vector>
#include <string>


#define SKIPBLANKS(x) while(((x)!=NULL)&&(xmlIsBlankNode(x)||((x)->type==XML_COMMENT_NODE))) x=(x)->next;


/* Перекодировка содержимого текстовых узлов и атрибутов из CP866 в UTF-8 */
int xml_encode_nodelist(xmlNodePtr node);

/* Перекодировка содержимого текстовых узлов и атрибутов из UTF-8 в CP866 */
int xml_decode_nodelist(xmlNodePtr node);

/**
 * @brief redirect xml error output to sirena log
*/
void xmlErrorToSirenaLog();

std::vector<uint8_t> CP866toUTF8(const uint8_t* buf, const size_t bufsize);
inline std::vector<uint8_t> CP866toUTF8(const std::vector<uint8_t>& buf) {  return CP866toUTF8(buf.data(), buf.size());  }

std::string CP866toUTF8(const std::string& value);
bool CP866toUTF8(const char* buf, size_t bufsize, std::string& out);
std::string UTF8toCP866(const std::string& value);
std::string CP866toUTF8(char);
std::string xml_node_dump(const xmlNodePtr, int format=1);

#endif

