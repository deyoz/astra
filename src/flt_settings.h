#pragma once

#include "astra_misc.h"

//настройки рейса
enum TTripSetType { /*не привязанные к рейсу*/
                    tsCraftInitVIP=1,                   //Автоматическая разметка VIP-мест в компоновке
                    tsEDSNoExchange=10,                 //Запрет обмена с СЭДом.
                    tsETSNoExchange=11,                 //Запрет обмена с СЭБом. Только ETL.
                    tsIgnoreTrferSet=12,                //Оформление любого трансфера без учета настроек
                    tsMixedNorms=13,                    //Смешивание норм, тарифов, сборов при трансфере
                    tsNoTicketCheck=15,                 //Отмена контроля номеров билетов
                    tsCharterSearch=16,                 //Поисковый запрос регистрации для чартеров
                    tsComponSeatNoChanges=17,            //Запрет изменения компоновки, назначенной на рейс
                    tsCheckMVTDelays=18,                //Контроль ввода задержек
                    tsSendMVTDelays=19,                 //Отправка MVT на взлет с задержками
                    tsPrintSCDCloseBoarding=21,         //Отображение планового времени в посадочном талоне
                    tsMintransFile=22,                  //Файл для Минтранса
                    tsAODBCreateFlight=26,              //Создание рейсов из AODB
                    tsSetDepTimeByMVT=27,               //Проставление вылета рейса по телеграмме MVT
                    tsSyncMeridian=28,                  //Синхронизация с меридианом
                    tsNoEMDAutoBinding=31,              //Запрет автопривязки EMD
                    tsCheckPayOnTCkinSegs=32,           //Контроль оплаты только на сквозных сегментах
                    tsAutoPTPrint=34,                   //Автоматическая печать посадочных
                    tsAutoPTPrintReseat=35,             //Автоматическая печать ПТ при изменении места
                    tsPrintFioPNL=36,                   //Печать в посадочном ФИО из бронирования (PNL)
                    tsWeightControl=37,                 //Контроль веса багажа
                    tsLCIPersWeights=38,                //Веса пассажиров на основании LCI
                    tsNoCrewCkinAlarm=40,               //Отмена тревоги 'Регистрация экипажа'
                    tsNoCtrlDocsCrew=41,                //Не контролировать ввод документов для экипажа
                    tsNoCtrlDocsExtraCrew=42,           //Не контролировать ввод документов для доп. экипажа
                    tsETSControlMethod=43,              //Контрольный метод при обмене с СЭБом
                    tsNoRefuseIfBrd=44,                 //Запрет отмены регистрации если пассажир статус "посажен"
                    tsRegWithoutNOREC=45,               //Запрет регистрации NOREC
                    tsBanAdultsWithBabyInOneZone=54,    //Запрет регистрации пассажиров с младецами в одном блоке мест
                    tsAdultsWithBabyInOneZoneWL=55,     //Регистрация на ЛО взрослого с ребенком, если нет больше блоков без младенцев
                    tsProcessInboundLDM=56,             //Обработка входных LDM
                    tsBaggageCheckInCrew=57,            //Оформление багажа для экипажа
                    tsDeniedSeatCHINEmergencyExit=60,   //Регистрация на ЛО взрослого с ребенком, если нет больше блоков без младенцев
                    tsDeniedSeatOnPNLAfterPay=61,       //Запред рассадки на оплаченные места из PNL/ADL
                    tsSeatDescription=62,               //Расчет стоимости в салоне по свойству мест
                    tsTimaticManual=63,                 //Работа с timatic в ручном режиме
                    tsTimaticAuto=64,                   //Работа с timatic в автоматическом режиме
                    tsTimaticInfo=65,                   //Работа с timatic в справочном режиме
                    tsLIBRACent=66,                     //Центровка LIBRA
                    tsBCBPInf=67,                       //Поддержка inf в 2D баркоде
                    tsShowTakeoffDiffTakeoffACT=70,     //Выводить сообщение в СОПП о разнице времен факта вылета и планового или разсчетного времени
                    tsShowTakeoffPassNotBrd=71,         //Выводить сообщение в СОПП о том, что есть не посаженные пассажира при проставлении факта вылета
                    tsDeniedBoardingJMP=80,             //Запрет посадки пассажиров JMP
                    tsChangeETStatusWhileBoarding=81,   //Изменение статуса ЭБ при посадке
                    tsNotUseBagNormFromET=82,           //Не применять весовую норму из ЭБ
                    tsPayAtDesk=90,                     //Оплата на стойке регистрации
                    tsReseatOnRFISC=93,                 //сообщение при пересадке пассажира на место с разметкой RFISC
                    tsAirlineCompLayerPriority=95,//расчет приоритета слоев по коду АК

