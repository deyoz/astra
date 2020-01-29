#include "msglog.h"

#include "cursctl.h"
#include "oci_selector_char.h"

#define  NICKNAME "ASP"
#include "slogger.h"

namespace msglog {

    using std::string;
    using std::vector;

    namespace {

        string make_autonomous(const string& str) {
            return "DECLARE PRAGMA autonomous_transaction;\n"
                "BEGIN\n" +
                str + ";\n"
                "COMMIT;\n"
                "END;";
        }

        Id gen_msglog_id() {
            char ida[256] = {0};

            make_curs("BEGIN\n"
                "  SELECT TO_CHAR(SEQ_MSGLOG.nextval) INTO :ida FROM DUAL;\n"
                "END;")
                .bindOut(":ida", ida)
                .exec();

            return Id(ida);
        }

        string gen_req(bool autonomous) {
            static const string req =
                "INSERT INTO msglog (id, text, time, chunk_num, domain)"
                "  VALUES (:id, :text, sysdate, :chunk_num, :domain)";

            if (autonomous)
                return make_autonomous(req);
            return req;
        }

        vector<string> gen_chunks(const string& text) {
            static const size_t MAX_CHUNK = 4000;
            vector<string> chunks;

            for (size_t offset = 0; offset < text.size(); offset += MAX_CHUNK)
                chunks.push_back(string(text.data() + offset,
                                        std::min(MAX_CHUNK, text.size() - offset)));
            return chunks;
        }

    } /* anonymous namespace */

    Id write(const Domain& domain, const string& text, bool is_autonomous) {
        const Id id = gen_msglog_id();

        size_t num = 0;
        for (const string& chunk : gen_chunks(text))
            make_curs(gen_req(is_autonomous))
                .stb()
                .bind(":id", id.str())
                .bind(":text", chunk)
                .bind(":chunk_num", num++)
                .bind(":domain", domain.str())
                .exec();

        return id;
    }

    string read(const Id& id) {
        string text;
        string chunk;

        auto curs = make_curs("SELECT text FROM msglog WHERE id = :id ORDER BY chunk_num");

        curs.stb()
            .def(chunk)
            .bind(":id", id.str())
            .exec();

        while (not curs.fen())
            text += chunk;

        return text;
    }

    bool remove(const Id& id) {
        auto curs = make_curs("DELETE FROM msglog WHERE id = :id");
        curs.stb()
            .bind(":id", id.str())
            .exec();
        return curs.rowcount();
    }

} /* namespace msglog */
