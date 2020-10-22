create or replace procedure SP_WB_REF_WS_AIR_SEC_B_TT_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

  select count(iata.id) into V_R_COUNT
  from WB_REF_WS_AIR_SEC_BAY_T t join WB_REF_AIRCO_ULD u
  on t.id=P_ID and
     t.id_ac=u.id_ac join WB_REF_ULD_IATA iata
     on iata.id=u.ULD_IATA_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("uld_iata_list",
                  xmlattributes(q.id as "id",
                                  q.name as "name"))).getClobVal() into cXML_data
      from (select distinct to_char(iata.id) as id,
                            iata.name
            from WB_REF_WS_AIR_SEC_BAY_T t join WB_REF_AIRCO_ULD u
            on t.id=P_ID and
               t.id_ac=u.id_ac join WB_REF_ULD_IATA iata
               on iata.id=u.ULD_IATA_ID
            order by iata.name) q
      order by q.name;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEC_BAY_TT
  where T_ID=P_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("table_data",
                  xmlattributes(to_char(ULD_IATA_ID) as "ULD_IATA_ID"))).getClobVal() into cXML_data
      from WB_REF_WS_AIR_SEC_BAY_TT
      where T_ID=P_ID;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEC_BAY_T
  where id=P_ID;

  if V_R_COUNT>0 then
    begin
      select '<uld_iata_string string_data="'||WB_CLEAR_XML(WB_WS_AIR_SEC_BAY_T_ULD_STR(P_ID))||'"/>' INTO cXML_data
      from dual;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SEC_B_TT_DATA;
/
