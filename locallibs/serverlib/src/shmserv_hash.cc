#include <setjmp.h>

#include <set>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "shmsrv.h"
#include "tclmon.h"
#include "monitor_ctl.h"
#include "ourtime.h"
#include "daemon_event.h"
#include "daemon_impl.h"
#include "testmode.h"
#include "logrange.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

bool operator<(const Zone& lhs, const Zone& rhs)
{
    return lhs.id < rhs.id;
}

bool operator<(const int lhs, const Zone& rhs)
{
    return lhs < rhs.id;
}

bool operator<(const Zone& lhs, const int rhs)
{
    return lhs.id < rhs;
}

bool operator==(const Zone& lhs, const Zone& rhs)
{
    return lhs.id == rhs.id
        && lhs.title == rhs.title
        && lhs.data == rhs.data;
}

std::ostream& operator<<(std::ostream& os, const Zone& zone)
{
    return os << "{ size of data: " << zone.data.size()
              << ", title: " << zone.title
              << ", id: " << zone.id
              << " }";
}

bool operator<(const std::shared_ptr< Zone >& lhs, const std::shared_ptr< Zone >& rhs)
{
    return (lhs && rhs) ? *lhs < *rhs : lhs.get() < rhs.get();
}

bool operator<(const int lhs, const std::shared_ptr< Zone >& rhs)
{
    return rhs ? lhs < *rhs : false;
}

bool operator<(const std::shared_ptr< Zone >& lhs, const int rhs)
{
    return lhs ? *lhs < rhs : false;
}

bool operator==(const std::shared_ptr< Zone >& lhs, const std::shared_ptr< Zone >& rhs)
{
    return (lhs && rhs) ? (*lhs == *rhs) : (lhs.get() == rhs.get());
}

class ZoneBuf
{
public:
    ZoneBuf(const bool null, const std::size_t dataSize, const std::size_t titleSize);

    static std::pair< std::shared_ptr< Zone >, const uint8_t* > unpack(const uint8_t* begin, const uint8_t* end);

    std::deque< boost::asio::mutable_buffer > buffers();

    constexpr std::size_t size() {
        return sizeof(null_) + sizeof(dataSize_) + sizeof(titleSize_);
    }

private:
    bool null_;
    std::size_t dataSize_;
    std::size_t titleSize_;
};

struct ShmCmdReq
{
    enum class Cmd { AddZone = 0, DelZone, UpdZone, GetZone, AduZone, GetList, DropList, ExitPult };

    int zoneNum;
    Cmd cmd;
    std::shared_ptr< Zone > zone;

    ShmCmdReq(const int zn, const Cmd c, const std::shared_ptr< Zone >& z);

    ShmCmdReq(const ShmCmdReq& ) = delete;
    ShmCmdReq& operator=(const ShmCmdReq& ) = delete;

    ShmCmdReq(const ShmCmdReq&& ) = delete;
    ShmCmdReq& operator=(const ShmCmdReq&& ) = delete;

    friend std::ostream& operator<<(std::ostream& os, const ShmCmdReq& cmd);
    friend bool operator==(const ShmCmdReq& lhs, const ShmCmdReq& rhs);
};
bool operator==(const std::shared_ptr< ShmCmdReq >& lhs, const std::shared_ptr< ShmCmdReq >& rhs);

struct ShmReq
{
    std::string owner;
    std::vector< std::shared_ptr< ShmCmdReq > > cmds;

    friend std::ostream& operator<<(std::ostream& os, const ShmReq& req);
    friend bool operator==(const ShmReq& lhs, const ShmReq& rhs);
    friend bool operator!=(const ShmReq& lhs, const ShmReq& rhs);
};

class ShmReqBuf
{
public:
    ShmReqBuf();

    std::shared_ptr< ShmReq > unpack();

    const std::deque< boost::asio::mutable_buffer >& head();
    const std::deque< boost::asio::mutable_buffer >& tail();

    std::deque< boost::asio::const_buffer >& all(const ShmReq& req);

private:
    unsigned ownerSize_;
    std::vector< uint8_t > owner_;
    unsigned cmdsSize_;
    std::vector< uint8_t > cmds_;
    std::deque< boost::asio::mutable_buffer > head_;
    std::deque< boost::asio::mutable_buffer > tail_;
    std::deque< boost::asio::const_buffer > all_;
    std::deque< ZoneBuf > zonesBufs_;
};

struct ShmCmdAnswer
{
    enum class Result { Success = 0, InternalServerError, InvalidRequest, InvalidZoneNumber };

    int zoneNum;
    Result result;
    ShmCmdReq::Cmd cmd;
    std::vector< std::shared_ptr< Zone > > zones;

    ShmCmdAnswer(const int zn, const Result r, const ShmCmdReq::Cmd c, std::vector< std::shared_ptr< Zone > >&& zns);

    ShmCmdAnswer(const ShmCmdAnswer& ) = delete;
    ShmCmdAnswer& operator=(const ShmCmdAnswer& ) = delete;

    ShmCmdAnswer(ShmCmdAnswer&& ) = delete;
    ShmCmdAnswer& operator=(ShmCmdAnswer&& ) = delete;

    friend std::ostream& operator<<(std::ostream& os, const ShmCmdAnswer& cmd);
    friend bool operator==(const ShmCmdAnswer& lhs, const ShmCmdAnswer& rhs);
};
bool operator==(const std::shared_ptr< ShmCmdAnswer >& lhs, const std::shared_ptr< ShmCmdAnswer >& rhs);

struct ShmAnswer
{
    std::string owner;
    std::vector< std::shared_ptr< ShmCmdAnswer > > answers;

    friend std::ostream& operator<<(std::ostream& os, const ShmAnswer& answer);
    friend bool operator==(const ShmAnswer& lhs, const ShmAnswer& rhs);
    friend bool operator!=(const ShmAnswer& lhs, const ShmAnswer& rhs);
};

class ShmAnswerBuf
{
public:
    ShmAnswerBuf();

    std::shared_ptr< ShmAnswer > unpack();

    const std::deque< boost::asio::mutable_buffer >& head();
    const std::deque< boost::asio::mutable_buffer >& tail();

    std::deque< boost::asio::const_buffer >& all(const ShmAnswer& answer);

private:
    unsigned ownerSize_;
    std::vector< uint8_t > owner_;
    unsigned answersSize_;
    std::vector< uint8_t > answers_;
    std::deque< boost::asio::mutable_buffer > head_;
    std::deque< boost::asio::mutable_buffer > tail_;
    std::deque< boost::asio::const_buffer > all_;
    std::deque< unsigned > zonesSizes_;
    std::deque< ZoneBuf > zonesBufs_;
};

struct ZoneList
{
    std::string owner;
    std::set< std::shared_ptr< Zone >, std::less<> > zones;
};

static int avost;
static jmp_buf termpoint;

static void runLoop(int controlPipe, char *addr);
static void process(ShmReq& req, ShmAnswer& answer);

ZoneBuf::ZoneBuf(const bool null, const std::size_t dataSize, const std::size_t titleSize)
    : null_ { null }, dataSize_ { dataSize }, titleSize_ { titleSize }
{ }

std::pair< std::shared_ptr< Zone >, const uint8_t* > ZoneBuf::unpack(const uint8_t* begin, const uint8_t* end)
{
    ASSERT(begin < end);

    const decltype(null_) null { *reinterpret_cast< const decltype(null_)* >(begin) };
    begin += sizeof(null);

    ASSERT(begin < end);

    const decltype(dataSize_) dataSize { *reinterpret_cast< const decltype(dataSize_)* >(begin) };
    begin += sizeof(dataSize);

    ASSERT(begin < end);

    const decltype(titleSize_) titleSize { *reinterpret_cast< const decltype(titleSize_)* >(begin) };
    begin += sizeof(titleSize) ;

    ASSERT(begin <= end);

    if (null) {
        return { std::shared_ptr< Zone > { }, begin };
    }

    ASSERT(begin < end);

    std::vector< uint8_t > data { begin, begin + dataSize };
    begin += dataSize;

    ASSERT(begin < end);

    std::string title { begin, begin + titleSize };
    begin += titleSize;

    ASSERT(begin < end);

    const decltype(Zone::id) id { *reinterpret_cast< const decltype(Zone::id)* >(begin) };
    begin += sizeof(id) ;

    ASSERT(begin <= end);

    return { std::make_shared< Zone >(Zone { id, std::move(title), std::move(data) }), begin };
}

