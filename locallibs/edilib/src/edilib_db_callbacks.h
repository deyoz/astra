#pragma once
#include "edilib/edi_session.h"
#include "edilib/EdiSessionTimeOut.h"


struct _MESSAGE_TABLE_STRUCT_;
struct _MES_STRUCT_TABLE_STRUCT_;
struct _SEG_STRUCT_TABLE_STRUCT_;
struct _COMP_STRUCT_TABLE_STRUCT_;
struct _SEGMENT_TABLE_STRUCT_;
struct _DATA_ELEM_TABLE_STRUCT_;
struct _COMPOSITE_TABLE_STRUCT_;
struct _COMMAND_STRUCT_;

namespace edilib
{

class EdiSessionRc;

class EdilibDbCallbacks {
    static const EdilibDbCallbacks *_instance;
public:
    static const int max_wait_session_times = 10; /*10 times*/
    static const int usec_sleep_4_sess  = 100000; /*100ms*/

    static const EdilibDbCallbacks *instance();
    static void setEdilibDbCallbacks(const EdilibDbCallbacks *);

    virtual void ediSessionWriteDb(EdiSession &) const = 0;
    virtual edilib::EdiSessionId_t ediSessionNextIda() const = 0;
    virtual unsigned ediSessionNextEdiId() const = 0;
    virtual EdiSession::ReadResult ediSessionReadByCarf(const std::string &ourcarf,
                                                        const std::string &othcarf, bool update,
                                                        int times_to_wait) const = 0;
    virtual EdiSession ediSessionReadByCarf(const std::string &ourcarf,
                                            const std::string &othcarf, bool update) const;

    virtual EdiSession::ReadResult ediSessionReadByIda(edilib::EdiSessionId_t sess_ida, bool update,
                                                       int times_to_wait) const = 0;
    virtual EdiSession ediSessionReadByIda(edilib::EdiSessionId_t sess_ida, bool update) const;

    virtual EdiSession::ReadResult ediSessionReadByKey(const EdiSessionSearchKey &key, bool update) const = 0;
    virtual void ediSessionReadByKey(std::list<EdiSession> &ledisession,
                                     const std::string &key, const std::string &sessionType,
                                     bool update) const = 0;
    virtual void ediSessionUpdateDb(EdiSession &edisession) const = 0;
    virtual void ediSessionDeleteDb(edilib::EdiSessionId_t Id) const = 0;
    virtual bool ediSessionIsExists(edilib::EdiSessionId_t sess_ida) const = 0;

    virtual void ediSessionToWriteDb(const EdiSessionTimeOut &edisess_to) const = 0;
    virtual EdiSessionTimeOut ediSessionToReadById(EdiSessionId_t id) const = 0;
    virtual bool ediSessionToIsExists(EdiSessionId_t Id) const = 0;
    virtual void ediSessionToReadExpired(std::list<EdiSessionTimeOut> & lExpired) const = 0;
    virtual void ediSessionToDeleteDb(EdiSessionId_t sessid) const = 0;

    virtual void clearMesTableData() const = 0;
    virtual void insertMesTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertMesStrTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertSegTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertSegStrTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertCompTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertCompStrTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;
    virtual void insertDataElemTableData(_COMMAND_STRUCT_ *pCommStr) const = 0;

    virtual void readMesTableData(std::vector<_MESSAGE_TABLE_STRUCT_> &vMesTable) const = 0;
    virtual void readMesStrTableData(std::vector<_MES_STRUCT_TABLE_STRUCT_> &vMesStrTable) const = 0;
    virtual void readSegStrTableData(std::vector<_SEG_STRUCT_TABLE_STRUCT_> &vSegStrTable) const = 0;
    virtual void readCompStrTableData(std::vector<_COMP_STRUCT_TABLE_STRUCT_> &vCompStrTable) const = 0;
    virtual void readDataElemTableData(std::vector<_DATA_ELEM_TABLE_STRUCT_> &vDataTable) const = 0;
    virtual void readSegTableData(std::vector<_SEGMENT_TABLE_STRUCT_> &vSegTable) const = 0;
    virtual void readCompTableData(std::vector<_COMPOSITE_TABLE_STRUCT_> &vCompTable) const = 0;

    virtual void ediSessionRcWriteDb(const EdiSessionRc &edisessRc) const = 0;
    virtual void ediSessionRcDeleteDb(const EdiSessionId_t& sessId) const = 0;
    virtual bool ediSessionRcIsExists( const EdiSessionId_t& sessId ) const = 0;
    virtual EdiSessionRc ediSessionRcReadById(const EdiSessionId_t& sessId) const = 0;
    virtual void ediSessionRcReadExpired(std::list< EdiSessionRc >& lExpired) const = 0;

    virtual void commit() const = 0;
    virtual void rollback() const = 0;

    virtual ~EdilibDbCallbacks(){};
};

} // namespace edilib