                    /*привязанные к рейсу (есть соответствующие поля в таблице trip_sets и CheckBox в "Подготовке к регистрации")*/
                    tsCheckLoad=2,                  //Контроль загрузки при регистрации
                    tsOverloadReg=3,                //Разрешение регистрации при превышении загрузки
                    tsExam=4,                       //Досмотровый контроль перед посадкой
                    tsCheckPay=5,                   //Контроль оплаты багажа при посадке
                    tsExamCheckPay=7,               //Контроль оплаты багажа при досмотре
                    tsRegWithTkn=8,                 //Запрет регистрации без номеров билетов
                    tsRegWithDoc=9,                 //Запрет регистрации без номеров документов
                    tsRegWithoutTKNA=14,            //Запрет регистрации TKNA
                    tsAutoWeighing=20,              //Контроль автоматического взвешивания багажа
                    tsFreeSeating=23,               //Свободная рассадка
                    tsAPISControl=24,               //Контроль данных APIS
                    tsAPISManualInput=25,           //Ручной ввод данных APIS
                    tsPieceConcept=30,              //Система расчета багажа и услуги из ГРС
                    tsUseJmp=39,                    //Регистрация на откидные сиденья
                    tsJmpCfg=1000,                  //Кол-во откидных сидений
                    tsTransitReg=1001,              //Перерегистрация транзита
                    tsTransitBortChanging=1002,     //Смена борта, авторегистрация транзита
                    tsTransitBrdWithAutoreg=1003,   //Посадка при авторегистрации транзита
                    tsTransitBlocking=1004,         //Ручная блокировка транзита

                    //Ден, Женя, не добавляйте в эту секцию настройки, которые не в таблице trip_sets

                    /*привязанные к рейсу по залам*/
                    tsBrdWithReg=101,               //Посадка при регистрации
                    tsExamWithBrd=102,              //Досмотр при посадке

                    /*не привязанные к рейсу для саморегистрации*/
                    tsRegWithSeatChoice=201,        //Запрет регистрации без выбора места
                    tsRegRUSNationOnly=203,         //Запрет регистрации нерезидентов РФ
                    tsSelfCkinCharterSearch=204,    //Поисковый запрос саморегистрации для чартеров
                    tsNoRepeatedSelfCkin=205,       //Запрет повторной саморегистрации
                    tsAllowCancelSelfCkin=206,       //Отмена саморегистрации без учета терминала
                    tsKioskCheckinOnPaidSeat=207,    //Регистрация пассажиров с киоска на платные места
                    tsEmergencyExitBPNotAllowed=208  //Запрет печати посадочного на местах аварийного выхода для саморегистрации

                  };

bool DefaultTripSets( const TTripSetType setType );
bool GetTripSets( const TTripSetType setType,
                  const TTripInfo &info );
bool GetSelfCkinSets( const TTripSetType setType,
                      const int point_id,
                      const ASTRA::TClientType client_type );
bool GetSelfCkinSets( const TTripSetType setType,
                      const TTripInfo &info,
                      const ASTRA::TClientType client_type );