std::deque< boost::asio::mutable_buffer > ZoneBuf::buffers()
{
    return std::deque< boost::asio::mutable_buffer > {
        boost::asio::buffer(&null_, sizeof(null_)),
        boost::asio::buffer(&dataSize_, sizeof(dataSize_)),
        boost::asio::buffer(&titleSize_, sizeof(titleSize_))
    };
}

ShmCmdReq::ShmCmdReq(const int zn, const Cmd c, const std::shared_ptr< Zone >& z)
    : zoneNum { zn }, cmd { c }, zone { z }
{ }

ShmCmdAnswer::ShmCmdAnswer(const int zn, const Result r, const ShmCmdReq::Cmd c,
                           std::vector< std::shared_ptr< Zone > >&& zns)
    : zoneNum { zn }, result { r }, cmd { c }, zones { std::move(zns) }
{ }

std::ostream& operator<<(std::ostream& os, const std::shared_ptr< Zone >& zone)
{
    return zone ? (os << *zone) : (os << "null");
}

std::ostream& operator<<(std::ostream& os, const ShmCmdReq::Cmd cmd)
{
    switch (cmd) {
        case ShmCmdReq::Cmd::AddZone: {
            os << "AddZone";
            break;
        }
        case ShmCmdReq::Cmd::DelZone: {
            os << "DelZone";
            break;
        }
        case ShmCmdReq::Cmd::UpdZone: {
            os << "UpdZone";
            break;
        }
        case ShmCmdReq::Cmd::GetZone: {
            os << "GetZone";
            break;
        }
        case ShmCmdReq::Cmd::AduZone: {
            os << "AduZone";
            break;
        }
        case ShmCmdReq::Cmd::GetList: {
            os << "GetList";
            break;
        }
        case ShmCmdReq::Cmd::DropList: {
            os << "DropList";
            break;
        }
        case ShmCmdReq::Cmd::ExitPult: {
            os << "ExitPult";
            break;
        }

        default:
            LogError(STDLOG) << "Unknown enum ShmCmdReq::Cmd: " << cmd;
            os << "Unknown";
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const ShmCmdReq& cmd)
{
    return os << "{ zoneNum: " << cmd.zoneNum
              << ", cmd: " << cmd.cmd
              << ", zone: " << cmd.zone
              << " }";
}

static std::ostream& operator<<(std::ostream& os, const std::shared_ptr< ShmCmdReq >& cmd)
{
    return cmd ? (os << *cmd) : (os << "null");
}

bool operator==(const ShmCmdReq& lhs, const ShmCmdReq& rhs)
{
    return lhs.zoneNum == rhs.zoneNum
        && lhs.cmd == rhs.cmd
        && lhs.zone == rhs.zone;
}

bool operator==(const std::shared_ptr< ShmCmdReq >& lhs, const std::shared_ptr< ShmCmdReq >& rhs)
{
    return (lhs && rhs) ? (*lhs == *rhs) : (lhs.get() == rhs.get());
}

std::ostream& operator<<(std::ostream& os, const ShmReq& req)
{
    return os << "{ owner: " << req.owner
              << ", cmds: " << LogRange(std::cbegin(req.cmds), std::cend(req.cmds))
              << " }";
}
bool operator==(const ShmReq& lhs, const ShmReq& rhs)
{
    return lhs.owner == rhs.owner
        && lhs.cmds == rhs.cmds;
}

bool operator!=(const ShmReq& lhs, const ShmReq& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const ShmCmdAnswer::Result result)
{
    switch (result) {
        case ShmCmdAnswer::Result::Success: {
            os << "Success";
            break;
        }
        case ShmCmdAnswer::Result::InternalServerError: {
            os << "InternalServerError";
            break;
        }
        case ShmCmdAnswer::Result::InvalidRequest: {
            os << "InvalidRequest";
            break;
        }
        case ShmCmdAnswer::Result::InvalidZoneNumber: {
            os << "InvalidZoneNumber";
            break;
        }

        default:
            LogError(STDLOG) << "Unknown enum ShmCmdAnswer::Result: " << result;
            os << "Unknown";
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const ShmCmdAnswer& cmd)
{
    return os << "{ zoneNum: " << cmd.zoneNum
              << ", result: " << cmd.result
              << ", cmd: " << cmd.cmd
              << ", zones: " << LogRange(std::cbegin(cmd.zones), std::cend(cmd.zones))
              << " }";
}

static std::ostream& operator<<(std::ostream& os, const std::shared_ptr< ShmCmdAnswer >& answer)
{
    return answer ? (os << *answer) : (os << "null");
}

bool operator==(const ShmCmdAnswer& lhs, const ShmCmdAnswer& rhs)
{
    return lhs.zoneNum == rhs.zoneNum
        && lhs.result == rhs.result
        && lhs.cmd == rhs.cmd
        && lhs.zones == rhs.zones;
}

bool operator==(const std::shared_ptr< ShmCmdAnswer >& lhs, const std::shared_ptr< ShmCmdAnswer >& rhs)
{
    return (lhs && rhs) ? (*lhs == *rhs) : (lhs.get() < rhs.get());
}

std::ostream& operator<<(std::ostream& os, const ShmAnswer& answer)
{
    return os << "{ owner: " << answer.owner
              << ", answers: " << LogRange(std::cbegin(answer.answers), std::cend(answer.answers))
              << " }";
}

bool operator==(const ShmAnswer& lhs, const ShmAnswer& rhs)
{
    return lhs.owner == rhs.owner
        && lhs.answers == rhs.answers;
}

bool operator!=(const ShmAnswer& lhs, const ShmAnswer& rhs)
{
    return !(lhs == rhs);
}

ShmReqBuf::ShmReqBuf()
    : ownerSize_ { 0 }, owner_ { }, cmdsSize_ { 0 }, cmds_ { }, head_ { }, tail_ { }, all_ { }, zonesBufs_ { }
{
    head_.emplace_back(&ownerSize_, sizeof(ownerSize_));
    head_.emplace_back(&cmdsSize_, sizeof(cmdsSize_));
}

std::shared_ptr< ShmReq > ShmReqBuf::unpack()
{
    std::vector< std::shared_ptr< ShmCmdReq > > cmds { };
    const uint8_t* i { cmds_.data() };
    const uint8_t* end { cmds_.data() + cmdsSize_ };

    while (i != end) {
        ASSERT(i < end);

        const decltype(ShmCmdReq::zoneNum) zoneNum { *reinterpret_cast< const decltype(ShmCmdReq::zoneNum)* >(i) };
        i += sizeof(zoneNum);

        ASSERT(i < end);

        const decltype(ShmCmdReq::cmd) cmd { *reinterpret_cast< const decltype(ShmCmdReq::cmd)* >(i) };
        i += sizeof(cmd);

        ASSERT(i < end);

        auto zoneUnpackResult { ZoneBuf::unpack(i, end) };

        cmds.emplace_back(std::make_shared< ShmCmdReq >(zoneNum, cmd, zoneUnpackResult.first));

        i = zoneUnpackResult.second;

        ASSERT(i <= end);
    }

    return std::make_shared< ShmReq >( ShmReq { std::string { std::cbegin(owner_), std::cend(owner_) }, std::move(cmds) } );
}

const std::deque< boost::asio::mutable_buffer >& ShmReqBuf::head()
{
    return head_;
}

const std::deque< boost::asio::mutable_buffer >& ShmReqBuf::tail()
{
    tail_.clear();

    owner_.resize(ownerSize_);
    cmds_.resize(cmdsSize_);

    tail_.emplace_back(owner_.data(), owner_.size());
    tail_.emplace_back(cmds_.data(), cmds_.size());

    return tail_;
}

std::deque< boost::asio::const_buffer >& ShmReqBuf::all(const ShmReq& req)
{
    all_ = { std::cbegin(head_), std::cend(head_) };
    ownerSize_ = req.owner.size();

    all_.emplace_back(req.owner.data(), req.owner.size());

    cmdsSize_ = 0;
    zonesBufs_.clear();

    for (const auto& c : req.cmds) {
        ASSERT(c);

        all_.emplace_back(&(c->zoneNum), sizeof(c->zoneNum));
        cmdsSize_ += sizeof(c->zoneNum);

        all_.emplace_back(&c->cmd, sizeof(c->cmd));
        cmdsSize_ += sizeof(c->cmd);

        zonesBufs_.emplace_back(ZoneBuf { static_cast< bool >(!c->zone),
                                          c->zone ? c->zone->data.size() : 0,
                                          c->zone ? c->zone->title.size() : 0 });

        for (const auto& b : zonesBufs_.back().buffers()) {
            all_.emplace_back(b);
            cmdsSize_ += boost::asio::buffer_size(b);
        }

        if (c->zone) {
            all_.emplace_back(c->zone->data.data(), c->zone->data.size());
            cmdsSize_ += c->zone->data.size();

            all_.emplace_back(c->zone->title.data(), c->zone->title.size());
            cmdsSize_ += c->zone->title.size();

            all_.emplace_back(&(c->zone->id), sizeof(c->zone->id));
            cmdsSize_ += sizeof(c->zone->id);
        }
    }

    return all_;
}

ShmAnswerBuf::ShmAnswerBuf()
    : ownerSize_ { 0 }, owner_ { }, answersSize_ { 0 }, answers_ { }, head_ { },
      tail_ { }, all_ { }, zonesSizes_ { }, zonesBufs_ { }
{
    head_.emplace_back(&ownerSize_, sizeof(ownerSize_));
    head_.emplace_back(&answersSize_, sizeof(answersSize_));
}

std::shared_ptr< ShmAnswer > ShmAnswerBuf::unpack()
{
    std::vector< std::shared_ptr < ShmCmdAnswer > > answers { };

    const uint8_t* i { answers_.data() };
    const uint8_t* end { answers_.data() + answersSize_ };

    while (i != end) {
        ASSERT(i < end);

        const decltype(ShmCmdAnswer::zoneNum) zoneNum { *reinterpret_cast< const decltype(ShmCmdAnswer::zoneNum)* >(i) };
        i += sizeof(zoneNum);

        ASSERT(i < end);

        const decltype(ShmCmdAnswer::result) result { *reinterpret_cast< const decltype(ShmCmdAnswer::result)* >(i) };
        i += sizeof(result);

        ASSERT(i < end);

        const decltype(ShmCmdAnswer::cmd) cmd { *reinterpret_cast< const decltype(ShmCmdAnswer::cmd)* >(i) };
        i += sizeof(cmd);

        ASSERT(i < end);

        const decltype(zonesSizes_)::value_type zonesSize { *reinterpret_cast< const decltype(zonesSizes_)::value_type* >(i) };
        i += sizeof(zonesSize);

        ASSERT(i <= end);

        const uint8_t* zonesEnd { i + zonesSize};
        std::vector< std::shared_ptr< Zone > > zones { };

        while (i != zonesEnd) {
            auto zoneUnpackResult { ZoneBuf::unpack(i, zonesEnd) };

            zones.emplace_back(zoneUnpackResult.first);

            i = zoneUnpackResult.second;

            ASSERT(i <= end);
        }

        answers.emplace_back(std::make_shared< ShmCmdAnswer > (zoneNum, result, cmd, std::move(zones)));

        ASSERT(i <= end);
    }

    return std::make_shared< ShmAnswer >( ShmAnswer { std::string { std::cbegin(owner_), std::cend(owner_) }, std::move(answers) } );
}

const std::deque< boost::asio::mutable_buffer >& ShmAnswerBuf::head()
{
    return head_;
}

const std::deque< boost::asio::mutable_buffer >& ShmAnswerBuf::tail()
{
    tail_.clear();

    owner_.resize(ownerSize_);
    answers_.resize(answersSize_);

    tail_.emplace_back(owner_.data(), owner_.size());
    tail_.emplace_back(answers_.data(), answers_.size());

    return tail_;
}

std::deque< boost::asio::const_buffer >& ShmAnswerBuf::all(const ShmAnswer& answer)
{
    all_ = { std::cbegin(head_), std::cend(head_) };
    ownerSize_ = answer.owner.size();
    all_.emplace_back(answer.owner.data(), answer.owner.size());

    answersSize_ = 0;
    zonesBufs_.clear();
    zonesSizes_.clear();

    for (const auto& a : answer.answers) {
        all_.emplace_back(&(a->zoneNum), sizeof(a->zoneNum));
        answersSize_ += sizeof(a->zoneNum);

        all_.emplace_back(&a->result, sizeof(a->result));
        answersSize_ += sizeof(a->result);

        all_.emplace_back(&a->cmd, sizeof(a->cmd));
        answersSize_ += sizeof(a->cmd);

        zonesSizes_.emplace_back(0);
        all_.emplace_back(&(zonesSizes_.back()), sizeof(zonesSizes_.back()));

        answersSize_ += sizeof(zonesSizes_.back());

        for (const auto& z : a->zones) {
            zonesBufs_.emplace_back(ZoneBuf { static_cast< bool >(!z),
                                              z ? z->data.size() : 0,
                                              z ? z->title.size() : 0 });

            for (const auto& b : zonesBufs_.back().buffers()) {
                all_.emplace_back(b);
                zonesSizes_.back() += boost::asio::buffer_size(b);
            }

            if (z) {
                all_.emplace_back(z->data.data(), z->data.size());
                zonesSizes_.back() += z->data.size();

                all_.emplace_back(z->title.data(), z->title.size());
                zonesSizes_.back() += z->title.size();

                all_.emplace_back(&(z->id), sizeof(z->id));
                zonesSizes_.back() += sizeof(z->id);
            }
        }

        answersSize_ += zonesSizes_.back();
    }

    return all_;
}

static void process(ShmReq& req, ShmAnswer& answer)
{
    LogTrace(TRACE5) << __FUNCTION__;

    InitLogTime((req.owner + "_SHMSRV").c_str());

    static std::unordered_map< std::string, ZoneList > htab;
    ZoneList& zl = htab.emplace(req.owner, ZoneList { req.owner, { } }).first->second;


    LogTrace(TRACE5) << "req: " << req;

    for (const auto& cmd : req.cmds) {
        ASSERT(cmd);

        LogTrace(TRACE5) << "cmd: " << *cmd;

        answer.answers.emplace_back(std::make_shared< ShmCmdAnswer >(cmd->zoneNum,
                                                                     ShmCmdAnswer::Result::Success,
                                                                     cmd->cmd,
                                                                     std::vector< std::shared_ptr< Zone > > { }));

        switch (cmd->cmd) {
            case ShmCmdReq::Cmd::AddZone: {
                if (cmd->zone) {
                    if (!zl.zones.emplace(cmd->zone).second) {
                        answer.answers.back()->result = ShmCmdAnswer::Result::InvalidZoneNumber;
                        LogError(STDLOG) << "ADD_ZONE failed: " <<  answer.answers.back()->result;
                    }
                } else {
                    answer.answers.back()->result = ShmCmdAnswer::Result::InvalidRequest;
                    LogError(STDLOG) << "ADD_ZONE without zone.";
                }

                break;
            } // AddZone

            case ShmCmdReq::Cmd::DelZone: {
                auto pp { zl.zones.find(cmd->zoneNum) };
                if (std::cend(zl.zones) != pp) {
                    zl.zones.erase(pp);
                } else {
                    answer.answers.back()->result = ShmCmdAnswer::Result::InvalidZoneNumber;
                    LogError(STDLOG) << "DEL_ZONE failed: " << answer.answers.back()->result;
                }

                break;
            } // DelZone

            case ShmCmdReq::Cmd::UpdZone: {
                if (cmd->zone) {
                    auto pp = zl.zones.find(cmd->zone->id);
                    if (std::cend(zl.zones) != pp) {
                        auto z { *pp };

                        z->data = cmd->zone->data;
                        z->title = cmd->zone->title;

                        zl.zones.erase(pp);
                        zl.zones.emplace(z);
                    } else {
                        answer.answers.back()->result = ShmCmdAnswer::Result::InvalidZoneNumber;
                        LogError(STDLOG) << "Zone N " << cmd->zoneNum << " not found.";
                    }
                } else {
                    answer.answers.back()->result = ShmCmdAnswer::Result::InvalidRequest;
                    LogError(STDLOG) << "UPD_ZONE without zone.";
                }

                break;
            } // UpdZone

            case ShmCmdReq::Cmd::AduZone: {
                if (cmd->zone) {
                    auto pp { zl.zones.find(cmd->zone->id) };
                    if (std::cend(zl.zones) != pp) {
                        auto z { *pp };

                        z->data = cmd->zone->data;
                        z->title = cmd->zone->title;

                        zl.zones.erase(pp);
                        zl.zones.emplace(z);
                    } else {
                        zl.zones.emplace(cmd->zone);
                    }
                } else {
                    answer.answers.back()->result = ShmCmdAnswer::Result::InvalidRequest;
                    LogError(STDLOG) << "UPD_ZONE without zone.";
                }

                break;
            } // AduZone

            case ShmCmdReq::Cmd::GetZone: {
                auto zone { zl.zones.find(cmd->zoneNum) };
                if (std::cend(zl.zones) != zone) {
                    answer.answers.back()->zones.emplace_back(*zone);
                } else {
                    answer.answers.back()->result = ShmCmdAnswer::Result::InvalidZoneNumber;
                    LogError(STDLOG) << "Zone N " << cmd->zoneNum << " not found.";
                }

                break;
            } // GetZone

            case ShmCmdReq::Cmd::GetList: {
                const auto before { std::end(answer.answers.back()->zones) };
                answer.answers.back()->zones.insert(before, std::cbegin(zl.zones), std::cend(zl.zones));

                break;
            } // GetList

            case ShmCmdReq::Cmd::ExitPult:
            case ShmCmdReq::Cmd::DropList: {
                zl.zones.clear();
                break;
            } // DropList

            default: {
                LogError(STDLOG) << "Unknown command: " << cmd->cmd;
            }
        }
    }
}

class Shmsrv
{
    typedef boost::asio::local::stream_protocol::socket SocketType;
private:
    class Connection
        : public boost::enable_shared_from_this<Shmsrv::Connection>
    {
    public:
        explicit Connection(boost::asio::io_service& service)
            : buf_(), socket_(service)
        { }

        Connection(const Connection& ) = delete;
        Connection& operator=(const Connection& ) = delete;

        Connection(Connection&& ) = delete;
        Connection& operator=(Connection&& ) = delete;

        ~Connection()
        {
            boost::system::error_code error;
            socket_.shutdown(Shmsrv::SocketType::shutdown_both, error);
            LogTrace(TRACE1) << __FUNCTION__ << ": " << error << ": " << error.message();
        }

        SocketType& socket() {
            return socket_;
        }

        void start() {
            boost::asio::async_read(socket_, buf_.head(),
                    boost::bind(&Shmsrv::Connection::handleReadHead, shared_from_this(), boost::asio::placeholders::error));
        }

    private:
        void handleReadHead(const boost::system::error_code& e) {
            if (!e) {
                boost::asio::async_read(socket_, buf_.tail(),
                        boost::bind(&Shmsrv::Connection::handleReadTail, shared_from_this(), boost::asio::placeholders::error));
            } else {
                LogTrace(TRACE1) << __FUNCTION__ << " error: " << e << ": " << e.message();
            }
        }

        void handleReadTail(const boost::system::error_code& e) {
            if (!e) {
                monitor_beg_work();

                auto req { buf_.unpack() };

                ShmAnswer answer { "", { } };

                if (req) {
                    answer.owner = req->owner;
                    process(*req, answer);
                } else {
                    LogError(STDLOG) << __FUNCTION__ << " unpack request failed.";
                }

                LogTrace(TRACE5) << "answer: " << answer;

                ShmAnswerBuf ansBuf { };
                boost::system::error_code error { };

                boost::asio::write(socket_, ansBuf.all(answer), boost::asio::transfer_all(), error);
                if (error) {
                    LogError(STDLOG) << "boost::asio::write failed: " << error << ": " << error.message();
                    return;
                }

                start();

                monitor_idle_zapr_type(1, QUEPOT_NULL);
            } else {
                LogTrace(TRACE1) << __FUNCTION__ << " error: " << e << ": " << e.message();
            }
        }
    private:
        ShmReqBuf buf_;
        SocketType socket_;
    };

public:
    Shmsrv(int controlPipe, const char* addr);

    Shmsrv(const Shmsrv& ) = delete;
    Shmsrv& operator=(const Shmsrv& ) = delete;

    Shmsrv(Shmsrv&& ) = delete;
    Shmsrv& operator=(Shmsrv&& ) = delete;

    ~Shmsrv();

    void run() {
        ServerFramework::Run();
    }

private:
    void accept();

    void handleAccept(const boost::system::error_code& e);

private:
    boost::asio::local::stream_protocol::acceptor acceptor_;
    boost::shared_ptr<Connection> newConnection_;
    ServerFramework::ControlPipeEvent control_;
};

Shmsrv::Shmsrv(int controlPipe, const char* addr) :
    acceptor_(ServerFramework::system_ios::Instance()),
    newConnection_(),
    control_()
{
    acceptor_.open(boost::asio::local::stream_protocol());
    acceptor_.set_option(boost::asio::local::stream_protocol::acceptor::reuse_address(true));
    acceptor_.bind(boost::asio::local::stream_protocol::endpoint(addr));
    acceptor_.listen();
    accept();
}

Shmsrv::~Shmsrv()
{ }

void Shmsrv::accept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    newConnection_.reset(new Connection(ServerFramework::system_ios::Instance()));
    acceptor_.async_accept(newConnection_->socket(),
                                 boost::bind(&Shmsrv::handleAccept,
                                             this,
                                             boost::asio::placeholders::error
                                 )
    );
}

void Shmsrv::handleAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (!e) {
        newConnection_->start();
    } else {
        LogError(STDLOG) << __FUNCTION__ << "(" << e << "): " << e.message();
    }
    accept();
}

