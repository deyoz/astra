create or replace procedure SP_WB_REF_AC_PAX_WEIGHTS_REC
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(100);
R_COUNT number;
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ---------------------------CLASS CODE---------------------------------------
    cXML_data:='';

    select XMLAGG(XMLELEMENT("class_code", xmlattributes(to_char(qq.id) "id",
                                                           qq.class_code "class_code"))).getClobVal() into cXML_data
    from (select q.id,
                   q.class_code
          from (select -1 as id,
                       case when P_LANG='ENG' then '<no class>' else '<без класса>' end class_code
                from dual
                union all
                select distinct id,
                                  class_code
                from WB_REF_AIRCO_CLASS_CODES
                where id_ac=P_AIRCO_ID) q
          order by q.class_code) qq;

    cXML_out:=cXML_out||cXML_data;
    ---------------------------CLASS CODE---------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_AIRCO_PAX_WEIGHTS
    where id_ac=P_AIRCO_ID;

    if R_COUNT>0 then
      begin
        cXML_data:='';

        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(qq.id "ID",
                                                         qq.class "CLASS",
                                                           qq.date_from "DATE_FROM",
                                                             qq.description "DESCRIPTION",
                                                               qq.adult "ADULT",
	                                                               qq.MALE "MALE",
	                                                                 qq.FEMALE "FEMALE",
                                                                     qq.CHILD "CHILD",
	                                                                     qq.INFANT "INFANT",
	                                                                       qq.HAND_BAG "HAND_BAG",
	                                                                         qq.HAND_BAG_INCLUDE "HAND_BAG_INCLUDE",	
                                                                             qq.BY_DEFAULT "BY_DEFAULT",
                                                                               qq.U_NAME "U_NAME",
	                                                                               qq.U_IP "U_IP",
	                                                                                 qq.U_HOST_NAME "U_HOST_NAME",
                                                                                     qq.DATE_WRITE "DATE_WRITE"))).getClobVal() into cXML_data
        from (select to_char(q.id) id,
                     q.class,
                     q.description,
                     q.date_from_output as date_from,
                     nvl(to_char(q.adult), 'NULL') as ADULT,
	                   nvl(to_char(q.MALE), 'NULL') as MALE,
	                   nvl(to_char(q.FEMALE), 'NULL') as FEMALE,
                     nvl(to_char(q.CHILD), 'NULL') as CHILD,
	                   nvl(to_char(q.INFANT), 'NULL') as INFANT,
	                   nvl(to_char(q.HAND_BAG), 'NULL') as HAND_BAG,
	                   to_char(q.HAND_BAG_INCLUDE) as HAND_BAG_INCLUDE,	
                     q.BY_DEFAULT,
                     q.U_NAME,
	                   q.U_IP,
	                   q.U_HOST_NAME,
                     q.DATE_WRITE
              from (select distinct clc.id,
                                     nvl(acl.CLASS_CODE, case when P_LANG='ENG' then '<no class>' else '<без класса>' end) class,
                                        clc.description,
                                          nvl(to_char(clc.date_from, 'dd.mm.yyyy'), 'NULL') date_from_output,
                                            clc.date_from,
                                              clc.adult,
	                                              clc.MALE,
	                                                clc.FEMALE,
                                                    clc.CHILD,
	                                                    clc.INFANT,
                                                        clc.hand_bag,
	                                                        clc.HAND_BAG_INCLUDE,	
	                                                          clc.BY_DEFAULT,
	                                                            clc.U_NAME,
	                                                              clc.U_IP,
	                                                                clc.U_HOST_NAME,
	                                                                  to_char(clc.DATE_WRITE, 'dd.mm.yyyy hh:mm:ss') DATE_WRITE
                    from WB_REF_AIRCO_PAX_WEIGHTS clc left outer join WB_REF_AIRCO_CLASS_CODES acl
                    on clc.id_ac=P_AIRCO_ID and
                       clc.id_class=acl.id
                    where clc.id_ac=P_AIRCO_ID) q
        order by q.class,
                 q.date_from) qq;

       cXML_out:=cXML_out||cXML_data;
      end;
    end if;


    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_AC_PAX_WEIGHTS_REC;
/
