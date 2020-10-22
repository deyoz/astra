create or replace procedure SP_WB_REF_WS_AIR_ULD_OVR_OVR
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_POSITION varchar2(100):='';

cXML_data clob;
V_R_COUNT number:=0;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_POSITION[1]') into P_POSITION from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEC_BAY_T
  where ID_AC=P_ID_AC and
        ID_WS=P_ID_WS and
        ID_BORT=P_ID_BORT and
        SEC_BAY_NAME<>P_POSITION;

  if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("list_data",
                    xmlattributes(q.SEC_BAY_NAME as "SEC_BAY_NAME"))).getClobVal() into cXML_data
        from (select distinct h.SEC_BAY_NAME
              from WB_REF_WS_AIR_SEC_BAY_T h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT and
                    h.SEC_BAY_NAME<>P_POSITION
              order by h.SEC_BAY_NAME) q
      order by q.SEC_BAY_NAME;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_ULD_OVER
  where ID_AC=P_ID_AC and
        ID_WS=P_ID_WS and
        ID_BORT=P_ID_BORT and
        POSITION=P_POSITION;

  if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.OVERLAY as "OVERLAY"))).getClobVal() into cXML_data
        from (select distinct OVERLAY
              from WB_REF_WS_AIR_ULD_OVER
              where ID_AC=P_ID_AC and
                    ID_WS=P_ID_WS and
                    ID_BORT=P_ID_BORT and
                    POSITION=P_POSITION
              order by OVERLAY) q
      order by q.OVERLAY;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_ULD_OVR_OVR;
/