static void on_term(int a)
{
    static int first = 1;

    if (a == SIGPIPE) {
        ProgError(STDLOG, "SIGPIPE caught");
        return;
    }

    WriteLog(STDLOG,
            (a == SIGINT || a == -1) ?
            "Received signal - %d, exiting ..." : "Killed in action - %d " , a);
    if (a != SIGINT) {
        avost = 1;
    }
    if (a != SIGINT && a != SIGTERM) {
        monitor_restart();
    }
    tst();
    if (first) {
        tst();
        first = 0;
        tst();
        longjmp(termpoint, 1);
    }
    tst();
}

int main_shmserv(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    char addr[30];
    Tcl_Obj* obj = NULL;
    struct sigaction sigact;
    sigset_t sigset;
    sigemptyset(&sigset);
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_mask = sigset;


    const int shm_num = boost::lexical_cast<int>(argv[1]);
    if ((obj = Tcl_ObjGetVar2(getTclInterpretator(),
                    Tcl_NewStringObj("SHMSERV", -1), 0, TCL_GLOBAL_ONLY |
                    TCL_LEAVE_ERR_MSG)) == 0) {
        printf("ERROR:main_shmserv wrong parameter SHMSERV:%s\n",
                Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
        return 1;
    }

    sprintf(addr, "%s-%d", Tcl_GetString(obj), shm_num);

    InitLogTime("SHMSRV");

    WriteLog(STDLOG, "Shared memory server started at %s",
             boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()).c_str());

    monitor_idle();
    monitor_special();

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    if (setjmp(termpoint) == 0) {
        sigact.sa_handler = on_term;
        if (sigaction(SIGINT, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGINT");
        }
        sigact.sa_handler = on_term;
        if (sigaction(SIGPIPE, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGPIPE");
        }
        sigact.sa_handler = on_term;
        if (sigaction(SIGTERM, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGTERM");
        }
        sigact.sa_handler = sigusr2;
        if (sigaction(SIGUSR2, &sigact, 0) < 0) {
            ProgError(STDLOG, "Sigaction failed - SIGUSR2");
        }

        if (!getenv("BROKEN_GDB"))      {
            sigact.sa_handler = on_term;
            if (sigaction(SIGSEGV, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGSEGV");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGBUS, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGBUS");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGFPE, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGFPE");
            }
            sigact.sa_handler = on_term;
            if (sigaction(SIGILL, &sigact, 0) < 0) {
                ProgError(STDLOG, "Sigaction failed - SIGILL");
            }
        }
        runLoop(supervisorSocket, addr);
    }

    if (!avost) {
        return 0;
    }

    return 1;
}

static void runLoop(int controlPipe, char* addr)
{
    unlink(addr);

    Shmsrv srv(controlPipe, addr);
    srv.run();
}

//Client part

class ShmZonesClientMng
{
public:
    using Zones = std::set< std::shared_ptr< Zone >, std::less<> >;

private:
    struct StashedBatch
    {
        std::set< int > created;
        std::set< int > updated;
        std::set< int > deleted;
        std::shared_ptr< std::set< std::shared_ptr< Zone >, std::less<> > > zones;
        std::map< std::string, std::shared_ptr< Zones > > tstZones;

        StashedBatch()
            : created { }, updated { }, deleted { }, zones { }
        { }

        StashedBatch(const StashedBatch& ) = delete;
        StashedBatch& operator=(const StashedBatch& ) = delete;

        StashedBatch(StashedBatch&& ) = delete;
        StashedBatch& operator=(StashedBatch&& ) = delete;
    };

    struct SocketWithCreationTime
    {
        boost::posix_time::ptime creationTime;
        std::shared_ptr< boost::asio::local::stream_protocol::socket > socket;
    };

public:
    static ShmZonesClientMng& Instance();

    void owner(const std::string& who) {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << who;

        owner_ = who;
        reset();
    }

    std::shared_ptr< boost::asio::local::stream_protocol::socket > getShmSrvSocket(const std::size_t shmSrvNum);

    void removeShmSrvSocket(const std::size_t shmSrvNum);

    std::shared_ptr< Zone > makeZone(const int zoneNum);

    std::shared_ptr< const Zone > getZone(const int zoneNum);

    std::shared_ptr< Zone > getZoneForUpdate(const int zoneNum);

    int updateZoneData(const int zoneNum, const size_t offset, const uint8_t* const data, const size_t len);

    int getZoneData(const int zoneNum, uint8_t* const data, const size_t offset, const size_t len);

    int resizeZone(const int zoneNum, const size_t len);

    int removeZone(const int zoneNum);

    int removeAllZones(const std::string& owner);

    void stash();

    void pop();

    void sync();

    void reset();

private:
    ShmZonesClientMng()
        : owner_ { }, created_ { }, updated_ { }, deleted_ { }, stash_ { }, zones_ { }, sockets_ { }
#ifdef XP_TESTING
          , tstZones_ { }
#endif // XP_TESTING
    { }

    bool initialized();

    bool init();


private:
    std::string owner_ { };
    std::set< int > created_;
    std::set< int > updated_;
    std::set< int > deleted_;
    std::deque< StashedBatch > stash_;
    std::shared_ptr< Zones > zones_;
    std::map< std::size_t, SocketWithCreationTime > sockets_;
#ifdef XP_TESTING
    std::map< std::string, std::shared_ptr< ShmZonesClientMng::Zones > > tstZones_;
#endif // XP_TESTING
};

ShmZonesClientMng& ShmZonesClientMng::Instance()
{
    static ShmZonesClientMng mng { }; // Singletone lifetime?
    return mng;
}

static unsigned getShmSrvNum(const std::string& who)
{
    static const auto shmSrvCount { readIntFromTcl("SHMSERV_NUM", 1) };
    static std::hash< std::string > hf { };

    return hf(who) % shmSrvCount;
}

static std::shared_ptr< boost::asio::local::stream_protocol::socket> getConnectedSocket(const std::string& path)
{
    boost::system::error_code ec { };
    boost::asio::local::stream_protocol::endpoint ep { path };
    boost::asio::local::stream_protocol::socket sock { ServerFramework::system_ios::Instance() };

    unsigned tryCount { 5 + 1 };
    while (--tryCount) { /* conect to address */
        sock.connect(ep, ec);
        if (!ec) {
            break;
        }

        LogTrace(TRACE0) << __FUNCTION__ << ": connection attempt to " << ep << " failed: (" << ec << "): " << ec.message();
        random_sleep();
    }

    return ec
        ? std::shared_ptr< boost::asio::local::stream_protocol::socket > { }
        : std::make_shared< boost::asio::local::stream_protocol::socket >(std::move(sock));
}

static bool expired(const std::shared_ptr< boost::asio::local::stream_protocol::socket >& socket, boost::posix_time::ptime& creationTime)
{
    static const auto TIMEOUT { boost::posix_time::seconds(readIntFromTcl("SHMSERV_CONNECTION_TIMEOUT", 8)) };
    return !socket && (TIMEOUT < (boost::posix_time::microsec_clock::local_time() - creationTime));
}

std::shared_ptr< boost::asio::local::stream_protocol::socket > ShmZonesClientMng::getShmSrvSocket(const std::size_t shmSrvNum)
{
    auto s { sockets_.find(shmSrvNum) };
    if ((std::cend(sockets_) == s) || expired(s->second.socket, s->second.creationTime)) {
        if (std::cend(sockets_) != s) {
            sockets_.erase(s);
        }

        static const auto cmdBase { readStringFromTcl("SHMSERV", "") };

        SocketWithCreationTime socket {
            boost::posix_time::microsec_clock::local_time(),
            getConnectedSocket(cmdBase + '-' + std::to_string(shmSrvNum))
        };

        s = sockets_.emplace(shmSrvNum, socket).first;
    }

    return s->second.socket;
}

void ShmZonesClientMng::removeShmSrvSocket(const std::size_t shmSrvNum)
{
    sockets_.erase(shmSrvNum);
    ShmZonesClientMng::reset();
}

static std::shared_ptr< ShmAnswer > writeRequestReadAnswer(ShmReq& req)
{
    auto shmSrvNum { getShmSrvNum(req.owner) };
    auto socket { ShmZonesClientMng::Instance().getShmSrvSocket(shmSrvNum) };
    if (!socket) {
        LogTrace(TRACE1) << "Invalid socket to shmsrv number " << shmSrvNum;
        return std::shared_ptr< ShmAnswer > { };
    }

    ShmReqBuf reqBuf { };
    boost::system::error_code error { };
    boost::asio::write(*socket, reqBuf.all(req), boost::asio::transfer_all(), error);
    if (error) {
        boost::system::error_code ec { };
        if (boost::asio::error::eof != error) {
            LogError(STDLOG) << "boost::asio::write to socket: " << socket->remote_endpoint(ec)
                             << " failed: " << error << ": " << error.message();
        } else {
            LogTrace(TRACE1) << "boost::asio::write to socket: " << socket->remote_endpoint(ec)
                             << " failed: " << error << ": " << error.message();
        }

        ShmZonesClientMng::Instance().removeShmSrvSocket(shmSrvNum);

        return std::shared_ptr< ShmAnswer > { };
    }

    ShmAnswerBuf answerBuf { };
    boost::asio::read(*socket, answerBuf.head(), boost::asio::transfer_all(), error);
    if (error) {
        boost::system::error_code ec { };
        if (boost::asio::error::eof != error) {
            LogError(STDLOG) << "boost::asio::read from socket: " << socket->remote_endpoint(ec)
                             << " failed with error: " << error << ": " << error.message();
        } else {
            LogTrace(TRACE1) << "boost::asio::read from socket: " << socket->remote_endpoint(ec)
                             << " failed with error: " << error << ": " << error.message();
        }

        ShmZonesClientMng::Instance().removeShmSrvSocket(shmSrvNum);

        return std::shared_ptr< ShmAnswer > { };
    }

    boost::asio::read(*socket, answerBuf.tail(), boost::asio::transfer_all(), error);
    if (error) {
        boost::system::error_code ec { };
        if (boost::asio::error::eof != error) {
            LogError(STDLOG) << "boost::asio::read from socket: " << socket->remote_endpoint(ec)
                             << " failed with error: " << error << ": " << error.message();
        } else {
            LogTrace(TRACE1) << "boost::asio::read from socket: " << socket->remote_endpoint(ec)
                             << " failed with error: " << error << ": " << error.message();
        }

        ShmZonesClientMng::Instance().removeShmSrvSocket(shmSrvNum);

        return std::shared_ptr< ShmAnswer > { };
    }

    return answerBuf.unpack();
}

static std::shared_ptr< ShmZonesClientMng::Zones > getZoneList(const std::string& owner)
{
    LogTrace(TRACE5) << __FUNCTION__;

    ShmReq req { owner, { std::make_shared< ShmCmdReq >(-1, ShmCmdReq::Cmd::GetList, std::shared_ptr< Zone > { }) } };

    LogTrace(TRACE5) << "Request to shmsrv: " << req;

    auto answer { writeRequestReadAnswer(req) };
    if (!answer) {
        LogError(STDLOG) << "writeRequestReadAnswer failed.";
        return std::shared_ptr< ShmZonesClientMng::Zones > { };
    }

    LogTrace(TRACE5) << "Answer from shmsrv: " << *answer;

    ASSERT(owner == answer->owner);
    ASSERT(1 == answer->answers.size());
    ASSERT(answer->answers.front());

    return std::make_shared< ShmZonesClientMng::Zones >(std::cbegin(answer->answers.front()->zones),
                                                        std::cend(answer->answers.front()->zones));
}

bool ShmZonesClientMng::initialized()
{
    return static_cast< bool >(zones_);
}

bool ShmZonesClientMng::init()
{
    if (initialized()) {
        return true;
    }

#ifdef XP_TESTING
    if (inTestMode()) {
        LogTrace(TRACE5) << __FUNCTION__ << ": in test mode.";

        zones_ = tstZones_.emplace(owner_, std::make_shared< ShmZonesClientMng::Zones >()).first->second;

        return true;
    }
#endif // XP_TESTING

    LogTrace(TRACE5) << __FUNCTION__;

    reset();

    zones_ = getZoneList(owner_);

    return initialized();
}

void ShmZonesClientMng::reset()
{
    LogTrace(TRACE5) << __FUNCTION__;

    zones_.reset();
    created_.clear();
    updated_.clear();
    deleted_.clear();
}

std::shared_ptr< Zone > ShmZonesClientMng::makeZone(const int zoneNum)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return std::shared_ptr< Zone > { };
    }

    const bool wasRemoved { 0 < deleted_.erase(zoneNum) };

    auto z { zones_->emplace(std::make_shared< Zone >(Zone { zoneNum, { }, { } })) };

    if (!wasRemoved && z.second) {
        created_.emplace(zoneNum);
    } else {
        updated_.emplace(zoneNum);
    }

    return *(z.first);
}

std::shared_ptr< const Zone > ShmZonesClientMng::getZone(const int zoneNum)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return std::shared_ptr< Zone > { };
    }

    auto z { zones_->find(zoneNum) };

    return (std::cend(*zones_) != z) ? *z : std::shared_ptr< const Zone > { };
}

