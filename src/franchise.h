#ifndef _FRANCHISE_H_
#define _FRANCHISE_H_

#include "astra_consts.h"
#include "astra_misc.h"

namespace Franchise {

    class TPropType {
        public:
            enum Enum {
                paxManifest,
                bagManifest,
                bp,
                bt,
                aodb,
                apis,
                mintrans,
                lci,
                ldm,
                mvtDelay,
                mvtDep,
                mvtArv,
                com,
                som,
                bsm,
                prl,
                btm,
                ptm,
                cpm,
                psm,
                tpm,
                wb,
                Unknown
            };

            static const std::list< std::pair<Enum, std::string> >& pairs()
            {
                static std::list< std::pair<Enum, std::string> > l;
                if (l.empty())
                {
                    l.push_back(std::make_pair(paxManifest, "pr_pax_manifest"));
                    l.push_back(std::make_pair(bagManifest, "pr_bag_manifest"));
                    l.push_back(std::make_pair(bp,          "pr_bp"));
                    l.push_back(std::make_pair(bt,          "pr_bt"));
                    l.push_back(std::make_pair(aodb,        "pr_aodb"));
                    l.push_back(std::make_pair(apis,        "pr_apis"));
                    l.push_back(std::make_pair(mintrans,    "pr_mintrans"));
                    l.push_back(std::make_pair(lci,         "pr_lci"));
                    l.push_back(std::make_pair(ldm,         "pr_ldm"));
                    l.push_back(std::make_pair(mvtDelay,    "pr_mvt_delay"));
                    l.push_back(std::make_pair(mvtDep,      "pr_mvt_dep"));
                    l.push_back(std::make_pair(mvtArv,      "pr_mvt_arv"));
                    l.push_back(std::make_pair(com,         "pr_com"));
                    l.push_back(std::make_pair(som,         "pr_som"));
                    l.push_back(std::make_pair(bsm,         "pr_bsm"));
                    l.push_back(std::make_pair(prl,         "pr_prl"));
                    l.push_back(std::make_pair(btm,         "pr_btm"));
                    l.push_back(std::make_pair(ptm,         "pr_ptm"));
                    l.push_back(std::make_pair(cpm,         "pr_cpm"));
                    l.push_back(std::make_pair(psm,         "pr_psm"));
                    l.push_back(std::make_pair(tpm,         "pr_tpm"));
                    l.push_back(std::make_pair(wb,          "pr_wb"));

                }
                return l;
            }

            static const std::list< std::pair<Enum, std::string> >& pairs_tlg()
            {
                static std::list< std::pair<Enum, std::string> > l;
                if (l.empty())
                {
                    l.push_back(std::make_pair(lci,         "LCI"));
                    l.push_back(std::make_pair(ldm,         "LDM"));
                    l.push_back(std::make_pair(mvtDelay,    "MVTC"));
                    l.push_back(std::make_pair(mvtDep,      "MVTA"));
                    l.push_back(std::make_pair(mvtArv,      "MVTB"));
                    l.push_back(std::make_pair(com,         "COM"));
                    l.push_back(std::make_pair(som,         "SOM"));
                    l.push_back(std::make_pair(bsm,         "BSM"));
                    l.push_back(std::make_pair(prl,         "PRL"));
                    l.push_back(std::make_pair(btm,         "BTM"));
                    l.push_back(std::make_pair(ptm,         "PTM"));
                    l.push_back(std::make_pair(ptm,         "PTMN"));
                    l.push_back(std::make_pair(cpm,         "CPM"));
                    l.push_back(std::make_pair(psm,         "PSM"));
                    l.push_back(std::make_pair(tpm,         "TPM"));

                }
                return l;
            }

    };

    class TPropTypes: public ASTRA::PairList<TPropType::Enum, std::string>
    {
        private:
            virtual std::string className() const { return "TPropTypes"; }
        public:
            TPropTypes() : ASTRA::PairList<TPropType::Enum, std::string>(TPropType::pairs(),
                    boost::none,
                    boost::none) {}
    };

    class TPropTypesTlg: public ASTRA::PairList<TPropType::Enum, std::string>
    {
        private:
            virtual std::string className() const { return "TPropTypesTlg"; }
        public:
            TPropTypesTlg() : ASTRA::PairList<TPropType::Enum, std::string>(TPropType::pairs_tlg(),
                    TPropType::Unknown,
                    boost::none) {}
    };

    const TPropTypes &PropTypes();
    const TPropTypesTlg &PropTypesTlg();

    enum TPropVal {
        pvYes,
        pvNo,
        pvEmpty,
        pvUnknown
    };

    struct TFlight {
        std::string airline;
        int flt_no;
        std::string suffix;
        TFlight() { clear(); }
        void clear();
    };


    struct TProp {
        TFlight oper;
        TFlight franchisee;
        TPropVal val;

        TProp() { clear(); }
        void clear();
        
        // поиск ведется по ст-цам
        // airline, flt_no, suffix
        bool get(const TTripInfo &info, TPropType::Enum prop, bool is_local_scd_out);
        bool get(int point_id, TPropType::Enum prop);
        bool get(int point_id, const std::string &tlg_type);

        // поиск ведется по ст-цам
        // airline_franchisee, flt_no_franchisee, suffix_franchisee
        bool get_franchisee( const TTripInfo &info, TPropType::Enum prop );
    };

}

#endif
