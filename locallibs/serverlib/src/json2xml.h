#pragma once

#include <libxml/tree.h>
#include <string>

#include "json_spirit_fwd.h"

class Message;

namespace HelpCpp
{

Message json2xml(xmlNodePtr, const json_spirit::mValue&);
Message json2xml(std::string&, const std::string& rootName, const json_spirit::mValue&);

} // HelpCpp