std::shared_ptr< Zone > ShmZonesClientMng::getZoneForUpdate(const int zoneNum)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return std::shared_ptr< Zone > { };
    }

    auto z { zones_->find(zoneNum) };
    if (std::cend(*zones_) == z) {
        return std::shared_ptr< Zone > { };
    }

    if (0 == created_.count(zoneNum)) {
        updated_.emplace(zoneNum);
    }

    return *z;
}

int ShmZonesClientMng::updateZoneData(const int zoneNum, const size_t offset, const uint8_t* const data, const size_t len)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return -1;
    }

    auto zIt { zones_->find(zoneNum) };
    if (std::cend(*zones_) == zIt) {
        LogError(STDLOG) << "Invalid zone number: " << zoneNum;
        return -1;
    }

    Zone& z { **zIt };

    if (z.data.size() < (offset + len)) {
        z.data.resize(offset + len);
    }

    std::copy(data, data + len, z.data.data() + offset);

    if (0 == created_.count(z.id)) {
        updated_.emplace(z.id);
    }

    return 0;
}

int ShmZonesClientMng::getZoneData(const int zoneNum, uint8_t* const data, const size_t offset, const size_t len)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return -1;
    }

    auto zIt { zones_->find(zoneNum) };
    if (std::cend(*zones_) == zIt) {
        LogError(STDLOG) << "Invalid zone number: " << zoneNum;
        return -1;
    }

    Zone& z { **zIt };

    if (z.data.size() < (offset + len)) {
        LogError(STDLOG) << "Invalid length: " << len << ", zone data size: " << z.data.size() << ", offset: " << offset;
        return -1;
    }

    const auto begin { std::cbegin(z.data) + offset };
    const auto end { begin + len };

    std::copy(begin, end, reinterpret_cast< uint8_t* >(data));

    return 0;
}

