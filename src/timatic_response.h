#pragma once

#include <libxml/tree.h>
#include "libtimatic/timatic_http.h"

void TimaticResponseToXML(xmlNodePtr bodyNode, const Timatic::HttpData &httpData);
