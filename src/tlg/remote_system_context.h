//
// C++ Interface: remote_system_context
//
// Description: ���⥪�� ��⥬�, �� ���ன ��襫 �����
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//

#ifndef _REMOTE_SYSTEM_CONTEXT_H_
#define _REMOTE_SYSTEM_CONTEXT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /*HAVE_CONFIG_H*/

#include "exceptions.h"
#include "CheckinBaseTypes.h"
#include "EdifactProfile.h"

#include <string>
#include <map>
#include <list>
#include <time.h>
#include <boost/shared_ptr.hpp>

#include <etick/lang.h>
#include <etick/tick_data.h>
#include <etick/exceptions.h>
#include <libtlg/tlgnum.h>


namespace Ticketing
{

namespace RemoteSystemContext
{
    class system_not_found
    {
      std::string m_airline;
      Ticketing::FlightNum_t m_flNum;
      public:
        system_not_found(const std::string &v_airline, const Ticketing::FlightNum_t &v_flNum) : m_airline(v_airline), m_flNum(v_flNum) {}
        const std::string &airline() const { return m_airline; }
        const Ticketing::FlightNum_t &flNum() const { return m_flNum; }
    };

    class UnknownSystAddrs: public EXCEPTIONS::Exception
    {
        std::string Src;
        std::string Dest;
    public:
        UnknownSystAddrs(const std::string &src, const std::string &dest);

        const std::string &src() const { return Src; }
        const std::string &dest() const { return Dest; }

        ~UnknownSystAddrs() throw() {}
    };

//---------------------------------------------------------------------------------------

    class DuplicateRecord: public EXCEPTIONS::Exception
    {
    public:
        DuplicateRecord(): Exception("RemoteSystemContext: Duplicate record") {}
    };

//---------------------------------------------------------------------------------------

    class InvalidSystemTypeCast: public EXCEPTIONS::Exception
    {
    public:
        InvalidSystemTypeCast(): Exception("Invalid System Type Cast") {}
    };


//---------------------------------------------------------------------------------------

    class DuplicateAirportDcsConf: public EXCEPTIONS::Exception
    {
    public:
        DuplicateAirportDcsConf():Exception("DcsSystemContext: Diplicate Dcs configuration") {}
    };

//---------------------------------------------------------------------------------------

    class DcsSystemNotFound : public EXCEPTIONS::Exception
    {
    public:
        DcsSystemNotFound():Exception("DcsSystemContext: Record not found"){}
    };

//---------------------------------------------------------------------------------------

    class EdsSystemContext;

    class InboundTlgInfo
    {
        bool ToBePostponed;
        bool RepeatedlyProcessed;
        tlgnum_t TlgNum;
        std::string TlgSrc;
    public:
        InboundTlgInfo()
            : ToBePostponed(false), RepeatedlyProcessed(false)
        {}
        void setToBePostponed() { ToBePostponed=true; }
        bool toBePostponed() const { return ToBePostponed; }

        const tlgnum_t& tlgNum() const { return TlgNum; }
        void setTlgNum(const tlgnum_t& tnum) { TlgNum = tnum; }

        const std::string& tlgSrc() const { return TlgSrc; }
        void setTlgSrc(const std::string& tlgSrc) { TlgSrc = tlgSrc; }

        void setRepeatedlyProcessed() { RepeatedlyProcessed=true; }
        bool repeatedlyProcessed() const { return RepeatedlyProcessed; }
    };

//---------------------------------------------------------------------------------------

    class SystemSettings
    {
    public:
        SystemSettings() {}

        virtual ~SystemSettings() {}
    };

//---------------------------------------------------------------------------------------

    /// @class SystemContext
    /// @brief ���� ��� �� 㤠������ (������) ��⥬�
    class SystemContext
    {
        friend struct SystemContextMaker;

        Ticketing::SystemAddrs_t Ida;
        std::string Airline;
        std::string OurAddrEdifact;
        std::string RemoteAddrEdifact;

        mutable InboundTlgInfo InbTlgInfo;

        SystemSettings CommonSettings;

    private:
        void checkContinuity() const;

    protected:
        static Ticketing::SystemAddrs_t getNextId();


