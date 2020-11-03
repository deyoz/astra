#include "simple_task.h"

#include "cursctl.h"
#include "dates_oci.h"
#include "str_utils.h"
#include "testmode.h"
#include "posthooks.h"

#define NICKNAME "ASP"
#include "slogger.h"

namespace ServerFramework {

    void fallback_failed(const TaskId& id) {
        LogError(STDLOG) << ": Warning! Wrong fallback called! id = " << id;
    }

    std::string make_autonomous(const std::string& str) {
        return "DECLARE PRAGMA autonomous_transaction;\n"
            "BEGIN\n" +
            str + ";\n"
            "COMMIT;\n"
            "END;";
    }

    void create_task(const TaskId& id,
                     const TaskPrefix& prefix,
                     const TaskAttempt& attempt,
                     const TaskAttempt& max_attempt) {
        LogTrace(TRACE5) << __FUNCTION__
						 << ": id = '" << id << "'"
						 << ", prefix = '" << prefix << "'";

        std::string q =
            "INSERT INTO simpletask (id, prefix, attempt, max_attempt, init_time)"
            "  VALUES (:id, :prefix, :attempt, :max_attempt, :init_time)";

        if (not inTestMode())
            q = make_autonomous(q);

        make_curs(q)
            .stb()
            .bind(":id", id.str())
            .bind(":prefix", prefix.str())
            .bind(":attempt", attempt.get())
            .bind(":max_attempt", max_attempt.get())
            .bind(":init_time", boost::posix_time::second_clock::local_time())
            .exec();
    }

    void increase_attempt(const TaskId& id, const TaskPrefix& prefix) {
        std::string q =
            "UPDATE simpletask SET attempt = attempt + 1"
            "  WHERE id = :id AND prefix = :prefix";

        if (not inTestMode())
            q = make_autonomous(q);

        make_curs(q)
            .stb()
            .bind(":id", id.str())
            .bind(":prefix", prefix.str())
            .exec();
    }

    Task read_task(const TaskId& id,
                   const TaskPrefix& prefix,
                   const TaskAttempt& max_attempt) {
        LogTrace(TRACE5) << __FUNCTION__
						 << ": id = '" << id << "'"
						 << ", prefix = '" << prefix << "'";

        int attempt = 0;
        boost::posix_time::ptime init_time;

        OciCpp::CursCtl curs = make_curs(
            "SELECT attempt, init_time"
            "  FROM simpletask"
            "    WHERE id = :id AND prefix = :prefix");
        curs
            .stb()
            .checkRowCount(1)
            .def(attempt)
            .def(init_time)
            .bind(":id", id.str())
            .bind(":prefix", prefix.str())
            .EXfet();

        if (curs.rowcount() == 0) {
            create_task(id, prefix, TaskAttempt(0), max_attempt);
            init_time = boost::posix_time::second_clock::local_time();
        }

        const Task task = {
            id,
            prefix,
            TaskAttempt(attempt),
            max_attempt,
            init_time
        };

        return task;
    }

    void remove_task(const TaskId& id, const TaskPrefix& prefix) {
        LogTrace(TRACE5) << __FUNCTION__
						 << ": id = '" << id << "'"
						 << ", prefix = '" << prefix << "'";

        std::string q =
            "DELETE FROM simpletask WHERE id = :id AND prefix = :prefix";

        if (not inTestMode())
            q = make_autonomous(q);

        make_curs(q)
            .stb()
            .bind(":id", id.str())
            .bind(":prefix", prefix.str())
            .exec();
    }

} /* ServerFramework */
