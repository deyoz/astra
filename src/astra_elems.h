#ifndef _ASTRA_ELEMS_H_
#define _ASTRA_ELEMS_H_

#include <string>
#include <vector>

enum TElemType { etCountry,etCity,etAirline,etAirp,etCraft,etClass,etSubcls,
                 etPersType,etGenderType,etPaxDocType,etPayType,etCurrency,
                 etRefusalType,etSuffix,etClsGrp,etTripType,etCompElemType,
                 etGrpStatusType,etClientType,etCompLayerType,etCrs,
                 etDevModel,etDevSessType,etDevFmtType,etDevOperType,
                 etGraphStage, etMiscSetType, etDelayType, etTripLiter, etTypeBType,
                 etBagNormType };
enum TElemContext { ecDisp, ecCkin, ecTrfer, ecTlgTypeB, ecNone };

//форматы:
enum TElemFmt {efmtUnknown=-1,
               efmtCodeNative=0,     //вн. код IATA
               efmtCodeInter=1,      //меж. код IATA
               efmtCodeICAONative=2, //вн. код ICAO
               efmtCodeICAOInter=3,  //меж. код ICAO
               efmtCodeISOInter=4,   //меж. код ISO
               efmtNameLong=10,      //длинное название
               efmtNameShort=12};    //сокращенное название

const char* EncodeElemContext(const TElemContext ctxt);
const char* EncodeElemType(const TElemType type);
const char* EncodeElemFmt(const TElemFmt type);

std::string ElemToElemId(TElemType type, const std::string &elem, TElemFmt &fmt, const std::string &lang, bool with_deleted=false);
std::string ElemToElemId(TElemType type, const std::string &elem, TElemFmt &fmt, bool with_deleted=false);


//все нижеописанные функции дублируются для строкового и числового идентификаторов елемента

//на вход подается вектор пар <формат элемента, язык>
//поочередно перебираются элементы вектора до тех пор пока не найдется строковое представление элемента
//если ни одно представление не найдено, возвращается пустая строка
std::string ElemIdToElem(TElemType type, const std::string &id, const std::vector< std::pair<TElemFmt, std::string> > &fmts, bool with_deleted=true);
std::string ElemIdToElem(TElemType type, int id, const std::vector< std::pair<TElemFmt, std::string> > &fmts, bool with_deleted=true);

//если представление с параметрами формат элемента, язык не найдено, возвращается пустая строка
std::string ElemIdToElem(TElemType type, const std::string &id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);
std::string ElemIdToElem(TElemType type, int id, TElemFmt fmt, const std::string &lang, bool with_deleted=true);

//в зависимости от параметра fmt внутри функции наполняется ветор перебора (вектор пар <формат элемента, язык>)
//учитывается язык терминала
//если на языке терминала ничего не найдено, принимется язык LANG_RU
//если и на языке LANG_RU ничего не найдено, возвращается пустая строка
std::string ElemIdToClientElem(TElemType type, const std::string &id, TElemFmt fmt, bool with_deleted=true);
std::string ElemIdToClientElem(TElemType type, int id, TElemFmt fmt, bool with_deleted=true);

//вызывается ElemIdToClientElem с fmt=efmtCodeNative
std::string ElemIdToCodeNative(TElemType type, const std::string &id);
std::string ElemIdToCodeNative(TElemType type, int id);

//вызывается ElemIdToClientElem с fmt=efmtNameLong
std::string ElemIdToNameLong(TElemType type, const std::string &id);
std::string ElemIdToNameLong(TElemType type, int id);

//вызывается ElemIdToClientElem с fmt=efmtNameShort
std::string ElemIdToNameShort(TElemType type, const std::string &id);
std::string ElemIdToNameShort(TElemType type, int id);

TElemFmt prLatToElemFmt(TElemFmt fmt, bool pr_lat);

//перекодировки в контексте
std::string ElemCtxtToElemId(TElemContext ctxt,TElemType type, std::string code,
                             TElemFmt &fmt, bool hard_verify, bool with_deleted=false);
std::string ElemIdToElemCtxt(TElemContext ctxt,TElemType type, std::string id,
                             TElemFmt fmt, bool with_deleted=true);


#endif /*_ASTRA_ELEMS_H_*/
