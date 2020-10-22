create or replace procedure SP_WB_REF_WS_AIR_REG_W_DATA
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
    from WB_REF_WS_AIR_REG_WGT_A
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRCO_WS_BORTS
        where ID_AC=P_ID_AC and
              ID_WS=P_ID_WS;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("bort_list_data", xmlattributes(e.id as "id",
                                                                       e.BORT_NUM as "BORT_NUM"))).getClobVal() into cXML_data
            from (select to_char(id) as id,
                           BORT_NUM
                  from WB_REF_AIRCO_WS_BORTS
                  where ID_AC=P_ID_AC and
                        ID_WS=P_ID_WS) e
            order by e.BORT_NUM;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_WCC
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("wcc_list_data", xmlattributes(e.id as "id",
                                                                      e.CODE_NAME as "CODE_NAME"))).getClobVal() into cXML_data
            from (select to_char(id) id,
                           CODE_NAME
                  from WB_REF_WS_AIR_WCC
                  where ID_AC=P_ID_AC and
                        ID_WS=P_ID_WS and
                        id_bort=P_ID_BORT) e
            order by e.CODE_NAME;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_SEAT_LAY_ADV
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("s_plan_list_data", xmlattributes(e.id as "id",
                                                                         e.TABLE_NAME as "TABLE_NAME"))).getClobVal() into cXML_data
            from (select to_char(id) id,
                           TABLE_NAME
                  from WB_REF_WS_AIR_SEAT_LAY_ADV
                  where ID_AC=P_ID_AC and
                        ID_WS=P_ID_WS and
                        id_bort=P_ID_BORT) e
            order by e.TABLE_NAME;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_REG_WGT
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("table_data", xmlattributes(e.id as "id",
                                                                   e.BORT_NUM as "BORT_NUM",
                                                                     e.slw_table_name as "SLW_TABLE_NAME",
                                                                       e.wcc_code_name as "WCC_CODE_NAME",
                                                                         e.is_acars as "IS_ACARS",
                                                                           e.IS_DEFAULT as "IS_DEFAULT",
                                                                             e.IS_APPROVED as "IS_APPROVED",
                                                                               e.REMARK as "REAMARK",
                                                                                 e.U_NAME as "U_NAME",
                                                                                   e.U_IP as "U_IP",
                                                                                     e.U_HOST_NAME as "U_HOST_NAME",
                                                                                       e.DATE_WRITE as "DATE_WRITE",
                                                                                         e.DATE_FROM_ as "DATE_FROM",
                                                                                           e.ARM as "ARM",
                                                                                             e.DOW as "DOW",
                                                                                               e.DOI as "DOI",
                                                                                                 e.PROC_MAC as "PROC_MAC"))).getClobVal() into cXML_data
            from (select to_char(rw.id) id,
                           wb.BORT_NUM,
                             slw.TABLE_NAME slw_table_name,
                               wcc.CODE_NAME wcc_code_name,
                                 to_char(rw.IS_ACARS) IS_ACARS,
                                   to_char(rw.IS_DEFAULT) IS_DEFAULT,
                                     to_char(rw.IS_APPROVED) IS_APPROVED,
                                       rw.DATE_FROM,
                                         rw.remark,
                                           rw.U_NAME,
                                             rw.U_IP,
                                               rw.U_HOST_NAME,
                                                 to_char(rw.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE,
                                                   to_char(rw.DATE_FROM, 'dd.mm.yyyy') DATE_FROM_,
                                                     case when rw.DOW is null then 'NULL' else to_char(rw.DOW) end DOW,
	                                                     case when rw.DOI is null then 'NULL' else to_char(rw.DOI) end DOI,
	                                                       case when rw.ARM is null then 'NULL' else to_char(rw.ARM) end ARM,
	                                                         case when rw.PROC_MAC is null then 'NULL' else to_char(rw.PROC_MAC) end PROC_MAC
                  from WB_REF_WS_AIR_REG_WGT rw join WB_REF_AIRCO_WS_BORTS wb
                  on rw.id_ac=P_ID_AC and
                     rw.id_ws=P_ID_WS and
                     rw.ID_BORT=wb.id join WB_REF_WS_AIR_SEAT_LAY_ADV slw
                     on slw.id=rw.S_L_ADV_ID join WB_REF_WS_AIR_WCC wcc
                        on wcc.id=rw.WCC_ID
                  order by wb.BORT_NUM,
                           rw.DATE_FROM) e
            order by e.BORT_NUM,
                     e.date_from;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;


      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_REG_W_DATA;
/
