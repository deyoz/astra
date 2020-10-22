create or replace procedure SP_WB_REF_WS_AIR_S_L_C_LIST
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
V_R_COUNT number:=0;
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_S_L_C_IDN
    where adv_id=P_ADV_ID;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.table_name "table_name"))).getClobVal() into cXML_data
        from (select id,
                       table_name
              from WB_REF_WS_AIR_S_L_C_IDN
              where adv_id=P_ADV_ID
              order by table_name,
                       id) e
        order by e.table_name,
                 e.id;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


    cXML_out:=cXML_out||'</root>';

  end SP_WB_REF_WS_AIR_S_L_C_LIST;
/
