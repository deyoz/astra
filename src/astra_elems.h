#ifndef _ASTRA_ELEMS_H_
#define _ASTRA_ELEMS_H_

#include <set>
#include <string>
#include <vector>
#include "base_tables.h"

enum TElemType {
                 etAgency,                   //�����᪨� ������⢠
                 etAirline,                  //������������
                 etAirp,                     //��ய����
                 etAirpTerminal,             //�ନ���� ��ய��⮢
                 etAlarmType,                //⨯� �ॢ��
                 etBagNormType,              //⨯� �������� ���
                 etBagType,                  //⨯� ������
                 etBIHall,                   //������-����
                 etBIPrintType,              //⨯� ���� �ਣ��襭��
                 etBIType,                   //⨯� �ਣ��襭��
                 etBrand,                    //�७��
                 etBPType,                   //⨯� ��ᠤ���� ⠫����
                 etBTType,                   //⨯� �������� ��ப
                 etCity,                     //��த�
                 etCkinRemType,              //���� ६�ப ॣ����樨
                 etClass,                    //������ ������
                 etClientType,               //⨯� �����⮢ (�ନ���, ���, ����)
                 etClsGrp,                   //��㯯� �������ᮢ
                 etCompLayerType,            //⨯� ᫮�� � ����������
                 etCompElemType,             //⨯� ����⮢ � ����������
                 etCountry,                  //���㤠��⢠
                 etCraft,                    //⨯ �� (�����譮� �㤭�)
                 etCurrency,                 //���� �����
                 etDCSServiceType,           //⨯� ��� � ����
                 etDelayType,                //���� ����থ� ३ᮢ
                 etDeskGrp,                  //��㯯� ���⮢
                 etDevFmtType,               //�ଠ�� ���ன��
                 etDevModel,                 //������ ���ன��
                 etDevOperType,              //⨯� ����権 ���ன��
                 etDevSessType,              //⨯� ����䥩ᮢ ���ன��
                 etExtendedPersType,         //���७�� ⨯ ���ᠦ�� ��, ��, ��, ��
                 etGenderType,               //��� ���ᠦ�஢
                 etGraphStage,               //�⠯� �孮�����᪮�� ��䨪�
                 etGraphStageWOInactive,     //�⠯� �孮�����᪮�� ��䨪� ��� ����� "����⨢��"
                 etGrpStatusType,            //����� ��㯯� ���ᠦ�஢
                 etHall,                     //���� ॣ����樨 � ��ᠤ��
                 etHotel,                    //�������� ���⨭��
                 etHotelRoomType,            //⨯� ����஢ � ���⨭���
                 etKiosksGrp,                //��㯯� ���᪮�
                 etLangType,                 //�모
                 etMiscSetType,              //⨯� ����஥� ��� ३ᮢ
                 etMsgTransport,             //⨯� �࠭ᯮ�⮢ ��� ᮮ�饭��
                 etPaxDocCountry,            //���� ���㤠��� � ���㬥��� ���ᠦ�஢
                 etPaxDocSubtype,            //���⨯� ���㬥�⮢ ���ᠦ�஢
                 etPaxDocType,               //⨯� ���㬥�⮢ ���ᠦ�஢
                 etPayType,                  //��� ⨯� ������
                 etPersType,                 //⨯ ���ᠦ�� ��, ��, ��
                 etProfiles,                 //��䨫� ��
                 etRateColor,                //梥� ��䮢 � ����������
                 etRcptDocType,              //⨯� ���㬥�⮢ ���ᠦ�஢ ��� ������ ������
                 etRefusalType,              //���� ��稭 �⪠�� � ॣ����樨
                 etReportType,               //⨯� ���⮢
                 etRemGrp,                   //��㯯� ६�ப
                 etRight,                    //�����䨪���� �ࠢ ���짮��⥫��
                 etRoles,                    //஫� ���짮��⥫��
                 etSalePoint,                //���� �㭪⮢ �த��
                 etSeasonType,               //����/���
                 etSeatAlgoType,             //⨯ ��ᠤ�� � ᠫ��� ��
                 etStationMode,              //०�� �ନ���� (ॣ������/��ᠤ��)
                 etSubcls,                   //�������� �஭�஢����
                 etSuffix,                   //���䨪�� ३ᮢ
                 etTagColor,                 //梥� �������� ��ப
                 etTripLiter,                //����� ३ᮢ
                 etTripType,                 //⨯� ३ᮢ
                 etTypeBOptionValue,         //⨯� ��ࠬ��஢ typeb-⥫��ࠬ�
                 etTypeBSender,              //業��� �஭�஢����
                 etTypeBType,                //⨯� typeb ⥫��ࠬ�
                 etUsers,                    //���짮��⥫�
                 etUserSetType,              //⨯� ���짮��⥫�᪨� ����஥�
                 etUserType,                 //⨯� ���짮��⥫��
                 etValidatorType,            //⨯� �������஢
                 etVoucherType               //������
               };

enum TElemContext { ecDisp, ecCkin, ecTrfer, ecTlgTypeB, ecNone };