bool TTripSetListItemLess(const std::pair<TTripSetType, boost::any> &a, const std::pair<TTripSetType, boost::any> &b);

class TTripSetList : public std::map<TTripSetType, boost::any>
{
  private:
    const std::map<TTripSetType, std::string> _settingTypes =
    { {tsCheckLoad,            "pr_check_load"},
      {tsOverloadReg,          "pr_overload_reg"},
      {tsExam,                 "pr_exam"},
      {tsCheckPay,             "pr_check_pay"},
      {tsExamCheckPay,         "pr_exam_check_pay"},
      {tsRegWithTkn,           "pr_reg_with_tkn"},
      {tsRegWithDoc,           "pr_reg_with_doc"},
      {tsRegWithoutTKNA,       "pr_reg_without_tkna"},
      {tsAutoWeighing,         "auto_weighing"},
      {tsFreeSeating,          "pr_free_seating"},
      {tsAPISControl,          "apis_control"},
      {tsAPISManualInput,      "apis_manual_input"},
      {tsPieceConcept,         "piece_concept"},
      {tsUseJmp,               "use_jmp"},
      {tsJmpCfg,               "jmp_cfg"},
      {tsTransitReg,           "pr_tranz_reg"},
      {tsTransitBortChanging,  "trzt_bort_changing"},
      {tsTransitBrdWithAutoreg,"trzt_brd_with_autoreg"},
      {tsTransitBlocking,      "pr_block_trzt"},
    };

    std::string toString(const TTripSetType settingType) const;

    void checkAndCorrectTransitSets(const boost::optional<bool>& pr_tranzit);
  public:
    const TTripSetList& toXML(xmlNodePtr node) const;
    TTripSetList& fromXML(xmlNodePtr node, const bool pr_tranzit, const TTripSetList &priorSettings);
    const TTripSetList& initDB(int point_id, int f, int c, int y) const;
    const TTripSetList& toDB(int point_id) const;
    TTripSetList& fromDB(int point_id);
    TTripSetList& getTransitSets(const TTripInfo &flt, boost::optional<bool> &pr_tranzit);
    TTripSetList& fromDB(const TTripInfo &info);
    void append(const TTripSetList &list);

    void throwBadCastException(const TTripSetType settingType, const std::string &where) const
    {
      throw EXCEPTIONS::Exception("%s: settingType=%d bad cast", where.c_str(), (int)settingType);
    }

    template<typename T>
    T value(const TTripSetType settingType) const
    {
      TTripSetList::const_iterator i=find(settingType);
      if (i==end())
        throw EXCEPTIONS::Exception("TTripSetList::%s: settingType=%d not found", __FUNCTION__, (int)settingType);
      try
      {
        return boost::any_cast<T>(i->second);
      }
      catch(const boost::bad_any_cast&)
      {
        throw EXCEPTIONS::Exception("TTripSetList::%s: settingType=%d bad cast", __FUNCTION__, (int)settingType);
      }
    }

    template<typename T>
    T value(const TTripSetType settingType, const T &defValue) const
    {
      TTripSetList::const_iterator i=find(settingType);
      if (i==end()) return defValue;
      try
      {
        return boost::any_cast<T>(i->second);
      }
      catch(const boost::bad_any_cast&)
      {
        throw EXCEPTIONS::Exception("TTripSetList::%s: settingType=%d bad cast", __FUNCTION__, (int)settingType);
      }
    }

    bool isInt(const TTripSetType settingType) const
    {
      return settingType==tsJmpCfg;
    }
    bool isBool(const TTripSetType settingType) const
    {
      return !isInt(settingType);
    }
    boost::any defaultValue(const TTripSetType settingType) const
    {
      if (isBool(settingType))
        return DefaultTripSets(settingType);
      else if (isInt(settingType))
        return (int)0;
      else
        return boost::any();
    }


};

