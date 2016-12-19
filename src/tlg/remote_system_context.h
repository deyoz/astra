//
// C++ Interface: remote_system_context
//
// Description: Контекст системы, от которой пришел запрос
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
#include "iatci_settings.h"

#include <etick/lang.h>
#include <etick/tick_data.h>
#include <etick/exceptions.h>
#include <libtlg/tlgnum.h>

#include <string>
#include <map>
#include <list>
#include <time.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>


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

    class InboundTlgInfo
    {
        bool ToBePostponed;
        bool RepeatedlyProcessed;
        boost::optional<tlgnum_t> TlgNum;
        std::string TlgSrc;
    public:
        InboundTlgInfo()
            : ToBePostponed(false), RepeatedlyProcessed(false)
        {}
        void setToBePostponed() { ToBePostponed=true; }
        bool toBePostponed() const { return ToBePostponed; }

        boost::optional<tlgnum_t> tlgNum() const { return TlgNum; }
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
    /// @brief общая инфа об удаленной (далеко) системе
    class SystemContext
    {
        friend struct SystemContextMaker;

        Ticketing::SystemAddrs_t Ida;
        std::string CanonName;
        std::string Airline;
        std::string OurAddrEdifact;
        std::string RemoteAddrEdifact;
        std::string OurAddrAirimp;
        std::string RemoteAddrAirimp;
        std::string EdifactProfileName;
        boost::shared_ptr<edifact::EdifactProfile> EdiProfile;

        mutable InboundTlgInfo InbTlgInfo;

        SystemSettings CommonSettings;

    public:
        typedef boost::shared_ptr<SystemContext> pSystemContext;

    private:
        void checkContinuity() const;
        void readEdifactProfile();

    protected:
        static Ticketing::SystemAddrs_t getNextId();

        static pSystemContext SysCtxt;
    public:
        Ticketing::SystemAddrs_t ida() const          { return Ida;                }
        const std::string& canonName() const          { return CanonName;          }
        const std::string& airline() const            { return Airline;            }
        const std::string& ourAddrEdifact() const     { return OurAddrEdifact;     }
        const std::string& remoteAddrEdifact() const  { return RemoteAddrEdifact;  }
        const std::string& ourAddrAirimp() const      { return OurAddrAirimp;      }
        const std::string& remoteAddrAirimp() const   { return RemoteAddrAirimp;   }
        const std::string& edifactProfileName() const { return EdifactProfileName; }
        const std::string& routerCanonName() const    { return CanonName;          }

        edifact::EdifactProfile edifactProfile() const;

        unsigned edifactResponseTimeOut() const;
        InboundTlgInfo& inbTlgInfo() const { return InbTlgInfo; }

        void setIda(Ticketing::SystemAddrs_t ida)     { Ida = ida; }
        virtual const SystemSettings& commonSettings() const { return CommonSettings; }
        virtual SystemSettings& commonSettings() { return CommonSettings; }

        static bool initialized();

        /*
         * @brief TODO !!!
         *        Сейчас при получении входящей тлг экземпляр SystemContext
         *        необходим только для проброса текста входящей тлг, поэтому
         *        нет особого смысла читать его по edifact адресам
         *        Сделаем это в будущем.
         */
        static SystemContext* initDummyContext();

        static SystemContext* init(const SystemContext &);
        /**
         * @brief Деициализировать контекст системы
         */
        static void free();

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
    /// @brief Сервер электронных документов (ЭБ+EMD)
    class EdsSystemContext : public SystemContext
    {
        EdsSystemSettings Settings;
    public:
        EdsSystemContext(const SystemContext& baseCnt, const EdsSystemSettings &Settings = EdsSystemSettings());
        static EdsSystemContext* read(const std::string& airl, const Ticketing::FlightNum_t& flNum);

#ifdef XP_TESTING
        static EdsSystemContext* create4TestsOnly(const std::string& airline,
                                                  const std::string& ediAddr,
                                                  const std::string& ourEdiAddr,
                                                  const std::string& h2hAddr = "",
                                                  const std::string& ourH2hAddr = "");
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
    /// @brief Система регистрации (в случае iacti)
    class DcsSystemContext : public SystemContext
    {
    public:
        DcsSystemContext(const SystemContext& baseCnt);
        static DcsSystemContext* read(const std::string& airl,
                                      const Ticketing::FlightNum_t& flNum = Ticketing::FlightNum_t());

        iatci::IatciSettings iatciSettings() const;

#ifdef XP_TESTING
        static DcsSystemContext* create4TestsOnly(const std::string& airline,
                                                  const std::string& ediAddr,
                                                  const std::string& ourEdiAddr,
                                                  const std::string& airAddr = "",
                                                  const std::string& ourAirAddr = "",
                                                  const std::string& h2hAddr = "",
                                                  const std::string& ourH2hAddr = "");
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

        void setOurAddrEdifact(const std::string& val);
        void setRemoteAddrEdifact(const std::string& val);
        void setOurAddrAirimp(const std::string& val);
        void setRemoteAddrAirimp(const std::string& val);
        void setAirline(const std::string& val);
        void setIda(SystemAddrs_t val);
        void setCanonName(const std::string& canonName);
        void setEdifactProfileName(const std::string& edifactProfileName);
        void setSystemSettings(const SystemSettings& sett);

        SystemContext getSystemContext();
    };

} // namespace RemoteSystemContext

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING

    struct RotParams
    {
        std::string canon_name;
        std::string h2h_addr;
        std::string our_h2h_addr;
        bool h2h;

        RotParams(const std::string &cn) :
            canon_name(cn),
            h2h(false)
        {}


        RotParams &setH2hAddrs(const std::string& their, const std::string& our)
        {
            h2h = true;
            h2h_addr = their;
            our_h2h_addr = our;
            return *this;
        }
    };

    std::string createRot(const RotParams &par);
    std::string createIatciEdifactProfile();

#endif /*XP_TESTING*/

} // namespace Ticketing
#endif /*_REMOTE_SYSTEM_CONTEXT_H_*/
