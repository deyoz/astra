#ifndef _DOCS_EXAM_H_
#define _DOCS_EXAM_H_

#include "docs_pax_list.h"
#include "docs_common.h"

void EXAM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void EXAMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void WEB(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);
void WEBTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode);

#endif
