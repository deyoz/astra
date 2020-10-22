create or replace procedure SP_WB_REF_AC_WS_BORTS_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  if P_ID_AC=-1 then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_AIRCO_WS_BORTS
      where ID_WS=P_ID_WS;

      SELECT XMLAGG(XMLELEMENT("table_data", xmlattributes(e.id as "id",
                                                             e.BORT_NUM as "BORT_NUM",
                                                               e.U_NAME as "U_NAME",
                                                                 e.U_IP as "U_IP",
                                                                   e.U_HOST_NAME as "U_HOST_NAME",
                                                                     e.date_write as "date_write"))).getClobVal() into cXML_data
      from (select to_char(id) as id,
                     BORT_NUM,
                       U_NAME,
                         U_IP,
                           U_HOST_NAME,
                             to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
            from WB_REF_AIRCO_WS_BORTS
            where ID_WS=P_ID_WS) e
            order by e.BORT_NUM;

      cXML_out:=cXML_out||cXML_data;
    end;
  else
    begin
      select count(id) into V_R_COUNT
      from WB_REF_AIRCO_WS_BORTS
      where ID_AC=P_ID_AC and
            ID_WS=P_ID_WS;

      SELECT XMLAGG(XMLELEMENT("table_data", xmlattributes(e.id as "id",
                                                             e.BORT_NUM as "BORT_NUM",
                                                               e.U_NAME as "U_NAME",
                                                                 e.U_IP as "U_IP",
                                                                   e.U_HOST_NAME as "U_HOST_NAME",
                                                                     e.date_write as "date_write"))).getClobVal() into cXML_data
      from (select to_char(id) as id,
                     BORT_NUM,
                       U_NAME,
                         U_IP,
                           U_HOST_NAME,
                             to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
            from WB_REF_AIRCO_WS_BORTS
            where ID_AC=P_ID_AC and
                  ID_WS=P_ID_WS) e
            order by e.BORT_NUM;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';

end SP_WB_REF_AC_WS_BORTS_DATA;
/
