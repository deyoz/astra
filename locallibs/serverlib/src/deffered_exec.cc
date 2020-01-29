#include "deffered_exec.h"
#include "cursctl.h"
#include "str_utils.h"

#define  NICKNAME "IDFUMG"
#include "slogger.h"

namespace deffered_exec {

RequestId RequestId::create()
{
    std::size_t req_id {0};
    make_curs("BEGIN\nSELECT seq_deffered_exec_reqid.nextval INTO :req_id FROM DUAL;\nEND;").
        bindOut(":req_id", req_id).
        exec();
    return RequestId(StrUtils::ToBase36Lpad(req_id, max_length));
}

bool RequestId::exists() const
{
    OciCpp::CursCtl curs = make_curs("SELECT 1 FROM deffered_exec WHERE id = :id");
    curs.
        bind(":id", id).
        EXfet();
    return curs.rowcount() > 0;
}

void RequestId::save() const
{
    make_curs("INSERT INTO deffered_exec (id, timestamp) VALUES (:id, sysdate)").
        stb().
        bind(":id", id).
        exec();
}

void RequestId::remove() const
{
    make_curs("DELETE FROM deffered_exec WHERE id = :id").
        stb().
        bind(":id", id).
        exec();
}

bool RequestId::operator<(const RequestId& param) const noexcept
{
    return id < param.id;
}

PostHookRequest::PostHookRequest(const Callable& callable)
    : callable(callable),
      id(RequestId::create())
{
    id.save();
}

PostHookRequest* PostHookRequest::clone() const
{
    return new PostHookRequest(*this);
}

void PostHookRequest::run()
{
    if (id.exists()) {
        LogTrace(TRACE1) << __FUNCTION__ << ": call " << id.id;
        callable();
        id.remove();
    }
}

bool PostHookRequest::less2(const Posthooks::BaseHook* param) const noexcept
{
    if (const auto hook = dynamic_cast<const PostHookRequest*>(param)) {
        return id < hook->id;
    }

    return false;
}

bool PostHookRequest::operator<(const Posthooks::BaseHook* param) const noexcept
{
    return less2(param);
}

} /* namespace deffered_exec */
