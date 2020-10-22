create or replace procedure SP_WB_REF_AC_LOAD_CODES_REC
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


    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(qq.id "id",
                                                     qq.date_from "date_from",
                                                       qq.date_to "date_to",
                                                         qq.a_code_name "a_code_name",
                                                           qq.i_code_name "i_code_name",
                                                             qq.priority "priority",
                                                               qq.density "density",
                                                                 qq.description "description",
                                                                   qq.U_NAME "U_NAME",
	                                                                   qq.U_IP "U_IP",
	                                                                     qq.U_HOST_NAME "U_HOST_NAME",
                                                                         qq.DATE_WRITE "DATE_WRITE"))).getClobVal() into cXML_data
    from (select to_char(q.id) id,
                 q.a_code_name,
                 q.i_code_name,
                 q.date_from_output date_from,
                 q.date_to_output date_to,
                 to_char(q.priority) priority,
                 to_char(q.density) density,
                 q.description,
                 q.U_NAME,
	               q.U_IP,
	               q.U_HOST_NAME,
                 q.DATE_WRITE
          from (select clc.id,
                       clc.CODE_NAME a_code_name,
                       ilc.CODE_NAME i_code_name,
                       nvl(to_char(clc.date_from, 'dd.mm.yyyy'), 'NULL') date_from_output,
                       clc.date_from,
                       nvl(to_char(clc.date_to, 'dd.mm.yyyy'), 'NULL') date_to_output,
                       clc.date_to,
                       clc.DENSITY,
                       clc.priority,
                       clc.description,
	                     clc.U_NAME,
	                     clc.U_IP,
	                     clc.U_HOST_NAME,
	                     to_char(clc.DATE_WRITE, 'dd.mm.yyyy hh:mm:ss') DATE_WRITE
                from WB_REF_AIRCO_LOAD_CODES clc join WB_REF_IATA_LOAD_CODES ilc
    on clc.id_ac=P_AIRCO_ID and
       ilc.id=clc.IATA_CODE_ID) q
    order by q.a_code_name,
                        q.date_from,
                         q.date_to) qq;


    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_AC_LOAD_CODES_REC;
/
