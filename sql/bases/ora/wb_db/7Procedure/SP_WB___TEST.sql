create or replace PROCEDURE SP_WB___TEST(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
P_id_ac number:=-1;
P_id_ws number:=-1;
cXML_data clob:='';
  BEGIN
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_id_ac[1]') into P_id_ac from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_id_ws[1]') into P_id_ws from dual;
    cXML_out:='<?xml version="1.0" ?><root>';


    select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.value) "value"))).getClobVal() into cXML_data
        from (select min(weight) as value from WB_REF_WS_AIR_CFV_TBL
WHERE (ID_AC=P_id_ac) and (ID_WS=P_id_ws)) e;

     cXML_out:=cXML_out||cXML_data||'</root>';

  END SP_WB___TEST;
/
