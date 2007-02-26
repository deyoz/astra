#ifndef _BASE_TABLES_H_
#define _BASE_TABLES_H_

#include <map>

class TBaseTable {
    private:
        typedef std::map<std::string, std::string> TFields;
        typedef std::map<std::string, TFields> TTable;
        TTable table;
    public:
        virtual char *get_cache_name() = 0;
        virtual char *get_sql_text() = 0;
        virtual ~TBaseTable() {};
        std::string get(std::string code, std::string name, bool pr_lat, bool pr_except = false);
};

class TBaseTables {
    private:
        typedef std::map<std::string, TBaseTable *> TTables;
        TTables base_tables;
    public:
        TBaseTable *get_base_table(std::string name);
        void Clear();
        ~TBaseTables() { Clear(); };
};

class TAirps: public TBaseTable {
    char *get_cache_name() { return "airps"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   city "
            "from "
            "   airps";
    };
};

class TPersTypes: public TBaseTable {
    char *get_cache_name() { return "pers_types"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat "
            "from "
            "   pers_types";
    };
};

class TCities: public TBaseTable {
    char *get_cache_name() { return "cities"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   country "
            "from "
            "   cities";
    };
};

class TAirlines: public TBaseTable {
    char *get_cache_name() { return "airlines"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat, "
            "   short_name, "
            "   short_name_lat "
            "from "
            "   airlines";
    };
};

class TClasses: public TBaseTable {
    char *get_cache_name() { return "crafts"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat "
            "from "
            "   classes ";
    };
};

class TCrafts: public TBaseTable {
    char *get_cache_name() { return "crafts"; };
    char *get_sql_text()
    {
        return
            "select "
            "   code, "
            "   code_lat, "
            "   name, "
            "   name_lat "
            "from "
            "   crafts";
    };
};

extern TBaseTables base_tables;


#endif
