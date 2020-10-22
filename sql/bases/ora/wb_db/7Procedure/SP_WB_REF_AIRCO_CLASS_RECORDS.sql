create or replace procedure SP_WB_REF_AIRCO_CLASS_RECORDS
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
                                                         qq.class_code "class_code",
                                                           qq.priority_code "priority_code",
                                                             qq.description "description",
                                                               qq.U_NAME "U_NAME",
	                                                               qq.U_IP "U_IP",
	                                                                 qq.U_HOST_NAME "U_HOST_NAME",
                                                                     qq.DATE_WRITE "DATE_WRITE"))).getClobVal() into cXML_data
    from (select to_char(q.id) id,
                 q.CLASS_CODE,
                 q.date_from_output date_from,
                 q.date_to_output date_to,
                 to_char(q.priority_code) priority_code,
                 q.description,
                 q.U_NAME,
	               q.U_IP,
	               q.U_HOST_NAME,
                 q.DATE_WRITE
          from (select id,
                       CLASS_CODE,
                       nvl(to_char(date_from, 'dd.mm.yyyy'), 'NULL') date_from_output,
                       date_from,
                       nvl(to_char(date_to, 'dd.mm.yyyy'), 'NULL') date_to_output,
                       date_to,
                       priority_code,
                       description,
	                     U_NAME,
	                     U_IP,
	                     U_HOST_NAME,
	                     to_char(DATE_WRITE, 'dd.mm.yyyy hh:mm:ss') DATE_WRITE
                from WB_REF_AIRCO_CLASS_CODES
                where id_ac=P_AIRCO_ID) q
                order by q.CLASS_CODE,
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
  end SP_WB_REF_AIRCO_CLASS_RECORDS;
/
