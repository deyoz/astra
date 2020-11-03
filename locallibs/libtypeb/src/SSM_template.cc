#include <vector>
#include <string>
#include "SSM_template.h"
#include "typeb/tb_elements.h"
#include "typeb/typeb_template.h"
#include "typeb/AnyStringTemplate.h"
#include "typeb/ReferenceTemplate.h"
#include "typeb/SsmTemplate.h"

namespace typeb_parser {

SsmTemplate::SsmTemplate()
    : typeb_template("SSM", "Standard schedule message")
{
    templ_group::ptr_t timeGroup(new templ_group);
    timeGroup->setAccessName("Time");
    timeGroup->addElement(new AnyStringTemplate({ "LT", "UTC" }));
    addElement(timeGroup); //time in msg

    templ_group::ptr_t refGroup(new templ_group);
    refGroup->setAccessName("Ref");
    refGroup->addElement(new ReferenceTemplate);
    addElement(refGroup); //ref in msg

    templ_group::ptr_t submsgs(new templ_group);
    submsgs->setAccessName("Submsgs");

    templ_switch_group::ptr_t sw(new templ_switch_group);
    // -- start of switch --
    {
        //NEW, RPL
        templ_group::ptr_t newSubmsg(new templ_group);
        newSubmsg->setAccessName("New");
        newSubmsg->addElement(new AnyStringTemplate({ "NEW", "RPL" }))->setNecessary();
        newSubmsg->addElement(new SsmFlightTemplate)->setNecessary();
        //organization is very complicated -- period can have equipment after it. any number of periods is allowed
        //but not necessary all of them is followed by equipment. in such case equipment is the same for all periods
        //mentioned. the same trouble exists with legs too - number is undefined and indefinite number of pairs of
        //equipment + leg for period is allowed. so all of these complications are dealt with in Info structures
        //constructor, not here
        templ_group::ptr_t three(new templ_group);//period or equipment or leg

        templ_switch_group::ptr_t tmp(new templ_switch_group);
        tmp->addElement(new SsmLongPeriodTemplate);
        tmp->addElement(new SsmEquipmentTemplate);
        tmp->addElement(new SsmRoutingTemplate);

        three->addElement(tmp)->setNecessary();

        newSubmsg->addElement(three)->setNecessary()->setUnlimRep();
        newSubmsg->addElement(new SsmSegmentTemplate)->setUnlimRep();
        newSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        newSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(newSubmsg);
    }
    {
        //CNL
        templ_group::ptr_t cnlSubmsg(new templ_group);
        cnlSubmsg->setAccessName("Cnl");
        cnlSubmsg->addElement(new AnyStringTemplate({ "CNL" }))->setNecessary();
        //iata ssim : flights and periods.
        //old parser : only 1 flight
        cnlSubmsg->addElement(new SsmShortFlightTemplate)->setNecessary();
        cnlSubmsg->addElement(new SsmLongPeriodTemplate)->setNecessary()->setUnlimRep();
        cnlSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        cnlSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(cnlSubmsg);
    }
    {
        //SKD
        templ_group::ptr_t skdSubmsg(new templ_group);
        skdSubmsg->setAccessName("Skd");
        skdSubmsg->addElement(new AnyStringTemplate({ "SKD", "RSD" }))->setNecessary();
        skdSubmsg->addElement(new SsmShortFlightTemplate)->setNecessary();
        skdSubmsg->addElement(new SsmSkdPeriodTemplate)->setNecessary();
        skdSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        skdSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(skdSubmsg);
    }
    {
        //ACK
        templ_group::ptr_t ackSubmsg(new templ_group);
        ackSubmsg->setAccessName("Ack");
        ackSubmsg->addElement(new AnyStringTemplate({ "ACK" }))->setNecessary();
        ackSubmsg->addElement(new SsmDontCareTemplate)->setUnlimRep();
        sw->addElement(ackSubmsg);
    }
    {
        //NAC
        templ_group::ptr_t nacSubmsg(new templ_group);
        nacSubmsg->setAccessName("Nac");
        nacSubmsg->addElement(new AnyStringTemplate({ "NAC" }))->setNecessary();

        nacSubmsg->addElement(new SsmEmptyTemplate);
        nacSubmsg->addElement(new SsmRejectInfoTemplate)->setNecessary()->setUnlimRep();
        nacSubmsg->addElement(new SsmEmptyTemplate);

        templ_group::ptr_t three(new templ_group);
        templ_switch_group::ptr_t tmp(new templ_switch_group);
        tmp->addElement(new SsmFlightTemplate);
        tmp->addElement(new SsmLongPeriodTemplate);
        tmp->addElement(new SsmDontCareTemplate);
        three->addElement(tmp);

        nacSubmsg->addElement(three)->setUnlimRep()->setNecessary();
        sw->addElement(nacSubmsg);
    }
    {
        //ADM
        templ_group::ptr_t admSubmsg(new templ_group);
        admSubmsg->setAccessName("Adm");
        admSubmsg->addElement(new AnyStringTemplate({ "ADM" }))->setNecessary();
        admSubmsg->addElement(new SsmFlightTemplate)->setNecessary();
        admSubmsg->addElement(new SsmLongPeriodTemplate)->setNecessary()->setUnlimRep();
        //admSubmsg->addElement(new SsmLegChangeTemplate);
        //legchange is not supported anyway
        //maybe its possible to have several legchanges linked to diff periods but
        //no such tlgs were caught and legchange is unsupported anyway.
        //to support multiple legchanges remake adm to look like tim
        admSubmsg->addElement(new SsmSegmentTemplate)->setUnlimRep();
        admSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        admSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(admSubmsg);
    }
    {
        //CON, EQT
        templ_group::ptr_t eqtconSubmsg(new templ_group);
        eqtconSubmsg->setAccessName("Con");
        eqtconSubmsg->addElement(new AnyStringTemplate({ "CON", "EQT" }))->setNecessary();
        eqtconSubmsg->addElement(new SsmFlightTemplate)->setNecessary();

        templ_group::ptr_t info(new templ_group); //period | equipment | lci

        templ_switch_group::ptr_t t(new templ_switch_group);
        t->addElement(new SsmLongPeriodTemplate);
        t->addElement(new SsmEquipmentTemplate);
        t->addElement(new SsmLegChangeTemplate);

        info->addElement(t)->setNecessary();

        eqtconSubmsg->addElement(info)->setNecessary()->setUnlimRep();
        eqtconSubmsg->addElement(new SsmSegmentTemplate)->setUnlimRep();
        eqtconSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        eqtconSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(eqtconSubmsg);
    }
    {
        //FLT
        templ_group::ptr_t fltSubmsg(new templ_group);
        fltSubmsg->setAccessName("Flt");
        fltSubmsg->addElement(new AnyStringTemplate({ "FLT" }))->setNecessary();
        fltSubmsg->addElement(new SsmShortFlightTemplate)->setNecessary();
        fltSubmsg->addElement(new SsmLongPeriodTemplate)->setNecessary()->setUnlimRep();
        fltSubmsg->addElement(new SsmShortFlightTemplate)->setNecessary();
        fltSubmsg->addElement(new SsmSegmentTemplate)->setUnlimRep();
        fltSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        fltSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(fltSubmsg);
    }
    {
        //REV
        templ_group::ptr_t revSubmsg(new templ_group);
        revSubmsg->setAccessName("Rev");
        revSubmsg->addElement(new AnyStringTemplate({ "REV" }))->setNecessary();
        revSubmsg->addElement(new SsmRevFlightTemplate)->setNecessary();
        revSubmsg->addElement(new SsmLongPeriodTemplate)->setNecessary()->setUnlimRep();
        revSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        revSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(revSubmsg);
    }
    {
        //TIM
        templ_group::ptr_t timSubmsg(new templ_group);
        timSubmsg->setAccessName("Tim");
        timSubmsg->addElement(new AnyStringTemplate({ "TIM" }))->setNecessary();
        timSubmsg->addElement(new SsmShortFlightTemplate)->setNecessary(); //flight
        templ_group::ptr_t periodGroup(new templ_group); //group of period(s) + leg(s)
        periodGroup->addElement(new SsmLongPeriodTemplate)->setUnlimRep();
        periodGroup->addElement(new SsmRoutingTemplate)->setNecessary()->setUnlimRep();
        timSubmsg->addElement(periodGroup)->setNecessary()->setUnlimRep(); //many such groups
        timSubmsg->addElement(new SsmSegmentTemplate)->setUnlimRep();
        timSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
        timSubmsg->addElement(new AnyStringTemplate({ "//" }));
        sw->addElement(timSubmsg); //switch of submsgs
    }
    //-- end of switch --
    submsgs->addElement(sw); //any submsg in msg
    addElement(submsgs)->setUnlimRep(); //unlimited number of any submsgs in msg

    templ_group::ptr_t supGroup(new templ_group);
    supGroup->setAccessName("Suppl");
    supGroup->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    addElement(supGroup); //suppl info in msg
}

}// typeb_parser