int ShmZonesClientMng::resizeZone(const int zoneNum, const size_t len)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return -1;
    }

    auto zIt { zones_->find(zoneNum) };
    if (std::cend(*zones_) == zIt) {
        LogError(STDLOG) << "Invalid zone number: " << zoneNum;
        return -1;
    }

    Zone& z { **zIt };

    z.data.resize(len);

    if (0 == created_.count(z.id)) {
        updated_.emplace(z.id);
    }

    return 0;
}

int ShmZonesClientMng::removeZone(const int zoneNum)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << zoneNum;

    if (!init()) {
        LogTrace(TRACE0) << "Shmsrv is not available.";
        return -1;
    }

    auto zIt { zones_->find(zoneNum) };
    if (std::cend(*zones_) == zIt) {
        LogTrace(TRACE5) << "Invalid zone number: " << zoneNum;
        return -1;
    }

    Zone& z { **zIt };

    if (0 == created_.erase(z.id)) {
        deleted_.emplace(z.id);
    }

    updated_.erase(z.id);


    zones_->erase(zIt);

    return 0;
}

int ShmZonesClientMng::removeAllZones(const std::string& owner)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << owner;

    if (inTestMode()) {
        LogTrace(TRACE5) << __FUNCTION__ << ": Test zones have been reset.";

#ifdef XP_TESTING
        tstZones_.erase(owner);
#endif // XP_TESTING

        ShmZonesClientMng::Instance().reset();

        return 0;
    }

    ShmReq req { owner, { std::make_shared< ShmCmdReq >(-1, ShmCmdReq::Cmd::DropList, std::shared_ptr< Zone > { }) } };

    LogTrace(TRACE5) << "Request to shmsrv: " << req;

    auto answer { writeRequestReadAnswer(req) };
    if (!answer) {
        LogError(STDLOG) << "writeRequestReadAnswer failed.";
        return -1;
    }

    LogTrace(TRACE5) << "Answer from shmsrv: " << *answer;

    ASSERT(owner == answer->owner);
    ASSERT(1 == answer->answers.size());
    ASSERT(answer->answers.front());

    if (answer->answers.front()->result != ShmCmdAnswer::Result::Success) {
        LogTrace(TRACE1) << "Cmd '" << answer->answers.front()->cmd
                         << "' failed: " << answer->answers.front()->result;
        return -1;
    }

    if (owner_ == owner) {
        ShmZonesClientMng::reset();
    }

    return 0;
}

