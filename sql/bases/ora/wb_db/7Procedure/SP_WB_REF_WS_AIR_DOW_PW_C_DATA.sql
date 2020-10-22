create or replace procedure SP_WB_REF_WS_AIR_DOW_PW_C_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';


    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_PW_CODES
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("portable_water_code_list", xmlattributes(e.id as "id",
                                                                             e.PW_CODE_NAME "PW_CODE_NAME",
                                                                               e.TOTAL_WEIGHT as "TOTAL_WEIGHT",
                                                                                 e.WEIHGT_DIFF as "WEIHGT_DIFF",
                                                                                   e.ARM_DIFF as "ARM_DIFF",
                                                                                     e.INDEX_DIFF as "INDEX_DIFF",
                                                                                       e.CODE_TYPE as "CODE_TYPE",
                                                                                         e.REMARKS as "REMARKS",
                                                                                           e.BY_DEFAULT as "BY_DEFAULT",
                                                                                             e.U_NAME as "U_NAME",
                                                                                               e.U_IP as "U_IP",
                                                                                                 e.U_HOST_NAME as "U_HOST_NAME",
                                                                                                   e.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(id) as id,
                       case when PW_CODE_NAME is null then 'NULL' else PW_CODE_NAME end as PW_CODE_NAME,
                         case when TOTAL_WEIGHT is null then 'NULL' else to_char(TOTAL_WEIGHT) end as TOTAL_WEIGHT,
                           case when WEIHGT_DIFF is null then 'NULL' else to_char(WEIHGT_DIFF) end as WEIHGT_DIFF,
	                           case when ARM_DIFF is null then 'NULL' else to_char(ARM_DIFF) end as ARM_DIFF,
                               case when INDEX_DIFF is null then 'NULL' else to_char(INDEX_DIFF) end as INDEX_DIFF,
	                               case when CODE_TYPE is null then 'NULL' else CODE_TYPE end as CODE_TYPE,
                                   case when REMARKS is null then 'NULL' else REMARKS end as REMARKS,
                                     to_char(BY_DEFAULT) as BY_DEFAULT,
                                       U_NAME,
                                         U_IP,
                                           U_HOST_NAME,
                                             to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_REF_WS_AIR_DOW_PW_CODES
              where id_ac=P_ID_AC and
                    id_ws=P_ID_WS and
                    id_bort=P_ID_BORT
              order by case when PW_CODE_NAME is null then 'NULL' else PW_CODE_NAME end) e
              order by e.PW_CODE_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
   end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_DOW_PW_C_DATA;
/
