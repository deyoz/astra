#include "docs_text_grid.h"
#include "xml_unit.h"
#include "stl_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

void TTextGrid::headerToXML(xmlNodePtr variablesNode)
{
    ostringstream s;
    s << left;
    fill_grid(s);
    NewTextChild(variablesNode,"page_header_bottom",s.str());
}

void TTextGrid::TRow::toXML(xmlNodePtr rowNode)
{
    ostringstream s;
    s << left;
    grd.fill_grid(s, *this);
    NewTextChild(rowNode,"str",s.str());
}

void TTextGrid::addCol(const string &caption, int length)
{
    if(length == NoExists)
        length = caption.size() + 1;
    tab_width += length;
    hdr.push_back(make_pair(caption, length));
}

TTextGrid::TRow &TTextGrid::addRow()
{
    grid.emplace_back(TRow(*this));
    return grid.back();
}

TTextGrid::TRow &TTextGrid::TRow::add(int data)
{
    return add(IntToString(data));
}

TTextGrid::TRow &TTextGrid::TRow::add(const string &data)
{
    push_back(data);
    return *this;
}

bool TTextGrid::check_fileds_empty(const map<int, vector<string>> &fields)
{
    bool result = false;
    for(const auto &i: fields)
        if(not i.second.empty()) {
            result = true;
            break;
        }
    return result;
}

void TTextGrid::fill_grid(ostringstream &s, boost::optional<const TRow &> row)
{
    if(row and row->size() != hdr.size())
        throw Exception("hdr not equal row: %d %d", row->size(), hdr.size());
    map< int, vector<string> > fields;
    int fields_idx = 0;
    for(const auto i: hdr) {
        const string &cell_data = (row ? (*row)[fields_idx] : i.first);
        if(cell_data.size() >= i.second) {
            vector<string> rows;
            SeparateString(cell_data.c_str(), i.second - 1, rows);
            fields[fields_idx++] = rows;
        } else
            fields[fields_idx++].push_back(cell_data);
    }
    int row_idx = 0;
    do {
        fields_idx = 0;
        if(row_idx != 0) s << endl;
        for(const auto &i: hdr) {
            s << setw(i.second) << (fields[fields_idx].empty() ? "" : *(fields[fields_idx].begin()));
            fields_idx++;
        }
        for(auto &i: fields)
            if(not i.second.empty()) i.second.erase(i.second.begin());
        row_idx++;
    } while(check_fileds_empty(fields));
}

void TTextGrid::trace_hdr(ostringstream &s)
{
    TRow row(*this);
    for(const auto &i: hdr) row.add(i.second);
    fill_grid(s, row);
    s << endl;
}

void TTextGrid::trace()
{
    LogTrace(TRACE5) << "TTextGrid::trace tab_width: " << tab_width;
    ostringstream s;
    s << left << endl;
    trace_hdr(s);
    fill_grid(s);
    s << endl;
    s << string(tab_width, '-') << endl;

    for(const auto &i: grid) {
        fill_grid(s, i);
        s << endl;
    }

    LogTrace(TRACE5) << s.str();
}
