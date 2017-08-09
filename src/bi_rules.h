#ifndef _BI_RULES_H
#define _BI_RULES_H

#include <list>
#include <utility>
#include <string>
#include "astra_consts.h"
#include "oralib.h"
#include "astra_misc.h"
#include "dev_consts.h"

namespace BIPrintRules {

class TPrintType
{
  public:
    enum Enum
    {
      One,
      OnePlusOne,
      All,
      None
    };

    static const std::list< std::pair<Enum, std::string> >& pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(One,        "ONE"));
        l.push_back(std::make_pair(OnePlusOne, "TWO"));
        l.push_back(std::make_pair(All,        "ALL"));
      }
      return l;
    }

    static const std::list< std::pair<Enum, std::string> >& view_pairs()
    {
      static std::list< std::pair<Enum, std::string> > l;
      if (l.empty())
      {
        l.push_back(std::make_pair(One,        "One"));
        l.push_back(std::make_pair(OnePlusOne, "OnePlusOne"));
        l.push_back(std::make_pair(All,        "All"));
        l.push_back(std::make_pair(None,       "None"));
      }
      return l;
    }
};

class TPrintTypes : public ASTRA::PairList<TPrintType::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TPrintTypes"; }
  public:
    TPrintTypes() : ASTRA::PairList<TPrintType::Enum, std::string>(TPrintType::pairs(),
                                                                   boost::none,
                                                                   boost::none) {}
};

class TPrintTypesView : public ASTRA::PairList<TPrintType::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TPrintTypesView"; }
  public:
    TPrintTypesView() : ASTRA::PairList<TPrintType::Enum, std::string>(TPrintType::view_pairs(),
                                                                       boost::none,
                                                                       boost::none) {}
};

const TPrintTypes& PrintTypes();
const TPrintTypesView& PrintTypesView();

    struct TRule {
        bool pr_get; // Признак того, что для тек. пакса был вызван Holder::get
        int id; // bi_print_rules.id
        typedef std::map<int, int> THallsList;
        THallsList halls; // список id залов; halls[hall] = terminal
        std::pair<int, int> curr_hall; // зал, выбранный на клиенте <hall, terminal>
        bool pr_print_bi;     // Печатать отдельное БП или нет
        TPrintType::Enum print_type;

        bool exists() const { return print_type != TPrintType::None; }
        void dump(const std::string &file, int line) const;
        void fromDB(TQuery &Qry);
        bool tags_enabled(ASTRA::TDevOper::Enum op_type, bool first_seg) const;
        TRule():
            pr_get(false),
            id(ASTRA::NoExists),
            curr_hall(std::pair<int, int>(ASTRA::NoExists, ASTRA::NoExists)),
            pr_print_bi(false),
            print_type(TPrintType::None)
        {}
    };

    void get_rule(
            const std::string &airline,
            const std::string &tier_level,
            const std::string &cls,
            const std::string &subcls,
            const std::string &rem_code,
            TRule &rule
            );

    bool bi_airline_service(
            const TTripInfo &info,
            TRule &rule
            );

    class Holder {
        private:
            typedef std::map<int, TRule> TPaxList;
            void getByGrpId(int grp_id);
            int get_hall_id(ASTRA::TDevOper::Enum op_type, int pax_id);
            std::set<int> grps;
            TPaxList items;
            ASTRA::TDevOper::Enum op_type;
        public:
            const TRule &get(int grp_id, int pax_id);
            void dump(const std::string &file, int line) const;
            bool complete() const;
            bool select(xmlNodePtr reqNode);
            void toXML(ASTRA::TDevOper::Enum op_type, xmlNodePtr resNode);
            Holder(ASTRA::TDevOper::Enum aop_type): op_type(aop_type) {}
    };

} //namespace BIPrintRules

// Данный класс вычисляет признак печати для операций dotPrnBP, dotPrnBI
// и возвращает по требованию
class TPrPrint {
    private:
        BIPrintRules::Holder bi_rules;

        typedef std::map<int, bool> TPaxPrint; // pax_id, pr_print
        typedef std::map<int, TPaxPrint> TPaxMap; // seg no, pax

        std::set<int> grps; // processed grp_id-s

        TPaxMap paxs;

        void fromDB(int grp_id, int pax_id, TQuery &Qry);
        bool get_pr_print(int pax_id);
    public:
        void get_pr_print(int grp_id, int pax_id, bool &pr_bp_print, bool &pr_bi_print);
        void dump(const std::string &file, int line);
        TPrPrint(): bi_rules(ASTRA::TDevOper::PrnBI) {}
};

#endif
