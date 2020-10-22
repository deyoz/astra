create or replace procedure SP_WB_REF_WS_AIR_ULD_OVER_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

cXML_data clob;
V_R_COUNT number:=0;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEC_BAY_T
  where ID_AC=P_ID_AC and
        ID_WS=P_ID_WS and
        ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.SEC_BAY_NAME as "SEC_BAY_NAME",
                                    q.OVER_STRING as "OVER_STRING",
                                      q.U_NAME as "U_NAME",
                                        q.U_IP as "U_IP",
                                          q.U_HOST_NAME as "U_HOST_NAME",
                                            q.date_write as "date_write"))).getClobVal() into cXML_data
        from (select distinct h.SEC_BAY_NAME,
                                WB_WS_AIR_ULD_OVER_STR(P_ID_AC,
                                                         P_ID_WS,
                                                           P_ID_BORT,
                                                             h.SEC_BAY_NAME) OVER_STRING,

                                   nvl((select q.u_NAME
                                        from (select o.u_name
                                              from WB_REF_WS_AIR_ULD_OVER o
                                              where o.ID_AC=P_ID_AC and
                                                    o.ID_WS=P_ID_WS and
                                                    o.ID_BORT=P_ID_BORT and
                                                    o.POSITION=h.SEC_BAY_NAME) q
                                        WHERE ROWNUM<2), 'NULL') U_NAME,

                                     nvl((select q.U_IP
                                          from (select o.U_IP
                                                from WB_REF_WS_AIR_ULD_OVER o
                                                where o.ID_AC=P_ID_AC and
                                                      o.ID_WS=P_ID_WS and
                                                      o.ID_BORT=P_ID_BORT and
                                                      o.POSITION=h.SEC_BAY_NAME) q
                                          WHERE ROWNUM<2), 'NULL') U_IP,

                                       nvl((select q.U_HOST_NAME
                                            from (select o.U_HOST_NAME
                                                  from WB_REF_WS_AIR_ULD_OVER o
                                                  where o.ID_AC=P_ID_AC and
                                                        o.ID_WS=P_ID_WS and
                                                        o.ID_BORT=P_ID_BORT and
                                                        o.POSITION=h.SEC_BAY_NAME) q
                                            WHERE ROWNUM<2), 'NULL') U_HOST_NAME,

                                       nvl((select to_char(q.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')
                                            from (select o.date_write
                                                  from WB_REF_WS_AIR_ULD_OVER o
                                                  where o.ID_AC=P_ID_AC and
                                                        o.ID_WS=P_ID_WS and
                                                        o.ID_BORT=P_ID_BORT and
                                                        o.POSITION=h.SEC_BAY_NAME) q
                                            WHERE ROWNUM<2), 'NULL') date_write
              from WB_REF_WS_AIR_SEC_BAY_T h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.SEC_BAY_NAME) q
      order by q.SEC_BAY_NAME;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_ULD_OVER_DATA;
/
