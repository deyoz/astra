#include "cr_lf.h"

using namespace std;

void clean_ending(string &data, const string &end)
{
    if(not data.empty()) {
        while(true) { // удалим все пробелы, TAB и CR/LF из конца строки
            size_t last_ch = data.size() - 1;
            if(
                    data[last_ch] == CR[0] or
                    data[last_ch] == LF[0] or
                    data[last_ch] == TAB[0] or
                    data[last_ch] == ' '
              )
                data.erase(last_ch);
            else
                break;
        }
        //и добавим один CR/LF
        data += end;
    }
}

string place_CR_LF(string data)
{
    size_t pos = 0;
    while(true) {
        pos = data.find(LF, pos);
        if(pos == string::npos)
            break;
        else {
            if(pos == 0 or data[pos - 1] != CR[0]) {
                data.replace(pos, 1, CR + LF);
                pos += 2;
            } else
                pos += 1;
        }
    }
    clean_ending(data, CR + LF);
    return data;
}
