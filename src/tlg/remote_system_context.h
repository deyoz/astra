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

#include <string>
#include <boost/shared_ptr.hpp>
#include <map>
#include <list>
#include <time.h>

// #include "dates.h"
#include "etick/lang.h"
#include "exceptions.h"
#include "etick/exceptions.h"
// #include "tlg_types.h"
#include "CheckinBaseTypes.h"
// #include "etick/etick_msg.h"
#include "etick/tick_data.h"
#include "etick/common_codes.h"
#include "EdifactProfile.h"
//#include "tlg_source_edifact.h"
// #include "optionals.h"
// #include "typeb/typeb_preprocessor.h"

//namespace OciCpp{
//class CursCtl;
//}

namespace Ticketing
{

    class UnknownSystAddrs : public EXCEPTIONS::Exception
    {
        std::string Src;
        std::string Dest;
    public:
        UnknownSystAddrs(const std::string &src, const std::string &dest);

        const std::string &src() const
        {
            return Src;
        }
        const std::string &dest() const
        {
            return Dest;
        }
        ~UnknownSystAddrs() throw(){}
    };

    class DuplicateRecord : public EXCEPTIONS::Exception
    {
    public:
        DuplicateRecord():Exception("RemoteSystemContext: Duplicate record"){}
    };

    class DcsSystemNotFound : public EXCEPTIONS::Exception
    {
    public:
        DcsSystemNotFound():Exception("DcsSystemContext: Record not found"){}
    };

    class InvalidSystemTypeCast : public EXCEPTIONS::Exception
    {
    public:
        InvalidSystemTypeCast():Exception("Invalid System Type Cast"){}
    };

    class DuplicateAirportDcsConf: public EXCEPTIONS::Exception
    {
    public:
        DuplicateAirportDcsConf():Exception("DcsSystemContext: Diplicate Dcs configuration"){}
    };


    class SystemTypeElem : public BaseTypeElem<int>
    {
        typedef BaseTypeElem<int> CodeListData_t;
        public:
            static const char *ElemName;
            SystemTypeElem(int codeI, const char *code,
                        const char *ldesc,
                        const char *rdesc) throw()
            :CodeListData_t(codeI, code, code, rdesc, ldesc)
            {
            }
            virtual ~SystemTypeElem(){}
    };

    /// @class SystemType - ⨯ ��⥬�
    class SystemType : public BaseTypeElemHolder<SystemTypeElem>
    {
    public:
        enum SystemTypes_t
        {
            DcsSystem = 'D',
            EtsSystem = 'E'
        };

        typedef BaseTypeElemHolder<SystemTypeElem>::TypesMap SystemTypesMap;

        SystemType(SystemTypes_t t):
                BaseTypeElemHolder<SystemTypeElem>(t)
        {
        }
        SystemType(const std::string &t):
                BaseTypeElemHolder<SystemTypeElem>(t)
        {
        }

        SystemType():
                BaseTypeElemHolder<SystemTypeElem>()
        {
        }
    };

    class DcsSystemContext;
    class EtsSystemContext;


    class SystemSettings
    {
    public:
        SystemSettings();

        virtual ~SystemSettings(){}
    };

    /// @class SystemContext
    /// @brief ���� ��� �� 㤠������ (������) ��⥬�
    class SystemContext
    {
        ASTRA::Airline_t OurAirline;
        ASTRA::Airline_t RemoteAirline;
        ASTRA::FlightNum_t OurFlightNum;
        ASTRA::FlightNum_t RemoteFlightNum;

        std::string OurAddrEdifact;
        std::string OurAddrEdifactSnd;
        std::string OurAddrEdifactExt;

        std::string RemoteAddrEdifact;
        std::string RemoteAddrEdifactSnd;
        std::string RemoteAddrEdifactExt;

        ASTRA::RouterId_t Router;
        SystemType SysType;
        std::string Description;

        ASTRA::SystemAddrs_t Ida;
        ASTRA::EdifactProfile_t EdifactProfileId;
        mutable edifact::pEdifactProfile EdifactProfileCache;
        mutable Language Lang;
        SystemSettings CommonSettings;
    private:
        /// @TODO VLAD ��� ��� SQL
        static std::string getSelText();
        /// @TODO VLAD ����� ����� ���
        static void defSelData(SystemContext &ctxt);

        static SystemContext readByEdiAddrsQR
                (const std::string &source, const std::string &source_ext,
                 const std::string &dest,   const std::string &dest_ext,
                 bool by_req);
        static SystemContext readByEdiAddrs
                (const std::string &source, const std::string &source_ext,
                 const std::string &dest,   const std::string &dest_ext);
        static SystemContext readByAnswerEdiAddrs
                (const std::string &source, const std::string &source_ext,
                 const std::string &dest,   const std::string &dest_ext);

