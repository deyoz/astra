#pragma once

#include "astra_consts.h"

#include <list>
#include <string>


namespace WBMessages {

class TMsgType {
    public:
        enum Enum {
            mtLOADSHEET,
            mtNOTOC,
            mtLIR,
            None
        };

        static const std::list< std::pair<Enum, std::string> >& pairs();
};

class TMsgTypes : public ASTRA::PairList<TMsgType::Enum, std::string>
{
    private:
        virtual std::string className() const { return "TMsgTypes"; }
    public:
        TMsgTypes() : ASTRA::PairList<TMsgType::Enum, std::string>(TMsgType::pairs(),
                boost::none,
                boost::none) {}
};

const TMsgTypes& MsgTypes();

void toDB(int point_id, TMsgType::Enum msg_type,
          const std::string &content, const std::string& source = "");

}//namespace WBMessages
