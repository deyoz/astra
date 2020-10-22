create or replace PROCEDURE SP_WB_REF_GET_CONTROL_ZFW (cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl ZFW колодец самолет без топлива
-->
<root name="get_load_control_zfw" result="ok">
    <list index="30.08" wheit="35000" />
    <list index="13.19" wheit="60250" />
    <list index="13.45" wheit="61688" />
    <list index="76.98" wheit="61688" />
    <list index="61.18" wheit="39470" />
    <list index="55.73" wheit="35000" />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_ZFW;
/