        static boost::shared_ptr<SystemContext> SysCtxt;
    public:
        Ticketing::SystemAddrs_t ida() const { return Ida; }
        const std::string& airline() const { return Airline; }
        const std::string& ourAddrEdifact() const { return OurAddrEdifact; }
        const std::string& remoteAddrEdifact() const { return RemoteAddrEdifact; }
        std::string routerCanonName() const;
        unsigned edifactResponseTimeOut() const;
        InboundTlgInfo& inbTlgInfo() const { return InbTlgInfo; }
        virtual const SystemSettings& commonSettings() const { return CommonSettings; }
        virtual SystemSettings& commonSettings() { return CommonSettings; }

        static bool initialized();

        /*
         * @brief TODO !!!
         *        ����� �� ����祭�� �室�饩 ⫣ ������� SystemContext
         *        ����室�� ⮫쪮 ��� �஡�� ⥪�� �室�饩 ⫣, ���⮬�
         *        ��� �ᮡ��� ��᫠ ���� ��� �� edifact ���ᠬ
         *        ������� �� � ���饬.
         */
        static SystemContext* initDummyContext();

        static SystemContext* init(const SystemContext &);
        /**
         * @brief ���樠����஢��� ���⥪�� ��⥬�
         */
        static void free();

        static SystemContext readById(Ticketing::SystemAddrs_t Id);

        static const SystemContext& Instance(const char *nick, const char *file, unsigned line);

        virtual void deleteDb();
        virtual void addDb();
        virtual void updateDb();

    public:
        SystemContext() {}
        virtual ~SystemContext() {}
    };


//---------------------------------------------------------------------------------------

    class EdsSystemContext;

    typedef boost::shared_ptr<EdsSystemContext> pEtsSystemContext;

    /**
     * @class EdsSystemSettings
     * @brief EDS system settings
    */
    class EdsSystemSettings : public SystemSettings
    {
        friend class EdsSystemContext;
    public:
        EdsSystemSettings() {}
        EdsSystemSettings(const SystemSettings& sett,
                          const EdsSystemSettings& ets_sett)
            : SystemSettings(sett)
        {
        }

        virtual ~EdsSystemSettings() {}
    };

//---------------------------------------------------------------------------------------

    /// @class EdsSystemContext
    /// @brief ��ࢥ� ���஭��� ���㬥�⮢ (��+EMD)
    class EdsSystemContext : public SystemContext
    {
        EdsSystemSettings Settings;
    public:
        EdsSystemContext(const SystemContext& baseCnt, const EdsSystemSettings &Settings = EdsSystemSettings());
        static EdsSystemContext* read(const std::string& airl, const Ticketing::FlightNum_t& flNum);

#ifdef XP_TESTING
        static EdsSystemContext* create4TestsOnly(const std::string& airline,
                                                  const std::string& ediAddr,
                                                  const std::string& ourEdiAddr);
#endif /*XP_TESTING*/

        /**
         * @brief Ets settings
         * @return
         */
        const EdsSystemSettings &settings() const { return Settings; }

        /**
         * @brief settings for all types of systems
         * @return
         */
        virtual const SystemSettings &commonSettings() const { return Settings; }

        virtual void deleteDb();
        virtual void addDb();
        virtual void updateDb();

        virtual ~EdsSystemContext() {}
    };

//---------------------------------------------------------------------------------------

    /// @class DcsSystemContext
    /// @brief ���⥬� ॣ����樨 (� ��砥 iacti)
    class DcsSystemContext : public SystemContext
    {
    public:
        DcsSystemContext(const SystemContext& baseCnt);
        static DcsSystemContext* read(const std::string& airl,
                                      const Ticketing::FlightNum_t& flNum = Ticketing::FlightNum_t());

#ifdef XP_TESTING
        static DcsSystemContext* create4TestsOnly(const std::string& airline,
                                                  const std::string& ediAddr,
                                                  const std::string& ourEdiAddr);
#endif /*XP_TESTING*/

        virtual void deleteDb();
        virtual void addDb();
        virtual void updateDb();

        virtual ~DcsSystemContext() {}
    };

//---------------------------------------------------------------------------------------

    class SystemContextMaker
    {
        SystemContext cont;
    public:
        SystemContextMaker() {}
        SystemContextMaker(const SystemContext &s)
            : cont(s)
        {}

        void setOurAddrEdifact(const std::string &val);
        void setRemoteAddrEdifact(const std::string &val);
        void setAirline(const std::string& val);
        void setIda(SystemAddrs_t val);
        void setSystemSettings(const SystemSettings &sett);

        SystemContext getSystemContext();
    };

} // namespace RemoteSystemContext
} // namespace Ticketing
#endif /*_REMOTE_SYSTEM_CONTEXT_H_*/
