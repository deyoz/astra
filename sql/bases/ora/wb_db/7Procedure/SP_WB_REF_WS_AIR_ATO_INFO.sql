create or replace PROCEDURE SP_WB_REF_WS_AIR_ATO_INFO(cXML_in IN clob,
                                                        cXML_out OUT CLOB)
AS
P_ID number:=-1;
P_LANG varchar2(50):='';
cXML_data clob:='';
R_COUNT number:=0;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id) into R_COUNT
    from WB_REF_WS_AIR_ATO
    where id=P_ID;

    if R_COUNT>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select '<list DATE_FROM="'||to_char(i.DATE_FROM, 'dd.mm.yyyy')||'" '||
                     'SORT_CONTAINER_LOADING="'||to_char(i.SORT_CONTAINER_LOADING)||'" '||
                     'EZFW_THRESHOLD_LIMIT="'||to_char(i.EZFW_THRESHOLD_LIMIT)||'" '||
                     'EZFW_THRESHOLD_LIMIT_VAL="'||nvl(to_char(i.EZFW_THRESHOLD_LIMIT_VAL), 'NULL')||'" '||
                     'CHECK_LATERAL_IMBALANCE_LIMITS="'||to_char(i.CHECK_LATERAL_IMBALANCE_LIMITS)||'" '||
                     'PTO_CLASS_TRIM="'||to_char(i.PTO_CLASS_TRIM)||'" '||
                     'PTO_CABIN_AREA_TRIM="'||to_char(i.PTO_CABIN_AREA_TRIM)||'" '||
                     'PTO_SEAT_ROW_TRIM="'||to_char(i.PTO_SEAT_ROW_TRIM)||'" '||
                     'REMARK="'||WB_CLEAR_XML(i.REMARK)||'" '||
                     'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                     'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                     'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                     'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           into cXML_data
           from WB_REF_WS_AIR_ATO i
           where i.id=P_ID;

        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_REF_WS_AIR_ATO_INFO;
/
