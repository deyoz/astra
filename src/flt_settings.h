#pragma once

#include "astra_misc.h"

//����ன�� ३�
enum TTripSetType { /*�� �ਢ易��� � ३��*/
                    tsCraftInitVIP=1,                   //��⮬���᪠� ࠧ��⪠ VIP-���� � ����������
                    tsEDSNoExchange=10,                 //����� ������ � �����.
                    tsETSNoExchange=11,                 //����� ������ � �����. ���쪮 ETL.
                    tsIgnoreTrferSet=12,                //��ଫ���� ��� �࠭��� ��� ��� ����஥�
                    tsMixedNorms=13,                    //���訢���� ���, ��䮢, ᡮ஢ �� �࠭���
                    tsNoTicketCheck=15,                 //�⬥�� ����஫� ����஢ ����⮢
                    tsCharterSearch=16,                 //���᪮�� ����� ॣ����樨 ��� ���஢
                    tsComponSeatNoChanges=17,            //����� ��������� ����������, �����祭��� �� ३�
                    tsCheckMVTDelays=18,                //����஫� ����� ����থ�
                    tsSendMVTDelays=19,                 //��ࠢ�� MVT �� ����� � ����প���
                    tsPrintSCDCloseBoarding=21,         //�⮡ࠦ���� ��������� �६��� � ��ᠤ�筮� ⠫���
                    tsMintransFile=22,                  //���� ��� ����࠭�
                    tsAODBCreateFlight=26,              //�������� ३ᮢ �� AODB
                    tsSetDepTimeByMVT=27,               //���⠢����� �뫥� ३� �� ⥫��ࠬ�� MVT
                    tsSyncMeridian=28,                  //����஭����� � ��ਤ�����
                    tsNoEMDAutoBinding=31,              //����� ��⮯ਢ離� EMD
                    tsCheckPayOnTCkinSegs=32,           //����஫� ������ ⮫쪮 �� ᪢����� ᥣ�����
                    tsAutoPTPrint=34,                   //��⮬���᪠� ����� ��ᠤ����
                    tsAutoPTPrintReseat=35,             //��⮬���᪠� ����� �� �� ��������� ����
                    tsPrintFioPNL=36,                   //����� � ��ᠤ�筮� ��� �� �஭�஢���� (PNL)
                    tsWeightControl=37,                 //����஫� ��� ������
                    tsLCIPersWeights=38,                //��� ���ᠦ�஢ �� �᭮����� LCI
                    tsNoCrewCkinAlarm=40,               //�⬥�� �ॢ��� '��������� �����'
                    tsNoCtrlDocsCrew=41,                //�� ����஫�஢��� ���� ���㬥�⮢ ��� �����
                    tsNoCtrlDocsExtraCrew=42,           //�� ����஫�஢��� ���� ���㬥�⮢ ��� ���. �����
                    tsETSControlMethod=43,              //����஫�� ��⮤ �� ������ � �����
                    tsNoRefuseIfBrd=44,                 //����� �⬥�� ॣ����樨 �᫨ ���ᠦ�� ����� "��ᠦ��"
                    tsRegWithoutNOREC=45,               //����� ॣ����樨 NOREC
                    tsBanAdultsWithBabyInOneZone=54,    //����� ॣ����樨 ���ᠦ�஢ � �����栬� � ����� ����� ����
                    tsAdultsWithBabyInOneZoneWL=55,     //��������� �� �� ���᫮�� � ॡ�����, �᫨ ��� ����� ������ ��� ������楢
                    tsProcessInboundLDM=56,             //��ࠡ�⪠ �室��� LDM
                    tsBaggageCheckInCrew=57,            //��ଫ���� ������ ��� �����
                    tsDeniedSeatCHINEmergencyExit=60,   //��������� �� �� ���᫮�� � ॡ�����, �᫨ ��� ����� ������ ��� ������楢
                    tsDeniedSeatOnPNLAfterPay=61,       //���। ��ᠤ�� �� ����祭�� ���� �� PNL/ADL
                    tsSeatDescription=62,               //����� �⮨���� � ᠫ��� �� ᢮���� ����
                    tsTimaticManual=63,                 //����� � timatic � ��筮� ०���
                    tsTimaticAuto=64,                   //����� � timatic � ��⮬���᪮� ०���
                    tsTimaticInfo=65,                   //����� � timatic � �ࠢ�筮� ०���
                    tsLIBRACent=66,                     //����஢�� LIBRA
                    tsBCBPInf=67,                       //�����প� inf � 2D ��મ��
                    tsShowTakeoffDiffTakeoffACT=70,     //�뢮���� ᮮ�饭�� � ���� � ࠧ��� �६�� 䠪� �뫥� � ��������� ��� ࠧ��⭮�� �६���
                    tsShowTakeoffPassNotBrd=71,         //�뢮���� ᮮ�饭�� � ���� � ⮬, �� ���� �� ��ᠦ���� ���ᠦ�� �� ���⠢����� 䠪� �뫥�
                    tsDeniedBoardingJMP=80,             //����� ��ᠤ�� ���ᠦ�஢ JMP
                    tsChangeETStatusWhileBoarding=81,   //��������� ����� �� �� ��ᠤ��
                    tsNotUseBagNormFromET=82,           //�� �ਬ����� ��ᮢ�� ���� �� ��
                    tsPayAtDesk=90,                     //����� �� �⮩�� ॣ����樨
                    tsReseatOnRFISC=93,                 //ᮮ�饭�� �� ���ᠤ�� ���ᠦ�� �� ���� � ࠧ��⪮� RFISC
                    tsAirlineCompLayerPriority=95,//���� �ਮ��� ᫮�� �� ���� ��
                    
