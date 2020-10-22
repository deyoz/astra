create or replace PROCEDURE SP_WB_REF_GET_CONTROL_LAW(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl LAW колодец самолет на посадке
-->
<root name="get_load_control_law" result="ok">
    <list index="30.08" wheit="35000" />
    <list index="13.19" wheit="60250" />
    <list index="14.11" wheit="65317" />
    <list index="37.04" wheit="66360" />
    <list index="84.24" wheit="66360" />
    <list index="65.5" wheit="40000" />
    <list index="59.43" wheit="35000" />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_LAW;
/