void ShmZonesClientMng::stash()
{
    stash_.emplace_back();

    stash_.back().zones.swap(zones_);
    stash_.back().created.swap(created_);
    stash_.back().updated.swap(updated_);
    stash_.back().deleted.swap(deleted_);
#ifdef XP_TESTING
    stash_.back().tstZones.swap(tstZones_);
#endif // XP_TESTING
}

void ShmZonesClientMng::pop()
{
    if (stash_.empty()) {
        LogError(STDLOG) << "ShmZonesclientMng::pop call without stashed zones.";
        return;
    }

    zones_.swap(stash_.back().zones);
    created_.swap(stash_.back().created);
    updated_.swap(stash_.back().updated);
    deleted_.swap(stash_.back().deleted);
#ifdef XP_TESTING
    tstZones_.swap(stash_.back().tstZones);
#endif // XP_TESTING

    stash_.pop_back();
}

void ShmZonesClientMng::sync()
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (!initialized()) {
        LogTrace(TRACE5) << "ShmZonesClientMng::sync without zones.";
        return;
    }

    ShmReq req { owner_, { } };

    std::set < int > createdAndDeleted { };
    std::set_intersection(std::cbegin(created_), std::cend(created_),
                          std::cbegin(deleted_), std::cend(deleted_),
                          std::inserter(createdAndDeleted, std::end(createdAndDeleted)));

    std::set< int > createdZones { };
    std::set_difference(std::cbegin(created_), std::cend(created_),
                        std::cbegin(createdAndDeleted), std::cend(createdAndDeleted),
                        std::inserter(createdZones, std::end(createdZones)));

    std::set< int > deletedZones { };
    std::set_difference(std::cbegin(deleted_), std::cend(deleted_),
                        std::cbegin(createdAndDeleted), std::cend(createdAndDeleted),
                        std::inserter(deletedZones, std::end(deletedZones)));

    std::set< int > updatedZones { };
    std::set_difference(std::cbegin(updated_), std::cend(updated_),
                        std::cbegin(createdAndDeleted), std::cend(createdAndDeleted),
                        std::inserter(updatedZones, std::end(updatedZones)));

    for (const auto& zn : createdZones) {
        auto z { zones_->find(zn) };

        ASSERT(std::cend(*zones_) != z);

        req.cmds.emplace_back(std::make_shared< ShmCmdReq >((*z)->id, ShmCmdReq::Cmd::AddZone, *z));
    }

    for (const auto& zn : updatedZones) {
        auto z { zones_->find(zn) };

        ASSERT(std::cend(*zones_) != z);

        req.cmds.emplace_back(std::make_shared< ShmCmdReq >((*z)->id, ShmCmdReq::Cmd::UpdZone, *z));
    }

    for (const auto& zn : deletedZones) {
        req.cmds.emplace_back(std::make_shared< ShmCmdReq >(zn, ShmCmdReq::Cmd::DelZone, std::shared_ptr< Zone > { }));
    }

    if (req.cmds.empty()) {
        LogTrace(TRACE5) << __FUNCTION__ << "Update nothing.";
        return;
    }

    LogTrace(TRACE5) << __FUNCTION__ << " : request: " << req;

    auto answer { writeRequestReadAnswer(req) };
    if (!answer) {
        LogError(STDLOG) << "writeRequestReadAnswer failed.";
        return;
    }

    LogTrace(TRACE5) << __FUNCTION__ << " : answer: " << *answer;
}

void InitZoneList(const char* pu)
{
    LogTrace(TRACE1) << __FUNCTION__ <<  ": " << pu;

    ShmZonesClientMng::Instance().owner(pu);
}

