create or replace procedure SP_WB_REF_WS_AIR_WCC_CODS_DATA
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
    from WB_REF_WS_AIR_DOW_CR_CODES
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("crew_codes_list",
                    xmlattributes(q.id as "id",
                                    q.CR_CODE_NAME as "crew_code_name"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       h.CR_CODE_NAME
              from WB_REF_WS_AIR_DOW_CR_CODES h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.CR_CODE_NAME) q
        order by q.CR_CODE_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_PT_CODES
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("pantry_codes_list",
                    xmlattributes(q.id as "id",
                                    q.PT_CODE_NAME as "pantry_code_name"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       h.PT_CODE_NAME
              from WB_REF_WS_AIR_DOW_PT_CODES h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.PT_CODE_NAME) q
        order by q.PT_CODE_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_PW_CODES
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("portable_water_codes_list",
                    xmlattributes(q.id as "id",
                                    q.PW_CODE_NAME as "portable_water_code_name"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       h.PW_CODE_NAME
              from WB_REF_WS_AIR_DOW_PW_CODES h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.PW_CODE_NAME) q
        order by q.PW_CODE_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_SWA_CODES
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("adjustment_codes_list",
                    xmlattributes(q.id as "id",
                                    q.CODE_NAME_1 as "adjustment_code_name"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       h.CODE_NAME_1
              from WB_REF_WS_AIR_DOW_SWA_CODES h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.CODE_NAME_1) q
        order by q.CODE_NAME_1;

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
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.ID as "id",
                                    q.CODE_NAME as "CODE_NAME",
                                      q.CR_CODE_NAME as "CREW_CODE_NAME",
                                        q.PT_CODE_NAME as "PANTRY_CODE_NAME",
                                          q.PW_CODE_NAME as "PORTABLE_WATER_CODE_NAME",
                                            q.ac_str as "AC_STR",
                                              q.U_NAME as "U_NAME",
                                                q.U_IP as "U_IP",
                                                  q.U_HOST_NAME as "U_HOST_NAME",
                                                    q.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       h.CODE_NAME,
                         crew_codes.CR_CODE_NAME,
                           pantry_codes.PT_CODE_NAME,
                             portable_water_codes.PW_CODE_NAME,
                               WB_WS_AIR_WCC_AC_STR(h.id) AC_STR,
                                 h.U_NAME,
                                   h.U_IP,
                                     h.U_HOST_NAME,
                                       to_char(h.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_REF_WS_AIR_WCC h join WB_REF_WS_AIR_DOW_CR_CODES crew_codes
              on h.id_ac=P_ID_AC and
                 h.id_ws=P_ID_WS and
                 h.id_bort=P_ID_BORT  and
                 h.CREW_CODE_ID=crew_codes.id join WB_REF_WS_AIR_DOW_PT_CODES pantry_codes
                 on h.PANTRY_CODE_ID=pantry_codes.id join WB_REF_WS_AIR_DOW_PW_CODES portable_water_codes
                    on h.PORTABLE_WATER_CODE_ID=portable_water_codes.id
              order by h.CODE_NAME) q
      order by q.CODE_NAME;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_WCC_CODS_DATA;
/
