create or replace PROCEDURE SP_WB_AIRCO_GET_C_D_DTM_INFO(cXML_in IN clob,
                                                           cXML_out OUT CLOB)
AS
P_ID_AC number:=-1;
P_DATE_FROM date;
P_DATE_FROM_D varchar2(50):='';
P_DATE_FROM_M varchar2(50):='';
P_DATE_FROM_Y varchar2(50):='';
cXML_data clob:='';
r_count number:=0;
  BEGIN
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    select count(id) into r_count
    from WB_REF_AIRCO_C_DATA_DTM
    where ID_AC=P_ID_AC and
          DATE_FROM=P_DATE_FROM;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (r_count)>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.dtm_id "dtm_id",
                                                         e.date_from "date_from",
                                                           e.u_name "U_NAME",
                                                             e.u_ip "U_IP",
                                                               e.u_host_name "U_HOST_NAME",
                                                                 e.date_write "DATE_WRITE"), xmlelement("remark", (select q.remark
                                                                                                                   from (select nvl(dtm.remark, 'NULL') remark
                                                                                                                         from WB_REF_AIRCO_C_DATA_DTM dtm
                                                                                                                         where dtm.ID_AC=P_ID_AC and
                                                                                                                               dtm.DATE_FROM=P_DATE_FROM) q
                                                                                                                   where rownum=1)))).getClobVal() into cXML_data
        from (select distinct to_char(dtm_id) dtm_id,
                              to_char(date_from, 'dd.mm.yyyy') date_from,
                              U_NAME,
                              U_IP,
                              U_HOST_NAME,
                              to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
              from WB_REF_AIRCO_C_DATA_DTM
              where ID_AC=P_ID_AC and
                    DATE_FROM=P_DATE_FROM
              order by dtm_id) e;

        if cXML_data  is not null
          then cXML_out:=cXML_out||cXML_data;
        end if;
      end;
    end if;


    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;
  END SP_WB_AIRCO_GET_C_D_DTM_INFO;
/