        static boost::shared_ptr<SystemContext> SysCtxt;
    public:
        ASTRA::Airline_t ourAirline() const
        {
            return OurAirline;
        }
        ASTRA::Airline_t remoteAirline() const
        {
            return RemoteAirline;
        }
        ASTRA::FlightNum_t ourFlightNum() const
        {
            return OurFlightNum;
        }
        ASTRA::FlightNum_t remoteFlightNum() const
        {
            return RemoteFlightNum;
        }

        /**
         * @brief our edifact address
         * @return
         */
        const std::string &ourAddrEdifact() const
        {
            return OurAddrEdifact;
        }

        /**
         * @brief our edifact address for sending a request
         * @return
         */
        const std::string &ourAddrEdifactSnd() const
        {
            return OurAddrEdifactSnd;
        }

        /**
         * @brief remote edifact address
         * @return
         */
        const std::string &remoteAddrEdifact() const
        {
            return RemoteAddrEdifact;
        }

        /**
         * @brief remote edifact address for sending a request
         * @return
         */
        const std::string &remoteAddrEdifactSnd() const
        {
            return RemoteAddrEdifactSnd;
        }

        /**
         * @brief Our edifact addresses extra info
         * @return
         */
        const std::string &ourAddrEdifactExt() const
        {
            return OurAddrEdifactExt;
        }
        /**
         * @brief remote edifact addresses extra info
         * @return
         */
        const std::string &remoteAddrEdifactExt() const
        {
            return RemoteAddrEdifactExt;
        }

        ASTRA::RouterId_t router() const
        {
            return Router;
        }

        const std::string canonName() const
        {
            return "LAZHA"; /// @TODO VLAD get Canon Name by router ID  - RouterId_t router() const
        }


        const SystemType & systemType() const
        {
            return SysType;
        }

        const std::string &description() const
        {
            return Description;
        }

        /**
         * @brief Edifact profile ID
         * @return
         */
        const ASTRA::EdifactProfile_t edifactProfileId() const
        {
            return EdifactProfileId;
        }

        const edifact::EdifactProfile &edifactProfile() const;

        /**
         * ��� ⥪�饩 ��⥬� ( ��-㬮�砭�� - ������᪨�)
         * @return Language
         */
        Language lang() const
        {
            return Lang;
        }

        /**
         * �������᪨ ������� �� ��� ������ ��⥬�
         * @param l language
         */
        void setLang(Language l) const
        {
            Lang = l;
        }

        ASTRA::SystemAddrs_t ida() const { return Ida; }

        /**
         * @brief Edifact time out in seconds
         * @return
         */
        unsigned edifactResponseTimeOut() const;

        /**
         * @brief settings for all types of systems
         * @return
         */
        virtual const SystemSettings &commonSettings() const { return CommonSettings; }

        virtual SystemSettings& commonSettings() { return CommonSettings; }

        static bool initialized();

        /**
         * @brief ���樠����஢��� ���⥪�� ��⥬� �� edifact ���ᠬ
         * @param src ���� ��ࠢ�⥫�
         * @param src_ext ��� ��� �� ��� ��� � ����� (Address for reverse routing)
         * @param dest ���� �����⥫�
         * @param dest_ext ��� ��� �� ��� ��� � ����� (Address for reverse routing)
         * @return SystemContext *
         */
        static SystemContext *initEdifact(const std::string &src, const std::string &src_ext,
                                          const std::string &dest,const std::string &dest_ext);
        /**
         * @brief ���樠����஢��� ���⥪�� ��⥬� �� edifact ���ᠬ (��� �⢥�)
         * @param src ���� ��ࠢ�⥫�
         * @param src_ext ��� ��� �� ��� ��� � ����� (Address for reverse routing)
         * @param dest ���� �����⥫�
         * @param dest_ext ��� ��� �� ��� ��� � ����� (Address for reverse routing)
         * @return SystemContext *
         */
        static SystemContext *initEdifactByAnswer(const std::string &src,
                                            const std::string &src_ext,
                                            const std::string &dest,
                                            const std::string &dest_ext);

        static SystemContext *init(const SystemContext &);
        /**
         * @brief ���樠����஢��� ���⥪�� ��⥬�
         */
        static void free();

        /**
         * @brief ������ ������ �� Id
         * @param Id - id
         * @return
         */
        static SystemContext readById(ASTRA::SystemAddrs_t Id);

