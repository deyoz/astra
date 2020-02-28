#pragma once

#include <string>
#include <vector>
#include <set>
#include "astra_types.h"
#include "trip_tasks.h"

namespace APPS {

const int NumSendAttempts = 5; // количество попыток до включения тревоги "Нет связи с APPS"
const int MaxSendAttempts = 99; // максимальное количество попыток

DECL_RIP(AppsSettingsId_t, int);

enum class TransferFlag
{
    None,
    Origin,
    Dest,
    Both
};

template<typename T>
using Opt = boost::optional<T>;

std::ostream& operator << (std::ostream& os, const TransferFlag& trfer);
std::vector<std::string> statusesFromDb(const PaxId_t& pax_id);
void appsFlightCloseout(const PointId_t& point_id);
bool processReply(const std::string& source_raw);
bool checkAPPSSets(const PointId_t& point_dep);
bool checkAPPSSets(const PointId_t& point_dep, const PointId_t& point_arv);
bool checkAPPSSets(const PointId_t& point_dep, const AirportCode_t& airp_arv);
bool checkAPPSFormats(const PointId_t &point_dep, const AirportCode_t& airp_arv,
                      std::set<std::string>& pFormats);
bool isAPPSAnswText(const std::string& tlg_body);
void reSendMsg(const int send_attempts, const std::string& msg_text, const int msg_id);
void deleteMsg(const int msg_id);
const char* getAPPSRotName();
std::string appsTextAsHumanReadable(const std::string& apps);
std::string emulateAnswer(const std::string& request);
bool checkNeedAlarmScdIn(const PointId_t& point_id);

void ifNeedAddTaskSendApps(const PointId_t& point_id, const std::string &task_name);
bool checkTime(const PointId_t& point_id);
void sendAllAPPSInfo(const TTripTaskKey& task);
void sendNewAPPSInfo(const TTripTaskKey& task);

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

} //namespace APPS

