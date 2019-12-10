#ifndef _DOCS_TEXT_GRID_H_
#define _DOCS_TEXT_GRID_H_

#include "astra_consts.h"
#include <libxml/tree.h>

struct TTextGrid {
    private:
        size_t tab_width;

        std::list<std::pair<std::string, size_t>> hdr;

        struct TRow:public std::vector<std::string> {
            private:
                TTextGrid &grd;
            public:
            TRow &add(int data);
            TRow &add(const std::string &data);
            void toXML(xmlNodePtr rowNode);
            TRow(TTextGrid &grd): grd(grd) {}
        };

        std::list<TRow> grid;

        bool check_fileds_empty(const std::map<int, std::vector<std::string>> &fields);
        void fill_grid(std::ostringstream &s, boost::optional<const TRow &> row = boost::none);
        void trace_hdr(std::ostringstream &s);

    public:
        void headerToXML(xmlNodePtr variablesNode);
        void addCol(const std::string &caption, int length = ASTRA::NoExists);
        TRow &addRow();
        void trace();
        void clear()
        {
            tab_width = 0;
            hdr.clear();
            grid.clear();
        }
        TTextGrid() { clear(); }
};

#endif