        /**
         * @brief ������ ������ ������ (Ets, Dcs)
         * @return pointer
         */
        SystemContext *readChildByType() const;

        /**
         * @brief Dcs system instance cast
         * @return
         */
        static const DcsSystemContext & DcsInstance(const char *nick, const char *file, unsigned line);
        /**
         * @brief Ets system instance cast
         * @return
         */
        static const EtsSystemContext & EtsInstance(const char *nick, const char *file, unsigned line);

        static const SystemContext &Instance(const char *nick, const char *file, unsigned line);

        /**
         * @brief split edifact address
         * @param src
         * @param dest
         * @param dest_ext
         */
        static void splitEdifactAddrs(std::string src, std::string &dest,
                                      std::string &dest_ext);

        /**
         * @brief join edifact addresses
         * @param src
         * @param src_ext
         * @return
         */
        static std::string joinEdifactAddrs(const std::string &src, const std::string &src_ext);

    public:
        SystemContext() {};
        virtual ~SystemContext(){}
    };

//     class SystemContextMaker
//     {
//         SystemContext cont;
//     public:
//         SystemContextMaker(){}
//         SystemContextMaker(const SystemContext &s):cont(s){}
// 
//         void setOurAirline(ASTRA::Airline_t val);
//         void setRemoteAirline(ASTRA::Airline_t val);
// 
//         // ���� ��� ����祭�� ����ᮢ
//         void setOurAddrEdifact(const std::string &val);
//         void setRemoteAddrEdifact(const std::string &val);
// 
//         // ���� ��� ��ࠢ�� ����ᮢ
//         void setOurAddrEdifactSnd(const std::string &val);
//         void setRemoteAddrEdifactSnd(const std::string &val);
// 
//         // ����७�� ��� ���ᮢ (� 㪠������ �������� ���ਬ��)
//         void setOurAddrEdifactExt(const std::string &val);
//         void setRemoteAddrEdifactExt(const std::string &val);
// 
//         void setOurAddrAirimp(const std::string &val);
//         void setRemoteAddrAirimp(const std::string &val);
//         void setRouter(RouterId_t val);
//         void setSysType(SystemType val);
//         void setDescription(const std::string &val);
//         void setIda(SystemAddrs_t val);
//         void setEdifactProfileId(EdifactProfile_t id);
// 
//         void setSystemSettings(const SystemSettings &sett);
// 
//         SystemContext getSystemContext();
//     };



    class DcsSystemSettings : public SystemSettings
    {
    public:
        DcsSystemSettings()
        {
        }
        virtual ~DcsSystemSettings(){}
    };

    /// @class DcsSystemContext
    /// @brief ���⥬� ॣ����樨
    class DcsSystemContext : public SystemContext
    {
        DcsSystemSettings Settings;
    public:
        /**
         * @brief Dcs settings
         * @return
         */
        const DcsSystemSettings &settings() const { return Settings; }
        DcsSystemContext(const SystemContext & baseCnt):SystemContext(baseCnt) {};
        static DcsSystemContext readFromDb(const SystemContext & baseCnt);
        DcsSystemContext readById(ASTRA::SystemAddrs_t Id);
    };

    class EtsSystemContext;

    typedef boost::shared_ptr<EtsSystemContext> pEtsSystemContext;

    /**
     * @class EtsSystemSettings
     * @brief ETS system settings
    */
    class EtsSystemSettings : public SystemSettings
    {
        friend class EtsSystemContext;
    public:
        EtsSystemSettings(){}
        EtsSystemSettings(const SystemSettings &sett,
                          const EtsSystemSettings &ets_sett)
               :SystemSettings(sett)
        {
        }
        virtual ~EtsSystemSettings(){}
    };

    /// @class EtsSystemContext
    /// @brief ��ࢥ� ���஭��� ����⮢ (���ૠ��)
    class EtsSystemContext : public SystemContext
    {
        EtsSystemSettings Settings;
    public:
        EtsSystemContext(const SystemContext & baseCnt, const EtsSystemSettings &Settings);
        static EtsSystemContext readById(ASTRA::SystemAddrs_t Id);
        static EtsSystemContext readFromDb(const SystemContext & baseCnt);

        /**
         * @brief Ets settings
         * @return
         */
        const EtsSystemSettings &settings() const { return Settings; }

        /**
         * @brief settings for all types of systems
         * @return
         */
        virtual const SystemSettings &commonSettings() const { return Settings; }

        virtual ~EtsSystemContext(){}
    };
} // namespace edifact
#endif /*_REMOTE_SYSTEM_CONTEXT_H_*/
