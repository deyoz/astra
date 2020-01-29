#ifndef __FUNC_PLACEHOLDERS_H
#define __FUNC_PLACEHOLDERS_H

#include <string>
#include <vector>
#include <map>

/*******************************************************************************
 * ����� � ᯨ᪮� ��ࠬ��஢
 ******************************************************************************/

namespace tok {

    struct Param {
        std::string name; /* ����� ���� ����� */
        std::string value; /* ����� ���� ����� */

        Param() {}
        Param(const std::string& name, const std::string& value): name(name), value(value) {}
    };

    std::string GetValue(
            const std::vector<Param>& params,
            const std::string& name,
            const std::string& defaultValue = std::string());

    std::string GetValue_ThrowUndefined(
            const std::vector<Param>& params,
            const std::string& name);

    std::string Validate(const std::string& value, const std::string& validValues);

    void ValidateParams(
            const std::vector<Param>& params,
            size_t minPositional,
            size_t maxPositional,
            const std::string& validNames);

    /* ���祭�� ������������� ��ࠬ��஢ � ���浪� ᫥������� */
    std::vector<std::string> PositionalValues(const std::vector<Param>& params);
    /* �����頥� ᯨ᮪ ���祭�� � �஢�મ� �� �� ����� ����� */
    std::vector<std::string> ForcePositionalValues(const std::vector<Param>& params);

} /* namespace tok */


/*******************************************************************************
 * ��堭��� ॣ����樨 ����� �㭪権
 ******************************************************************************/

/* ���쪮 ����樮��� (������������) ��ࠬ���� */
typedef std::string (*FuncPh1_t)(const std::vector<std::string>& p);
/* ���������� � ����樮��� ��ࠬ���� */
typedef std::string (*FuncPh2_t)(const std::vector<tok::Param>& p);

struct FuncPhProc {
    FuncPh1_t type1;
    FuncPh2_t type2;

    FuncPhProc(FuncPh1_t proc): type1(proc), type2(NULL) {}
    FuncPhProc(FuncPh2_t proc): type1(NULL), type2(proc) {}
};

typedef std::map<std::string, FuncPhProc> FuncPhMap_t;
FuncPhMap_t& GetFuncPhMap();
void RegisterFuncPh(const std::string& name, FuncPhProc proc);

#define FP_REGISTRATOR_MKNAME_INTERNAL(a, b) a##b
#define FP_REGISTRATOR_MKNAME(a, b) FP_REGISTRATOR_MKNAME_INTERNAL(a, b)

#define FP_REGISTER(name_, proc_)\
    namespace {\
        struct FP_REGISTRATOR_MKNAME(FP_Registrator, __LINE__) {\
            FP_REGISTRATOR_MKNAME(FP_Registrator, __LINE__)(const std::string& name, FuncPhProc proc)\
            {\
                RegisterFuncPh(name, proc);\
            }\
        } FP_REGISTRATOR_MKNAME(_Registrator, __LINE__)(name_, proc_);\
    } /* end of unnamed namespace */

#endif /* #ifndef __FUNC_PLACEHOLDERS_H */
