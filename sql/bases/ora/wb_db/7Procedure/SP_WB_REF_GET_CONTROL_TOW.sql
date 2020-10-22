create or replace PROCEDURE SP_WB_REF_GET_CONTROL_TOW(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl TOW колодец самолет на взлете
-->
<root name="get_load_control_tow" result="ok">
    <list index="30.08" wheit="35000" />
    <list index="13.19" wheit="60250" />
    <list index="14.15" wheit="65543" />
    <list index="23.31" wheit="75000" />
    <list index="84.19" wheit="75000" />
    <list index="87.52" wheit="73055" />
    <list index="44.48" wheit="35000" />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_TOW;
/
