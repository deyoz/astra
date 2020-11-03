#include <vector>
#include <string>
#include "ASM_template.h"
#include <typeb/tb_elements.h>
#include <typeb/typeb_template.h>
#include <typeb/AnyStringTemplate.h>
#include <typeb/ReferenceTemplate.h>
#include <typeb/AsmTypes.h>

namespace typeb_parser {

AsmTemplate::AsmTemplate()
    :typeb_template("ASM", "Ad-hoc schedule message")
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
    // NEW|RPL
    {
    templ_group::ptr_t newSubmsg(new templ_group);
    newSubmsg->addElement(new AnyStringTemplate({ "NEW", "RPL" }))->setNecessary();
    newSubmsg->addElement(new AsmFlightTemplate)->setNecessary();
    
    templ_group::ptr_t tmp(new templ_group);
    tmp->addElement(new AsmEquipmentTemplate);
    tmp->addElement(new AsmRoutingTemplate)->setUnlimRep();

    newSubmsg->addElement(tmp)->setNecessary()->setUnlimRep();
    newSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    newSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    newSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(newSubmsg);
    }
    // CNL|RIN
    {
    templ_group::ptr_t cnlSubmsg(new templ_group);
    cnlSubmsg->addElement(new AnyStringTemplate({ "CNL", "RIN" }))->setNecessary();
    //old parser : only 1 flight
    cnlSubmsg->addElement(new AsmFlightTemplate)->setNecessary();//short flight
    cnlSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    cnlSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(cnlSubmsg);
    }
    // ACK
    {
    templ_group::ptr_t ackSubmsg(new templ_group);
    ackSubmsg->addElement(new AnyStringTemplate({ "ACK" }))->setNecessary();
    ackSubmsg->addElement(new AsmDontCareTemplate)->setUnlimRep();
    sw->addElement(ackSubmsg);
    }
    // NAC
    {
    templ_group::ptr_t nacSubmsg(new templ_group);
    nacSubmsg->addElement(new AnyStringTemplate({ "NAC" }))->setNecessary();
    nacSubmsg->addElement(new SsmEmptyTemplate);
    nacSubmsg->addElement(new AsmRejectInfoTemplate)->setNecessary()->setUnlimRep();
    nacSubmsg->addElement(new SsmEmptyTemplate);

    templ_group::ptr_t two(new templ_group);
    templ_switch_group::ptr_t tmp(new templ_switch_group);
    tmp->addElement(new AsmFlightTemplate);
    tmp->addElement(new AsmDontCareTemplate);
    two->addElement(tmp);

    nacSubmsg->addElement(two)->setUnlimRep()->setNecessary();
    sw->addElement(nacSubmsg);
    }
    // ADM
    {
    templ_group::ptr_t admSubmsg(new templ_group);
    admSubmsg->addElement(new AnyStringTemplate({ "ADM" }))->setNecessary();
    admSubmsg->addElement(new AsmFlightTemplate)->setNecessary(); //flight + deis
    admSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    admSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    admSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(admSubmsg);
    }
    // CON|EQT
    {
    templ_group::ptr_t eqtconSubmsg(new templ_group);
    eqtconSubmsg->addElement(new AnyStringTemplate({ "CON", "EQT" }))->setNecessary();
    eqtconSubmsg->addElement(new AsmFlightTemplate)->setNecessary(); //it depends
    eqtconSubmsg->addElement(new AsmEquipmentTemplate)->setNecessary();
    eqtconSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    eqtconSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    eqtconSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(eqtconSubmsg);
    }
    // FLT
    {
    templ_group::ptr_t fltSubmsg(new templ_group);
    fltSubmsg->addElement(new AnyStringTemplate({ "FLT" }))->setNecessary();
    fltSubmsg->addElement(new AsmFlightTemplate)->setNecessary(); //flight + new flight
    fltSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    fltSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    fltSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(fltSubmsg);
    }
    // TIM
    {
    templ_group::ptr_t timSubmsg(new templ_group);
    timSubmsg->addElement(new AnyStringTemplate({ "TIM" }))->setNecessary();
    timSubmsg->addElement(new AsmFlightTemplate)->setNecessary(); //flight without anything
    timSubmsg->addElement(new AsmRoutingTemplate)->setNecessary()->setUnlimRep();
    timSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    timSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    timSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(timSubmsg); //switch of submsgs
    }
    // RRT
    {
    templ_group::ptr_t rrtSubmsg(new templ_group);
    rrtSubmsg->addElement(new AnyStringTemplate({ "RRT" }))->setNecessary();
    rrtSubmsg->addElement(new AsmFlightTemplate)->setNecessary();
    rrtSubmsg->addElement(new AsmEquipmentTemplate);
    //TODO в SSIM написано, что можно несколько плечей и, если экип на них разный,
    //то нужно указывать строку с экипом. т.е. по хорошему шаблон выглядеть должен так
    // [equip + routing list]{1,} Проблема в том, что экип - необязательный элемент,
    // и потому завязывать на него группу нельзя (первый элемент в группе - обязательный для 
    // typeb-парсера). Т.е. при организации как выше не будут проходить тлг без экипа.
    // Поэтому написано так, как сейчас - только 1 экип на плечо/плечи.
    rrtSubmsg->addElement(new AsmRoutingTemplate)->setNecessary()->setUnlimRep();
    rrtSubmsg->addElement(new AsmSegmentTemplate)->setUnlimRep();
    rrtSubmsg->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    rrtSubmsg->addElement(new AnyStringTemplate({ "//" }));
    sw->addElement(rrtSubmsg); //switch of submsgs
    }

    //-- end of switch --
    submsgs->addElement(sw); //any submsg in msg
    addElement(submsgs)->setUnlimRep();
    
    templ_group::ptr_t supGroup(new templ_group);
    supGroup->setAccessName("Suppl");
    supGroup->addElement(new SsmSupplInfoTemplate)->setUnlimRep();
    addElement(supGroup); //suppl info in msg
}

}// typeb_parser

