#ifndef _FRANCHISE_H_
#define _FRANCHISE_H_

#include "astra_consts.h"

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
                spm,
                psm,
                tpm
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
                    l.push_back(std::make_pair(spm,         "pr_spm"));
                    l.push_back(std::make_pair(psm,         "pr_psm"));
                    l.push_back(std::make_pair(tpm,         "pr_tpm"));

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

    enum TPropVal {
        Oper,
        Franchisee,
        Both,
        Unknown
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
        bool get(int point_id, TPropType::Enum prop);
    };

}

#endif
