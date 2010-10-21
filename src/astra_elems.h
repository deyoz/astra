#ifndef _ASTRA_ELEMS_H_
#define _ASTRA_ELEMS_H_

#include <string>
#include <vector>

enum TElemType { etCountry,etCity,etAirline,etAirp,etCraft,etClass,etSubcls,
                 etPersType,etGenderType,etPaxDocType,etPayType,etCurrency,
                 etRefusalType,etSuffix,etClsGrp,etTripType,etCompElemType,
                 etGrpStatusType,etClientType,etCompLayerType,etCrs,etHall,
                 etDevModel,etDevSessType,etDevFmtType,etDevOperType,
                 etGraphStage, etMiscSetType, etDelayType, etTripLiter, etTypeBType,
                 etBagNormType, etLangType, etTagColor };
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

#endif /*_ASTRA_ELEMS_H_*/
