#pragma once

#include <string>
#include <vector>
#include <set>
#include <optional>
#include "astra_types.h"
#include "trip_tasks.h"
#include "astra_dates.h"

namespace APPS {

const int NumSendAttempts = 5; // ������⢮ ����⮪ �� ����祭�� �ॢ��� "��� �裡 � APPS"
const int MaxSendAttempts = 99; // ���ᨬ��쭮� ������⢮ ����⮪

DECL_RIP(AppsSettingsId_t, int);

enum class TransferFlag
{
    None,
    Origin,
    Dest,
    Both
};

template<typename T>
using Opt = std::optional<T>;

void dumpAppsPaxData();
void dumpAppsMsg();

std::ostream& operator << (std::ostream& os, const TransferFlag& trfer);
std::vector<std::string> statusesFromDb(const PaxId_t& pax_id);
void appsFlightCloseout(const PointId_t& point_id);
bool processReply(const std::string& source_raw);
bool checkAPPSSets(const PointId_t& point_dep);
bool checkAPPSSets(const PointId_t& point_dep, const PointId_t& point_arv);
bool checkAPPSSets(const PointId_t& point_dep, const AirportCode_t& airp_arv);
bool checkAPPSFormats(const PointId_t &point_dep, const AirportCode_t& airp_arv,
                      std::set<std::string> *pFormats);
bool isAPPSAnswText(const std::string& tlg_body);
void reSendMsg(int msg_id);
void deleteMsg(int msg_id);
Opt<PointId_t> pointIdByMsgId(int msg_id);
const char* getAPPSRotName();
std::string appsTextAsHumanReadable(const std::string& apps);
std::string emulateAnswer(const std::string& request);
bool checkNeedAlarmScdIn(const PointId_t& point_id);

void ifNeedAddTaskSendApps(const PointId_t& point_id, const std::string &task_name);
bool checkTime(const PointId_t& point_id);
void sendAllInfo(const TTripTaskKey& task);
void sendNewInfo(const TTripTaskKey& task);

bool allowedToBoarding(const PaxId_t& paxId);
void init_callbacks();

class AppsCollector
{
public:
    struct PaxItem
    {
        PaxId_t       pax_id;
        std::string override;
        bool        is_forced;
    };
    void addPaxItem(const PaxId_t& pax_id, const std::string& override = "", bool is_forced = false);
    void send();

private:
    std::vector<PaxItem> m_paxItems;
};

#ifdef XP_TESTING
void updateAppsMsg(int msg_id, Dates::DateTime_t send_time, int send_attempts);
#endif

} //namespace APPS

