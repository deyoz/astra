create or replace procedure SP_WB_REF_WS_AIR_MIN_W_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_IDN number:=-1;

cXML_data clob;
V_R_COUNT number:=0;
begin
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IDN[1]')) into P_IDN from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_MIN_WGHT_IDN
    where ID=P_IDN;

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
        from WB_REF_WS_AIR_MIN_WGHT_T
        where IDN=P_IDN;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("table_data", xmlattributes(e.id as "id",
                                                                   e.BORT_NUM as "BORT_NUM",
                                                                     e.MIN_RMP_WEIGHT as "MIN_RMP_WEIGHT",
	                                                                     e.MIN_ZF_WEIGHT as "MIN_ZF_WEIGHT",
	                                                                       e.MIN_TO_WEIGHT as "MIN_TO_WEIGHT",
	                                                                         e.MIN_LND_WEIGHT as "MIN_LND_WEIGHT",
                                                                             e.IS_DEFAULT as "IS_DEFAULT",
                                                                               e.REMARK as "REAMARK",
                                                                                 e.U_NAME as "U_NAME",
                                                                                   e.U_IP as "U_IP",
                                                                                     e.U_HOST_NAME as "U_HOST_NAME",
                                                                                       e.DATE_WRITE as "DATE_WRITE"))).getClobVal() into cXML_data
            from (select to_char(rw.id) id,
                           wb.BORT_NUM,
                             to_char(MIN_RMP_WEIGHT) MIN_RMP_WEIGHT,
	                             to_char(MIN_ZF_WEIGHT) MIN_ZF_WEIGHT,
	                               to_char(MIN_TO_WEIGHT) MIN_TO_WEIGHT,
	                                 to_char(MIN_LND_WEIGHT) MIN_LND_WEIGHT,
                                     to_char(rw.IS_DEFAULT) IS_DEFAULT,
                                       rw.remark,
                                         rw.U_NAME,
                                           rw.U_IP,
                                             rw.U_HOST_NAME,
                                               to_char(rw.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
                  from WB_REF_WS_AIR_MIN_WGHT_T rw join WB_REF_AIRCO_WS_BORTS wb
                  on rw.IDN=P_IDN and
                     rw.ID_BORT=wb.id
                  order by wb.BORT_NUM) e
            order by e.BORT_NUM;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;
      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_MIN_W_DATA;
/
