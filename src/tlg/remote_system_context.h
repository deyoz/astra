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

#include <string>
#include <boost/shared_ptr.hpp>
#include <map>
#include <list>
#include <time.h>

// #include "dates.h"
#include "etick/lang.h"
// #include "exceptions.h"
#include "etick/exceptions.h"
// #include "tlg_types.h"
// #include "TicketBaseTypes.h"
// #include "etick/etick_msg.h"
#include "etick/tick_data.h"
#include "EdifactProfile.h"
#include "tlg_source_edifact.h"
// #include "optionals.h"
// #include "typeb/typeb_preprocessor.h"

namespace OciCpp{
class CursCtl;
}

namespace edifact
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


    enum SystemTypes_t
    {
        DcsSystem = 'D',
        EtsSystem = 'E',
    };
    typedef BaseTypeElem<SystemTypes_t> SystemTypeElem;

    /// @class SystemType - тип системы
    class SystemType : public BaseTypeElemHolder<SystemTypeElem>
    {
    public:
        typedef BaseTypeElemHolder<SystemTypeElem>::TypesMap SystemTypesMap;

        SystemType(SystemTypes_t t):
                BaseTypeElemHolder<SystemTypeElem>(t)
        {
        }
        SystemType():
                BaseTypeElemHolder<SystemTypeElem>()
        {
        }


        static SystemType fromTypeStr(const char *);
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
    /// @brief общая инфа об удаленной (далеко) системе
    class SystemContext
    {
        Airline_t Airline;
        FlightNum_t FlightNum;

        std::string OurAddrEdifact;
        std::string RemoteAddrEdifact;

        RouterId_t Router;
        SystemType SysType;
        std::string Description;

        SystemAddrs_t Ida;
        EdifactProfile_t EdifactProfileId;
        mutable edifact::pEdifactProfile EdifactProfileCache;
        mutable Language Lang;
        SystemSettings CommonSettings;

        SystemContext()
        {
        }
    private:
        /// @TODO VLAD дай мне SQL
        static std::string getSelText();
        /// @TODO VLAD получи данные тут
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
        Airline_t airline() const
        {
            return Airline;
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

        RouterId_t router() const
        {
            return Router;
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
        const EdifactProfile_t edifactProfileId() const
        {
            return EdifactProfileId;
        }

        const edifact::EdifactProfile &edifactProfile() const;

        /**
         * Язык текущей системы ( по-умолчанию - Английский)
         * @return Language
         */
        Language lang() const
        {
            return Lang;
        }

        /**
         * Динамически изменяет язык для данной системы
         * @param l language
         */
        void setLang(Language l) const
        {
            Lang = l;
        }

        SystemAddrs_t ida() const { return Ida; }

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
         * @brief Инициализировать контекст системы по edifact адресам
         * @param src Адрес отправителя
         * @param src_ext для нас это доп инфа к адресу (Address for reverse routing)
         * @param dest Адрес получателя
         * @param dest_ext для нас это доп инфа к адресу (Address for reverse routing)
         * @return SystemContext *
         */
        static SystemContext *initEdifact(const std::string &src, const std::string &src_ext,
                                          const std::string &dest,const std::string &dest_ext);
        /**
         * @brief Инициализировать контекст системы по edifact адресам (Для ответа)
         * @param src Адрес отправителя
         * @param src_ext для нас это доп инфа к адресу (Address for reverse routing)
         * @param dest Адрес получателя
         * @param dest_ext для нас это доп инфа к адресу (Address for reverse routing)
         * @return SystemContext *
         */
        static SystemContext *initEdifactByAnswer(const std::string &src,
                                            const std::string &src_ext,
                                            const std::string &dest,
                                            const std::string &dest_ext);

        static SystemContext *init(const SystemContext &);
        /**
         * @brief Деициализировать контекст системы
         */
        static void free();

        /**
         * @brief Прочесть запись по Id
         * @param Id - id
         * @return
         */
        static SystemContext readById(SystemAddrs_t Id);

        /**
         * @brief Прочесть дочернюю запись (Ets, Dcs)
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
        virtual ~SystemContext(){}
    };


    class DcsSystemSettings : public SystemSettings
    {
    public:
        DcsSystemSettings()
        {
        }
        virtual ~DcsSystemSettings(){}
    };

    /// @class DcsSystemContext
    /// @brief Система регистрации
    class DcsSystemContext : public SystemContext
    {
    public:
        DcsSystemContext() {};
        DcsSystemContext readFromDb(const SystemContext & baseCnt);
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
        virtual ~EtsSystemSettings(){}
    };

    /// @class EtsSystemContext
    /// @brief Сервер электронных билетов (интерлайн)
    class EtsSystemContext : public SystemContext
    {
        EtsSystemSettings Settings;
    public:
        EtsSystemContext(){}

        static EtsSystemContext readById(SystemAddrs_t Id);
        
        EtsSystemContext readFromDb(const SystemContext & baseCnt);

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