//�ଠ��:
enum TElemFmt {efmtUnknown=-1,
               efmtCodeNative=0,     //��. ��� IATA
               efmtCodeInter=1,      //���. ��� IATA
               efmtCodeICAONative=2, //��. ��� ICAO
               efmtCodeICAOInter=3,  //���. ��� ICAO
               efmtCodeISOInter=4,   //���. ��� ISO
               efmtNameLong=10,      //������� ��������
               efmtNameShort=12};    //᮪�饭��� ��������

const char* EncodeElemContext(const TElemContext ctxt);
const char* EncodeElemType(const TElemType type);
const char* EncodeElemFmt(const TElemFmt type);

std::string ElemToElemId(TElemType type, const std::string &elem, TElemFmt &fmt, const std::string &lang, bool with_deleted=false);
std::string ElemToElemId(TElemType type, const std::string &elem, TElemFmt &fmt, bool with_deleted=false);


//�� �������ᠭ�� �㭪樨 �㡫������� ��� ��ப����� � �᫮���� �����䨪��஢ �������

//�� �室 �������� ����� ��� <�ଠ� �����, ��>
//����।�� ��ॡ������ ������ ����� �� �� ��� ���� �� �������� ��ப���� �।�⠢����� �����
//�᫨ �� ���� �।�⠢����� �� �������, �����頥��� ����� ��ப�
std::string ElemIdToElem(TElemType type, const std::string &id, const std::vector< std::pair<TElemFmt, std::string> > &fmts, bool with_deleted=true);
std::string ElemIdToElem(TElemType type, int id, const std::vector< std::pair<TElemFmt, std::string> > &fmts, bool with_deleted=true);

//�᫨ �।�⠢����� � ��ࠬ��ࠬ� �ଠ� �����, �� �� �������, �����頥��� ����� ��ப�
std::string ElemIdToElem(TElemType type, const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);
std::string ElemIdToElem(TElemType type, int id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);

//� ����ᨬ��� �� ��ࠬ��� fmt ����� �㭪樨 ���������� ���� ��ॡ�� (����� ��� <�ଠ� �����, ��>)
//���뢠���� �� �ନ����
//�᫨ �� �몥 �ନ���� ��祣� �� �������, �ਭ������ �� LANG_RU
//�᫨ � �� �몥 LANG_RU ��祣� �� �������, �����頥��� ����� ��ப�
std::string ElemIdToClientElem(TElemType type, const std::string &id, TElemFmt fmt, bool with_deleted=true);
std::string ElemIdToClientElem(TElemType type, int id, TElemFmt fmt, bool with_deleted=true);
std::string ElemIdToPrefferedElem(TElemType type, const int &id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);
std::string ElemIdToPrefferedElem(TElemType type, const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);

//��뢠���� ElemIdToClientElem � fmt=efmtCodeNative
std::string ElemIdToCodeNative(TElemType type, const std::string &id);
std::string ElemIdToCodeNative(TElemType type, int id);

//��뢠���� ElemIdToClientElem � fmt=efmtNameLong
std::string ElemIdToNameLong(TElemType type, const std::string &id);
std::string ElemIdToNameLong(TElemType type, int id);

//��뢠���� ElemIdToClientElem � fmt=efmtNameShort
std::string ElemIdToNameShort(TElemType type, const std::string &id);
std::string ElemIdToNameShort(TElemType type, int id);

TElemFmt prLatToElemFmt(TElemFmt fmt, bool pr_lat);

//��४���஢�� � ���⥪��
std::string ElemCtxtToElemId(TElemContext ctxt,TElemType type, std::string code,
                             TElemFmt &fmt, bool hard_verify, bool with_deleted=false);
std::string ElemIdToElemCtxt(TElemContext ctxt,TElemType type, std::string id,
                             TElemFmt fmt, bool with_deleted=true);

void getElemFmts(TElemFmt fmt, std::string basic_lang, std::vector< std::pair<TElemFmt,std::string> > &fmts);

TBaseTable& getBaseTable(TElemType type);

std::string ElemToPaxDocCountryId(const std::string &elem, TElemFmt &fmt);
std::string PaxDocCountryIdToPrefferedElem(const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);
std::string classIdsToCodeNative(const std::string &orig_cl,
                                 const std::string &cabin_cl);
std::string clsGrpIdsToCodeNative(const int orig_cl_grp,
                                  const int cabin_cl_grp);

namespace AstraLocale
{

class OutputLang
{
  private:
    std::string _lang;
    bool _onlyTrueIATACodes;
  public:
    enum Props {OnlyTrueIataCodes};

    explicit OutputLang(const std::string& lang="");
    explicit OutputLang(const std::string& lang,
                        const std::set<Props>& props);
    const std::string& get() const { return _lang; }
    bool isLatin() const { return _lang!=LANG_RU; }
    bool onlyTrueIATACodes() const { return _onlyTrueIATACodes; }
};

} //namespace AstraLocale

std::string airlineToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang);
std::string airpToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang);
std::string craftToPrefferedCode(const std::string &code, const AstraLocale::OutputLang& lang);

std::string getElemId(TElemType type, const std::string &elem);

#endif /*_ASTRA_ELEMS_H_*/
