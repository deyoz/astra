#ifndef __JXT_CONT_IMPL_H__
#define __JXT_CONT_IMPL_H__
#ifdef __cplusplus
#include <string>
#include <vector>
#include "jxt_cont.h"

namespace JxtContext
{

class JxtContHandlerSir : public JxtContHandler
{
  private:
    virtual JxtCont *createContext(int handle);
    virtual void deleteSavedCtxt(int handle);
    virtual int getNumbersOfSaved(std::vector<int> &contexts_vec) const;
  public:
    virtual bool isSavedCtxt(int handle) const;
    explicit JxtContHandlerSir(const std::string &pult) :
                               JxtContHandler(pult) {}
};

class JxtContSir : public JxtCont
{
  private:
    virtual bool isSavedRow(const std::string &name) override;
    virtual const std::string readSavedRow(const std::string &name) override;
    virtual void RemoveLikeL(const std::string &key) override;
    static const int PageSize=2000;
    static const int RowNameLength=50;
    //void checkRowName(const std::string &name);
    void saveRow(const JxtContRow *row);
    void addRow(const JxtContRow *row);
    void deleteRow(const JxtContRow *row);
  public:
    virtual ~JxtContSir() = default;
    explicit JxtContSir(const std::string &pult, int hnd,
                        const JxtContStatus &stat=UNCHANGED) :
                        JxtCont(pult,hnd) {}
    virtual void Save() override;
    virtual void readAll() override;
    virtual void dropAll() override;
};

} // namespace JxtContext

#endif /* __cplusplus */
#endif /* __JXT_CONT_IMPL_H__ */
