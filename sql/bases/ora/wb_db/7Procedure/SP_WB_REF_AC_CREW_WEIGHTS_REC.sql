create or replace procedure SP_WB_REF_AC_CREW_WEIGHTS_REC
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(100);
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    cXML_out:='<?xml version="1.0" ?><root>';


    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(qq.id "ID",
                                                     qq.date_from "DATE_FROM",
                                                       qq.description "DESCRIPTION",
                                                         qq.FC_STANDART "FC_STANDART",
	                                                         qq.FC_MALE "FC_MALE",
	                                                           qq.FC_FEMALE "FC_FEMALE",
	                                                             qq.FC_HAND_BAG "FC_HAND_BAG",
	                                                               qq.FC_HAND_BAG_INCLUDE "FC_HAND_BAG_INCLUDE",
	                                                                 qq.CC_STANDART "CC_STANDART",
	                                                                   qq.CC_MALE "CC_MALE",
	                                                                     qq.CC_FEMALE "CC_FEMALE",
	                                                                       qq.CC_HAND_BAG "CC_HAND_BAG",
	                                                                         qq.CC_HAND_BAG_INCLUDE "CC_HAND_BAG_INCLUDE",
	                                                                           qq.FC_BAGGAGE_WEIGHT "FC_BAGGAGE_WEIGHT",
	                                                                             qq.CC_BAGGAGE_WEIGHT "CC_BAGGAGE_WEIGHT",
                                                                                 qq.BY_DEFAULT "BY_DEFAULT",
                                                                                   qq.U_NAME "U_NAME",
	                                                                                   qq.U_IP "U_IP",
	                                                                                     qq.U_HOST_NAME "U_HOST_NAME",
                                                                                         qq.DATE_WRITE "DATE_WRITE"))).getClobVal() into cXML_data
    from (select to_char(q.id) id,
                 q.description,
                 q.date_from_output as date_from,
                 nvl(to_char(q.FC_STANDART), 'NULL') as FC_STANDART,
	               nvl(to_char(q.FC_MALE), 'NULL') as FC_MALE,
	               nvl(to_char(q.FC_FEMALE), 'NULL') as FC_FEMALE,
	               nvl(to_char(q.FC_HAND_BAG), 'NULL') as FC_HAND_BAG,
	               to_char(q.FC_HAND_BAG_INCLUDE) as FC_HAND_BAG_INCLUDE,
	               nvl(to_char(q.CC_STANDART), 'NULL') as CC_STANDART,
	               nvl(to_char(q.CC_MALE), 'NULL') as CC_MALE,
	               nvl(to_char(q.CC_FEMALE), 'NULL') as CC_FEMALE,
	               nvl(to_char(q.CC_HAND_BAG), 'NULL') as CC_HAND_BAG,
	               to_char(q.CC_HAND_BAG_INCLUDE) as CC_HAND_BAG_INCLUDE,
	               nvl(to_char(q.FC_BAGGAGE_WEIGHT), 'NULL') as FC_BAGGAGE_WEIGHT,
	               nvl(to_char(q.CC_BAGGAGE_WEIGHT), 'NULL') as CC_BAGGAGE_WEIGHT,
                 q.BY_DEFAULT,
                 q.U_NAME,
	               q.U_IP,
	               q.U_HOST_NAME,
                 q.DATE_WRITE
          from (select clc.id,
                         clc.description,
                           nvl(to_char(clc.date_from, 'dd.mm.yyyy'), 'NULL') date_from_output,
                             clc.date_from,
                               clc.FC_STANDART,
	                               clc.FC_MALE,
	                                 clc.FC_FEMALE,
	                                   clc.FC_HAND_BAG,
	                                     clc.FC_HAND_BAG_INCLUDE,
	                                       clc.CC_STANDART,
	                                         clc.CC_MALE,
	                                           clc.CC_FEMALE,
	                                             clc.CC_HAND_BAG,
	                                               clc.CC_HAND_BAG_INCLUDE,
	                                                 clc.FC_BAGGAGE_WEIGHT,
	                                                   clc.CC_BAGGAGE_WEIGHT,
	                                                     clc.BY_DEFAULT,
	                                                       clc.U_NAME,
	                                                         clc.U_IP,
	                                                           clc.U_HOST_NAME,
	                                                             to_char(clc.DATE_WRITE, 'dd.mm.yyyy hh:mm:ss') DATE_WRITE
                from WB_REF_AIRCO_CREW_WEIGHTS clc
                where clc.id_ac=P_AIRCO_ID) q
    order by q.date_from) qq;


    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_AC_CREW_WEIGHTS_REC;
/
