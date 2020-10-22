create or replace procedure SP_WB_REF_AC_PR_FLT_RECORDS
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
                                                     qq.date_from_ "date_from",
                                                       qq.PORT_NAME "PORT_NAME",
                                                         qq.sign "sign",
                                                           qq.UTC_DIFF "UTC_DIFF",
                                                             qq.utc_diff_str "UTC_DIFF_STR",
                                                               qq.IS_CHECK_IN "IS_CHECK_IN",
                                                                 qq.IS_LOAD_CONTROL "IS_LOAD_CONTROL",
                                                                   qq.U_NAME "U_NAME",
	                                                                   qq.U_IP "U_IP",
	                                                                     qq.U_HOST_NAME "U_HOST_NAME",
                                                                         qq.DATE_WRITE "DATE_WRITE"))).getClobVal() into cXML_data
    from (select to_char(q.id) id,
                 q.date_from_,
                 q.PORT_NAME,
                 case when q.UTC_DIFF>=0 then '+' else '-' end sign,
                 to_char(q.UTC_DIFF) UTC_DIFF,
                 WB_TIME_INT_TO_STR(q.UTC_DIFF, ',') utc_diff_str,
                 to_char(q.IS_CHECK_IN) IS_CHECK_IN,
	               to_char(q.IS_LOAD_CONTROL) IS_LOAD_CONTROL,
                 q.U_NAME,
	               q.U_IP,
	               q.U_HOST_NAME,
                 q.DATE_WRITE
          from (select f.id,
                       f.date_from,
                       to_char(f.date_from, 'dd.mm.yyyy') date_from_,
                       case when P_LANG='RUS' then a.NAME_RUS_SMALL else a.NAME_ENG_SMALL end PORT_NAME,
                       f.UTC_DIFF,
         	             f.IS_CHECK_IN,
	                     f.IS_LOAD_CONTROL,
	                     f.U_NAME,
	                     f.U_IP,
	                     f.U_HOST_NAME,
	                     to_char(f.DATE_WRITE, 'dd.mm.yyyy hh:mm:ss') DATE_WRITE
                from WB_REF_AIRCO_PORT_FLIGHT f join WB_REF_AIRPORTS a
                on f.id_ac=P_AIRCO_ID and
                   f.id_port=a.id) q
                order by q.PORT_NAME,
          q.date_from) qq;

    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_AC_PR_FLT_RECORDS;
/
