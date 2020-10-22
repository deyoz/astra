create or replace PROCEDURE SP_WB_REF_GET_CONTROL_WHEIT(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl wheit summary
-->
<root name="get_load_control_wheit" result="ok">
    <list load="PAX" Estimated="5200" Actual="3250" Maximum="" />
    <list load="BAG" Estimated="1050" Actual="543"	Maximum="" />
    <list load="Cargo" Estimated="250" Actual="143"	Maximum="" />
    <list load="Mail" Estimated="70" Actual="57"	Maximum="" />
    <list load="Transit Load" Estimated="0" Actual="0"	Maximum="" />
    <list load="Total Traffic Load" Estimated="6570" Actual="3993" Maximum="" />
    <list load="Allowed Traffic Load" Estimated="8500" Actual="8500" Maximum="" />
    <list load="" Estimated="" Actual="" Maximum="" />
    <list load="Dry Operating Weight" Estimated="42500" Actual="42575"	Maximum="" />
    <list load="Zero fuel Weight" Estimated="49070" Actual="46568"	Maximum="61000" />
    <list load="Take off Fuel" Estimated="12000" Actual="12000"	Maximum="" />
    <list load="Take off Weight" Estimated="61070" Actual="58568"	Maximum="70000" />
    <list load="Trip Fuel" Estimated="8500" Actual="8500"	Maximum="" />
    <list load="Landing Weight" Estimated="1050" Actual="543"	Maximum="65000" />
    <list load="" Estimated="" Actual=""	Maximum="" />
    <list load="Underload Befor LMC" Estimated="1930" Actual="4507"	Maximum="" />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_WHEIT;
/
