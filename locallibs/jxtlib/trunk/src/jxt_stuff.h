#ifndef __JXT_STUFF_H__
#define __JXT_STUFF_H__

#include <string>
#include <map>

class ILanguage
{
  private:
    std::map<int,std::string> lang_map;
    void initILangMap();
    ILanguage()
    {
      initILangMap();
    }
  public:
    static ILanguage &getILanguage();
    ~ILanguage()
    {
    }
    std::string getCode(int ida);
    int getIda(const std::string &code);
};

#endif /* __JXT_STUFF_H__ */
