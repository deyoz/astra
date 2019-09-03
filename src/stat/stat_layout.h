#ifndef _STAT_LAYOUT_H_
#define _STAT_LAYOUT_H_

#include "astra_consts.h"
#include "oralib.h"
#include <libxml/tree.h>

struct TParamItem {
    std::string code;
    int visible;
    std::string label;
    std::string caption;
    std::string ctype;
    std::string name; // control name property
    int width;
    int len;
    int isalnum;
    std::string ref;
    std::string ref_field;
    std::string tag;
    std::string edit_fmt;
    std::string filter;
    TParamItem():
        visible(ASTRA::NoExists),
        width(ASTRA::NoExists),
        len(ASTRA::NoExists),
        isalnum(ASTRA::NoExists)
    {}
    void fromDB(TQuery &Qry);
    void toXML(xmlNodePtr resNode);
};

struct TLayout {
    std::map<int, TParamItem> params;
    void get_params();
    void toXML(xmlNodePtr resNode);
    void toXML(xmlNodePtr resNode, const std::string &tag, const std::string &qry);
};

#endif