                    /*�ਢ易��� � ३�� (���� ᮮ⢥�����騥 ���� � ⠡��� trip_sets � CheckBox � "�����⮢�� � ॣ����樨")*/
                    tsCheckLoad=2,                  //����஫� ����㧪� �� ॣ����樨
                    tsOverloadReg=3,                //����襭�� ॣ����樨 �� �ॢ�襭�� ����㧪�
                    tsExam=4,                       //��ᬮ�஢� ����஫� ��। ��ᠤ���
                    tsCheckPay=5,                   //����஫� ������ ������ �� ��ᠤ��
                    tsExamCheckPay=7,               //����஫� ������ ������ �� ��ᬮ��
                    tsRegWithTkn=8,                 //����� ॣ����樨 ��� ����஢ ����⮢
                    tsRegWithDoc=9,                 //����� ॣ����樨 ��� ����஢ ���㬥�⮢
                    tsRegWithoutTKNA=14,            //����� ॣ����樨 TKNA
                    tsAutoWeighing=20,              //����஫� ��⮬���᪮�� ����訢���� ������
                    tsFreeSeating=23,               //��������� ��ᠤ��
                    tsAPISControl=24,               //����஫� ������ APIS
                    tsAPISManualInput=25,           //��筮� ���� ������ APIS
                    tsPieceConcept=30,              //���⥬� ���� ������ � ��㣨 �� ���
                    tsUseJmp=39,                    //��������� �� �⪨��� ᨤ����
                    tsJmpCfg=1000,                  //���-�� �⪨���� ᨤ����
                    tsTransitReg=1001,              //���ॣ������ �࠭���
                    tsTransitBortChanging=1002,     //����� ����, ���ॣ������ �࠭���
                    tsTransitBrdWithAutoreg=1003,   //��ᠤ�� �� ���ॣ����樨 �࠭���
                    tsTransitBlocking=1004,         //��筠� �����஢�� �࠭���

                    //���, ����, �� �������� � ��� ᥪ�� ����ன��, ����� �� � ⠡��� trip_sets

                    /*�ਢ易��� � ३�� �� �����*/
                    tsBrdWithReg=101,               //��ᠤ�� �� ॣ����樨
                    tsExamWithBrd=102,              //��ᬮ�� �� ��ᠤ��

                    /*�� �ਢ易��� � ३�� ��� ᠬ�ॣ����樨*/
                    tsRegWithSeatChoice=201,        //����� ॣ����樨 ��� �롮� ����
                    tsRegRUSNationOnly=203,         //����� ॣ����樨 ��१����⮢ ��
                    tsSelfCkinCharterSearch=204,    //���᪮�� ����� ᠬ�ॣ����樨 ��� ���஢
                    tsNoRepeatedSelfCkin=205,       //����� ����୮� ᠬ�ॣ����樨
                    tsAllowCancelSelfCkin=206,       //�⬥�� ᠬ�ॣ����樨 ��� ��� �ନ����
                    tsKioskCheckinOnPaidSeat=207,    //��������� ���ᠦ�஢ � ���᪠ �� ����� ����
                    tsEmergencyExitBPNotAllowed=208  //����� ���� ��ᠤ�筮�� �� ����� ���਩���� ��室� ��� ᠬ�ॣ����樨

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

