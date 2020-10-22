create or replace PROCEDURE SP_WB_REF_GET_CONTROL_PASS(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl пассажиры
-->
<root name="get_load_control_pass" result="ok">
	    <Class
		Code="F"
		Booked="1"
		CheckIn="1"
		Transit="0"
	    />
	    <Class
		Code="C"
		Booked="5"
		CheckIn="4"
		Transit="0"
	    />
	    <Class
		Code="Y"
		Booked="50"
		CheckIn="23"
		Transit="1"
	    />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_PASS;
/