Zone* NewZone(const int zoneNum, const size_t len, const char* const head)
{
    auto z { ShmZonesClientMng::Instance().makeZone(zoneNum) };
    if (!z) {
        return nullptr;
    }

    z->title = head;
    z->data.resize(len);

    return z.get();
}

Zone* NewZone(const int zoneNum, std::vector< uint8_t >&& data, std::string&& head)
{
    auto z { ShmZonesClientMng::Instance().makeZone(zoneNum) };
    if (!z) {
        return nullptr;
    }

    z->title = std::move(head);
    z->data = std::move(data);

    return z.get();
}

const Zone* getZonePtr(const int zoneNum)
{
    return ShmZonesClientMng::Instance().getZone(zoneNum).get();
}

Zone* getZonePtrForUpdate(const int zoneNum)
{
    return ShmZonesClientMng::Instance().getZoneForUpdate(zoneNum).get();
}

int setZoneData(const Zone* const zone, const void* const data, const size_t start, const size_t len)
{
    if (nullptr == zone) {
        LogError(STDLOG) << "Invalid zone: " << zone;
        return -1;
    }

    return ShmZonesClientMng::Instance().updateZoneData(zone->id, start, reinterpret_cast< const uint8_t* >(data), len);
}

int setZoneData2(const Zone* const zone, const void* const data, const size_t size, const size_t index, const size_t count)
{
    if (nullptr == zone) {
        LogError(STDLOG) << "Invalid zone: " << zone;
        return -1;
    }

    const size_t offset { index * size };
    const size_t length { count * size };

    return ShmZonesClientMng::Instance().updateZoneData(zone->id, offset, reinterpret_cast< const uint8_t* >(data), length);
}

int getZoneData(const Zone* const zone, void* const data, const size_t start, const size_t len)
{
    if (nullptr == zone) {
        LogError(STDLOG) << "Invalide zone: " << zone;
        return -1;
    }

    return ShmZonesClientMng::Instance().getZoneData(zone->id, reinterpret_cast< uint8_t* >(data), start, len);
}

int getZoneData2(const Zone* const zone, void* const data, const size_t size, const size_t index, const size_t count)
{
    if (nullptr == zone) {
        LogError(STDLOG) << "Invalide zone: " << zone;
        return -1;
    }

    return ShmZonesClientMng::Instance().getZoneData(zone->id, reinterpret_cast< uint8_t* >(data), index * size, count * size);
}

int ReallocZone(const Zone* const zone, const size_t len)
{
    if (nullptr == zone) {
        LogError(STDLOG) << "Invalid zone: " << zone;
        return -1;
    }

    return ShmZonesClientMng::Instance().resizeZone(zone->id, len);
}

int DelZone(const int zoneNum)
{
    return ShmZonesClientMng::Instance().removeZone(zoneNum);
}

void DropZoneList()
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (!inTestMode()) {
        ShmZonesClientMng::Instance().reset();
    }
}

void SaveZoneList()
{
    LogTrace(TRACE5) << __FUNCTION__;

    ShmZonesClientMng::Instance().stash();
}

void RestoreZoneList()
{
    LogTrace(TRACE5) << __FUNCTION__;

    ShmZonesClientMng::Instance().pop();
}

void UpdateZoneList(const char* const pu)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": " << pu;

    if (!inTestMode()) {
        ShmZonesClientMng::Instance().sync();

        ShmZonesClientMng::Instance().reset();
    }
}

int DropAllAreas(const char* const owner)
{
    LogTrace(TRACE5) << __FUNCTION__;

    return ShmZonesClientMng::Instance().removeAllZones(owner);
}

#ifdef XP_TESTING
#include "checkunit.h"

static std::shared_ptr< ShmReq > packUnpackReq(const ShmReq& req)
{
    LogTrace(TRACE5) << __FUNCTION__ << " : " << req;

    ShmReqBuf sendBuf { };

    std::vector< uint8_t > raw { };
    for (const auto& b : sendBuf.all(req)) {
        raw.insert(std::end(raw), boost::asio::buffer_cast< const uint8_t* >(b),
                                  boost::asio::buffer_cast< const uint8_t* >(b) + boost::asio::buffer_size(b));
    }

    ShmReqBuf receiveBuf { };

    std::size_t offset { 0 };
    for (const auto& b : receiveBuf.head()) {
        auto begin { std::cbegin(raw) + offset };
        auto end { begin + boost::asio::buffer_size(b) };

        std::copy(begin, end, boost::asio::buffer_cast< uint8_t* >(b));

        offset += boost::asio::buffer_size(b);
    }

    for (const auto& b : receiveBuf.tail()) {
        auto begin { std::cbegin(raw) + offset };
        auto end { begin + boost::asio::buffer_size(b) };

        std::copy(begin, end, boost::asio::buffer_cast< uint8_t* >(b));

        offset += boost::asio::buffer_size(b);
    }

    return receiveBuf.unpack();
}

START_TEST(shmsrv_request_pack_unpack)
{
    const ShmReq withoutCmd { "EMPTY", { } };
    auto receivedRequest { packUnpackReq(withoutCmd) };

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(withoutCmd != *receivedRequest, "The unpacked request is not equal to the original one.");


    const ShmReq oneCmdWithoutZone { "ONE_CMD_WITHOUT_ZONE", { std::make_shared< ShmCmdReq >(-1, ShmCmdReq::Cmd::GetList, std::shared_ptr< Zone > { }) } };
    receivedRequest = packUnpackReq(oneCmdWithoutZone);

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(oneCmdWithoutZone != *receivedRequest, "The unpacked request is not equal to the original one.");


    const ShmReq oneCmdWithZone { "ONE_CMD_WITH_ZONE",
                                 { std::make_shared< ShmCmdReq >(1, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 1, "title", { 1, 2, 3, } })) }
    };

    receivedRequest = packUnpackReq(oneCmdWithZone);

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(oneCmdWithZone != *receivedRequest, "The unpacked request is not equal to the original one.");


    const ShmReq twoCmdsWithZone { "TWO_CMDS_WITH_ZONE", {
                                    std::make_shared< ShmCmdReq >(1, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 1, "first", { 1, 2, 3, } })),
                                    std::make_shared< ShmCmdReq >(2, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 2, "second", { 4, 5 } }))
                                 }
    };

    receivedRequest = packUnpackReq(twoCmdsWithZone);

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(twoCmdsWithZone != *receivedRequest, "The unpacked request is not equal to the original one.");


    const ShmReq threeCmdsWithZone { "THREE_CMDS_WITH_ZONE", {
                                     std::make_shared< ShmCmdReq >(1, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 1, "first", { 1, 2, 3, } } )),
                                     std::make_shared< ShmCmdReq >(2, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 2, "second", { 4, 5 } })),
                                     std::make_shared< ShmCmdReq >(3, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 3, "third", { 6 } }))
                                   }
    };

    receivedRequest = packUnpackReq(threeCmdsWithZone);

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(threeCmdsWithZone != *receivedRequest, "The unpacked request is not equal to the original one.");


    const ShmReq fourCmdsWithZone { "FOUR_CMDS_WITH_ZONE", {
                                     std::make_shared< ShmCmdReq >(1, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 1, "first", { 1, 2, 3, } })),
                                     std::make_shared< ShmCmdReq >(2, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 2, "second", { 4, 5 } })),
                                     std::make_shared< ShmCmdReq >(3, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 3, "third", { 6 } })),
                                     std::make_shared< ShmCmdReq >(4, ShmCmdReq::Cmd::UpdZone, std::make_shared< Zone >(Zone { 4, "fourth", { } }))
                                  }
    };

    receivedRequest = packUnpackReq(fourCmdsWithZone);

    fail_if(nullptr == receivedRequest, "unpack failed.");
    fail_if(fourCmdsWithZone != *receivedRequest, "The unpacked request is not equal to the original one.");
}
END_TEST

static std::shared_ptr< ShmAnswer > packUnpackAnswer(const ShmAnswer& answer)
{
    LogTrace(TRACE5) << __FUNCTION__ << " : " << answer;

    ShmAnswerBuf sendBuf { };

    std::vector< uint8_t > raw { };
    for (const auto& b : sendBuf.all(answer)) {
        raw.insert(std::end(raw), boost::asio::buffer_cast< const uint8_t* >(b),
                                  boost::asio::buffer_cast< const uint8_t* >(b) + boost::asio::buffer_size(b));
    }

    ShmAnswerBuf receiveBuf { };

    std::size_t offset { 0 };

    for (const auto& b : receiveBuf.head()) {
        auto begin { std::cbegin(raw) + offset };
        auto end { begin + boost::asio::buffer_size(b) };

        std::copy(begin, end, boost::asio::buffer_cast< uint8_t* >(b));

        offset += boost::asio::buffer_size(b);
    }

    for (const auto& b : receiveBuf.tail()) {
        auto begin { std::cbegin(raw) + offset };
        auto end { begin + boost::asio::buffer_size(b) };

        std::copy(begin, end, boost::asio::buffer_cast< uint8_t* >(b));

        offset += boost::asio::buffer_size(b);
    }

    return receiveBuf.unpack();
}

