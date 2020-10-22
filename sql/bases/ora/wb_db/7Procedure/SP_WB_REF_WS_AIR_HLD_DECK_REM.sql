create or replace procedure SP_WB_REF_WS_AIR_HLD_DECK_REM
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data CLOB:='';
R_COUNT number:=0;
  begin
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into R_COUNT
    from WB_REF_WS_AIR_HLD_DECK_REM
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if R_COUNT>0 then
      begin
        select '<adv_data REMARKS="'||WB_CLEAR_XML(i.REMARKS)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
        from WB_REF_WS_AIR_HLD_DECK_REM i
        where i.id_ac=P_ID_AC and
              i.id_ws=P_ID_WS and
              i.id_bort=P_ID_BORT;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';
  end SP_WB_REF_WS_AIR_HLD_DECK_REM;
/
