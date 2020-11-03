#pragma once
#include "edilib_db_callbacks.h"

namespace PgCpp {
  namespace details {
    class SessionDescription;
  }
}

namespace edilib
{

class EdilibPgCallbacks: public EdilibDbCallbacks
{
    PgCpp::details::SessionDescription *sd;
public:
    EdilibPgCallbacks(PgCpp::details::SessionDescription *sd) : sd(sd) {}
    virtual edilib::EdiSessionId_t ediSessionNextIda() const override;
    virtual unsigned ediSessionNextEdiId() const override;
    virtual EdiSession::ReadResult ediSessionReadByCarf(const std::string &ourcarf,
                                                        const std::string &othcarf, bool update,
                                                        int times_to_wait) const override;
    virtual EdiSession::ReadResult ediSessionReadByIda(edilib::EdiSessionId_t sess_ida, bool update,
                                                       int times_to_wait) const override;
    virtual EdiSession::ReadResult ediSessionReadByKey(const EdiSessionSearchKey &key, bool update) const override;
    virtual void ediSessionReadByKey(std::list<EdiSession> &ledisession,
                                     const std::string &key, const std::string &sessionType,
                                     bool update) const override;
    virtual void ediSessionWriteDb(EdiSession &) const override;
    virtual void ediSessionUpdateDb(EdiSession &edisession) const override;
    virtual void ediSessionDeleteDb(edilib::EdiSessionId_t Id) const override;
    virtual bool ediSessionIsExists(edilib::EdiSessionId_t sess_ida) const override;

    // EdiSessionTimeOut
    virtual void ediSessionToWriteDb(const EdiSessionTimeOut &edisess_to) const override;
    virtual EdiSessionTimeOut ediSessionToReadById(EdiSessionId_t id) const override;
    virtual bool ediSessionToIsExists(EdiSessionId_t Id) const override;
    virtual void ediSessionToReadExpired(std::list<EdiSessionTimeOut> & lExpired) const override;
    virtual void ediSessionToDeleteDb(EdiSessionId_t sessid) const override;

    // write edi message tables
    virtual void clearMesTableData() const override;
    virtual void insertMesTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertMesStrTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertSegTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertSegStrTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertCompTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertCompStrTableData(_COMMAND_STRUCT_ *pCommStr) const override;
    virtual void insertDataElemTableData(_COMMAND_STRUCT_ *pCommStr) const override;

    // read edi message tables
    virtual void readMesTableData(std::vector<_MESSAGE_TABLE_STRUCT_> &vMesTable) const override;
    virtual void readMesStrTableData(std::vector<_MES_STRUCT_TABLE_STRUCT_> &vMesStrTable) const override;
    virtual void readSegStrTableData(std::vector<_SEG_STRUCT_TABLE_STRUCT_> &vSegStrTable) const override;
    virtual void readCompStrTableData(std::vector<_COMP_STRUCT_TABLE_STRUCT_> &vCompStrTable) const override;
    virtual void readDataElemTableData(std::vector<_DATA_ELEM_TABLE_STRUCT_> &vDataTable) const override;
    virtual void readSegTableData(std::vector<_SEGMENT_TABLE_STRUCT_> &vSegTable) const override;
    virtual void readCompTableData(std::vector<_COMPOSITE_TABLE_STRUCT_> &vCompTable) const override;

    virtual void ediSessionRcWriteDb(const EdiSessionRc &edisessRc) const override;
    virtual void ediSessionRcDeleteDb(const EdiSessionId_t& sessId) const override;
    virtual bool ediSessionRcIsExists( const EdiSessionId_t& sessId ) const override;
    virtual EdiSessionRc ediSessionRcReadById(const EdiSessionId_t& sessId) const override;
    virtual void ediSessionRcReadExpired(std::list< EdiSessionRc >& lExpired) const override;

    virtual void commit() const override;
    virtual void rollback() const override;
};

} // namespace edilib
