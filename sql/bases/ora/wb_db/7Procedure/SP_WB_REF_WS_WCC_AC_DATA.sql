create or replace procedure SP_WB_REF_WS_WCC_AC_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

  select count(u.id) into V_R_COUNT
  from WB_REF_WS_AIR_WCC t join WB_REF_WS_AIR_DOW_SWA_CODES u
  on t.ID=P_ID and
     t.ID_AC=u.ID_AC and
     t.ID_WS=u.ID_WS and
     t.ID_BORT=u.ID_BORT;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("adjustment_codes_list",
                  xmlattributes(q.id as "id",
                                  q.ac_name as "ac_name"))).getClobVal() into cXML_data
      from (select distinct to_char(h.id) as id,
                             h.CODE_NAME_1 ac_name
            from WB_REF_WS_AIR_WCC t join WB_REF_WS_AIR_DOW_SWA_CODES h
            on t.ID=P_ID and
               t.ID_AC=h.ID_AC and
               t.ID_WS=h.ID_WS and
               t.ID_BORT=h.ID_BORT
            order by h.CODE_NAME_1) q
      order by q.ac_name;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_WCC_AC
  where WCC_ID=P_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("table_data",
                  xmlattributes(to_char(ADJ_CODE_ID) as "ADJ_CODE_ID"))).getClobVal() into cXML_data
      from WB_REF_WS_AIR_WCC_AC
      where WCC_ID=P_ID;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_WCC
  where id=P_ID;

  if V_R_COUNT>0 then
    begin
      select '<ac_string string_data="'||WB_WS_AIR_WCC_AC_STR(P_ID)||'"/>' INTO cXML_data
      from dual;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_WCC_AC_DATA;
/
