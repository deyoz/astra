#include "base_tables.h"

#include <string>
#include "oralib.h"
#include "exceptions.h"
#include "stl_utils.h"

using namespace std;
using namespace EXCEPTIONS;

void TBaseTables::Clear()
{
    for(TTables::iterator ti = base_tables.begin(); ti != base_tables.end(); ti++)
        delete ti->second;
    base_tables.clear();
}

TBaseTable &TBaseTables::get(string name)
{
    name = upperc(name);
    TTables::iterator ti = base_tables.find(name);
    if(ti == base_tables.end()) {
        if(name == "AIRPS")
            base_tables[name] = new TAirps();
        else if(name == "PERS_TYPES")
            base_tables[name] = new TPersTypes();
        else if(name == "CITIES")
            base_tables[name] = new TCities();
        else if(name == "AIRLINES")
            base_tables[name] = new TAirlines();
        else if(name == "CLASSES")
            base_tables[name] = new TClasses();
        else if(name == "CRAFTS")
            base_tables[name] = new TCrafts();
        else
            throw Exception("TBaseTables::get_base_table: " + name + " not found");
    }
    return *(base_tables[name]);
}

string TBaseTable::get(string code, string name, bool pr_lat, bool pr_except)
{
    if(table.empty()) {
        TQuery Qry(&OraSession);
        Qry.SQLText = get_sql_text();
        Qry.Execute();
        if(Qry.Eof) throw Exception("TBaseTable::get: table is empty");
        while(!Qry.Eof) {
            TFields fields;
            for(int i = 0; i < Qry.FieldsCount(); i++)
                fields[Qry.FieldName(i)] = Qry.FieldAsString(i);
            table[Qry.FieldAsString(0)] = fields;
            Qry.Next();
        }
    }
    name = upperc(name);
    TTable::iterator ti = table.find(code);
    if(ti == table.end())
        throw Exception((string)"TBaseTable::get data not found in " + get_cache_name() + " for code " + code);
    TFields::iterator fi = ti->second.find(name);
    if(fi == ti->second.end())
        throw Exception("TBaseTable::get: field " + name + " not found for " + get_cache_name());
    if(pr_lat) {
        TFields::iterator fi_lat = ti->second.find(name + "_LAT");
        if(fi_lat != ti->second.end())
            fi = fi_lat;
        else if(pr_except)
            throw Exception("TBaseTable::get: field " + name + "_LAT not found for " + get_cache_name());
    }
    return fi->second;
}

TBaseTables base_tables;
