create or replace procedure SP_WB_REF_WS_AIR_DOW_SW_C_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_WS_TYPES
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS;

    if V_R_COUNT>0 then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOW_SWA_CDER
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        if V_R_COUNT=0 then
          begin
            insert into WB_REF_WS_AIR_DOW_SWA_CDER (ID,
	                                                    ID_AC,
	                                                      ID_WS,
                                                    	    ID_BORT,
	                                                          IS_BALANCE_ARM,
                                                          	  IS_INDEX_UNIT,
	                                                              U_NAME,
	                                                                U_IP,
	                                                                  U_HOST_NAME,
	                                                                    DATE_WRITE)
            select SEC_WB_REF_WS_AIR_DOW_SWA_CDER.nextval,
                     P_ID_AC,
                       P_ID_WS,
                         P_ID_BORT,
                           1,
                             0,
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
                                     sysdate()
            from dual;
          end;
        end if;

        SELECT XMLAGG(XMLELEMENT("swa_cder_list", xmlattributes(to_char(IS_BALANCE_ARM) as "IS_BALANCE_ARM",
                                                                  to_char(IS_INDEX_UNIT) as "IS_INDEX_UNIT",
                                                                    U_NAME as "U_NAME",
                                                                      U_IP as "U_IP",
                                                                        U_HOST_NAME as "U_HOST_NAME",
                                                                          to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as "date_write"))).getClobVal() into cXML_data
        from WB_REF_WS_AIR_DOW_SWA_CDER
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        cXML_out:=cXML_out||cXML_data;

        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOW_SWA_CODES
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("swa_code_list", xmlattributes(e.id as "id",
                                                                      e.CODE_NAME_1 "CODE_NAME_1",
                                                                        e.CODE_NAME_2 "CODE_NAME_2",
                                                                          e.WEIGHT as "WEIGHT",
                                                                            e.BALANCE_ARM as "BALANCE_ARM",
                                                                              e.INDEX_UNIT as "INDEX_UNIT",
                                                                                e.REMARKS as "REMARKS",
                                                                                  e.U_NAME as "U_NAME",
                                                                                    e.U_IP as "U_IP",
                                                                                      e.U_HOST_NAME as "U_HOST_NAME",
                                                                                        e.date_write as "date_write"))).getClobVal() into cXML_data
            from (select to_char(id) as id,
                           case when CODE_NAME_1 is null then 'NULL' else CODE_NAME_1 end as CODE_NAME_1,
                             case when CODE_NAME_2 is null then 'NULL' else CODE_NAME_2 end as CODE_NAME_2,
                               case when WEIGHT is null then 'NULL' else to_char(WEIGHT) end as WEIGHT,
	                               case when BALANCE_ARM is null then 'NULL' else to_char(BALANCE_ARM) end as BALANCE_ARM,
                                   case when INDEX_UNIT is null then 'NULL' else to_char(INDEX_UNIT) end as INDEX_UNIT,	
                                     case when REMARKS is null then 'NULL' else REMARKS end as REMARKS,
                                       U_NAME,
                                         U_IP,
                                           U_HOST_NAME,
                                             to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
                  from WB_REF_WS_AIR_DOW_SWA_CODES
                  where id_ac=P_ID_AC and
                        id_ws=P_ID_WS and
                        id_bort=P_ID_BORT
                  order by case when CODE_NAME_1 is null then 'NULL' else CODE_NAME_1 end,
                           case when CODE_NAME_2 is null then 'NULL' else CODE_NAME_2 end) e
                  order by e.CODE_NAME_1,
                           e.CODE_NAME_2;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;
      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_DOW_SW_C_DATA;
/