START_TEST(shmsrv_answer_pack_unpack)
{
    const ShmAnswer withoutCmd { "EMPTY", { } };
    auto receivedAnswer { packUnpackAnswer(withoutCmd) };

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(withoutCmd != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer oneCmdWithoutZone { "ONE_CMD_WITHOUT_ZONE", {
                                            std::make_shared< ShmCmdAnswer >(-1, ShmCmdAnswer::Result::InternalServerError,
                                                                             ShmCmdReq::Cmd::GetList,
                                                                             std::vector< std::shared_ptr< Zone > > { })
                                        }
    };

    receivedAnswer = packUnpackAnswer(oneCmdWithoutZone);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(oneCmdWithoutZone != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer oneCmdWithZones { "ONE_CMD_WITH_ZONES", {
                                       std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                        ShmCmdReq::Cmd::GetZone,
                                                                        std::vector< std::shared_ptr< Zone > > {
                                                                            std::make_shared< Zone >(Zone { 1, "title", { 1, 2, 3, } })
                                                                        }
                                       ) }
    };

    receivedAnswer = packUnpackAnswer(oneCmdWithZones);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(oneCmdWithZones != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer oneCmdWithTwoZones { "ONE_CMD_WITH_TWO_ZONES", {
                                       std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                        ShmCmdReq::Cmd::GetList,
                                                                        std::vector< std::shared_ptr< Zone > > {
                                                                            std::make_shared< Zone >(Zone { 1, "first", { 1, 2, 3, } }),
                                                                            std::make_shared< Zone >(Zone { 2, "second", { 4, 5 } })
                                                                        }
                                       ) }
    };

    receivedAnswer = packUnpackAnswer(oneCmdWithTwoZones);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(oneCmdWithTwoZones != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer oneCmdWithThreeZones { "ONE_CMD_WITH_THREE_ZONES", {
                                           std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                            ShmCmdReq::Cmd::GetList,
                                                                            std::vector< std::shared_ptr< Zone > > {
                                                                                std::make_shared< Zone >(Zone { 1, "first", { 1, 2, 3, } }),
                                                                                std::make_shared< Zone >(Zone { 2, "second", { 4, 5 } }),
                                                                                std::make_shared< Zone >(Zone { 3, "third", { 5 } })
                                                                            }
                                           ) }
    };

    receivedAnswer = packUnpackAnswer(oneCmdWithThreeZones);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(oneCmdWithThreeZones != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer twoCmdsWithZone { "TWO_CMDS_WITH_ZONE", {
                                      std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                       ShmCmdReq::Cmd::GetZone,
                                                                       std::vector< std::shared_ptr< Zone > > {
                                                                           std::make_shared< Zone >(Zone { 1, "first", { 1 } }),
                                                                       }),
                                      std::make_shared< ShmCmdAnswer >(2, ShmCmdAnswer::Result::Success,
                                                                       ShmCmdReq::Cmd::GetZone,
                                                                       std::vector< std::shared_ptr< Zone > > {
                                                                           std::make_shared< Zone >(Zone { 2, "second", { } })
                                                                       })
                                      }
    };

    receivedAnswer = packUnpackAnswer(twoCmdsWithZone);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(twoCmdsWithZone != *receivedAnswer, "The unpacked answer is not equal to the original one.");


    const ShmAnswer threeCmdsWithZone { "THREE_CMDS_WITH_ZONE", {
                                         std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                          ShmCmdReq::Cmd::GetZone,
                                                                          std::vector< std::shared_ptr< Zone > > {
                                                                              std::make_shared< Zone >(Zone { 1, "first", { } }),
                                                                          }),
                                         std::make_shared< ShmCmdAnswer >(2, ShmCmdAnswer::Result::Success,
                                                                          ShmCmdReq::Cmd::GetList,
                                                                          std::vector< std::shared_ptr< Zone > > {
                                                                              std::make_shared< Zone >(Zone { 2, "second", { 1 } }),
                                                                              std::make_shared< Zone >(Zone { 3, "third", { 2, 3 } }),
                                                                          }),
                                         std::make_shared< ShmCmdAnswer >(4, ShmCmdAnswer::Result::Success,
                                                                          ShmCmdReq::Cmd::GetZone,
                                                                          std::vector< std::shared_ptr< Zone > > {
                                                                              std::make_shared< Zone >(Zone { 4, "fourth", { 4 } })
                                                                          })
                                         }
    };

    receivedAnswer = packUnpackAnswer(threeCmdsWithZone);

    fail_if(nullptr == receivedAnswer, "unpack failed.");
    fail_if(threeCmdsWithZone != *receivedAnswer, "The unpacked request is not equal to the original one.");
}
END_TEST

START_TEST(shmsrv_process_cmd)
{
    ShmReq getListReq { "PROCESS_CMD", { std::make_shared< ShmCmdReq >(-1, ShmCmdReq::Cmd::GetList, std::shared_ptr< Zone > { }) } };

    const ShmAnswer getListExpectedAnswer { getListReq.owner,
                                            { std::make_shared< ShmCmdAnswer >(-1, ShmCmdAnswer::Result::Success,
                                                                               ShmCmdReq::Cmd::GetList,
                                                                               std::vector< std::shared_ptr< Zone > > { }) }
    };

    ShmAnswer getListAnswer { getListReq.owner, { } };

    process(getListReq, getListAnswer);

    LogTrace(TRACE5) << "getListAnswer: " << getListAnswer;

    fail_if(getListReq.owner != getListAnswer.owner, "owners are different.");
    fail_if(1 != getListAnswer.answers.size(), "answers not empty.");
    fail_if(getListExpectedAnswer != getListAnswer);

    const std::shared_ptr< Zone > zone { std::make_shared< Zone >(Zone { 1, "title", { 1, 2, 3 } }) };

    ShmReq addZoneCmd { getListReq.owner, { std::make_shared< ShmCmdReq >(1, ShmCmdReq::Cmd::AddZone, zone ) } };

    ShmAnswer addZoneAnswer { addZoneCmd.owner, { } };
    const ShmAnswer addZoneExpectedAnswer { addZoneCmd.owner,
                                            { std::make_shared< ShmCmdAnswer >(1, ShmCmdAnswer::Result::Success,
                                                                               ShmCmdReq::Cmd::AddZone,
                                                                               std::vector< std::shared_ptr< Zone > > { }) }
    };

    process(addZoneCmd, addZoneAnswer);

    LogTrace(TRACE5) << "addZoneAnswer: " << addZoneAnswer;

    fail_if(getListReq.owner != addZoneAnswer.owner, "owners are different.");
    fail_if(1 != addZoneAnswer.answers.size(), "answers.size is not equal 1.");
    fail_if(nullptr == addZoneAnswer.answers.front());
    fail_if(addZoneExpectedAnswer != addZoneAnswer);


    const ShmAnswer getListAfterAddExpectedAnswer { getListReq.owner,
                                                    { std::make_shared< ShmCmdAnswer >(-1, ShmCmdAnswer::Result::Success,
                                                                                       ShmCmdReq::Cmd::GetList,
                                                                                       std::vector< std::shared_ptr< Zone > > { zone }) }
    };

    ShmAnswer getListAfterAddAnswer { getListReq.owner, { } };

    process(getListReq, getListAfterAddAnswer);

    LogTrace(TRACE5) << "getListAfterAddAnswer: " << getListAfterAddAnswer;

    fail_if(getListReq.owner != getListAfterAddAnswer.owner, "owners are different.");
    fail_if(1 != getListAfterAddAnswer.answers.size(), "answers.size is not equal 1.");
    fail_if(nullptr == getListAfterAddAnswer.answers.front());
    fail_if(getListAfterAddExpectedAnswer != getListAfterAddAnswer);
}
END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(nullptr, nullptr)
    ADD_TEST(shmsrv_request_pack_unpack)
    ADD_TEST(shmsrv_answer_pack_unpack)
    ADD_TEST(shmsrv_process_cmd)
TCASEFINISH
#undef SUITENAME

#endif // XP_TESTING
