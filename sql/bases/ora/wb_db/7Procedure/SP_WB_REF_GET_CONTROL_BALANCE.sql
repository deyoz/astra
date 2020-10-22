create or replace PROCEDURE SP_WB_REF_GET_CONTROL_BALANCE(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl balance
-->
<root name="get_load_control_balance" result="ok">
    <Estimated DOI="45,1" LI_ZFW="45" LI_TOW="41" LI_LAW="44" ZFW_MAC="46568" TOW_MAC="58568" LAW_MAC="50068"/>
    <Actual DOI="45,7" LI_ZFW="50" LI_TOW="56" LI_LAW="52" ZFW_MAC="47000" TOW_MAC="60000" LAW_MAC="51000"/>
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_BALANCE;
/
