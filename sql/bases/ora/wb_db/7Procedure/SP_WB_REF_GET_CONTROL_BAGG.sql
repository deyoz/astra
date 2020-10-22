create or replace PROCEDURE SP_WB_REF_GET_CONTROL_BAGG(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select count(t1.ELEM_ID) into REC_COUNT
  from WB_TMP_XML t1
  where t1.ELEM_ID = P_ELEM_ID and t1.TABLENAME = 'LoadControlBagg';

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cXML_data
    from
    (
      select t1.XML_VALUE
      from WB_TMP_XML t1
      where t1.TABLENAME = 'LoadControlBagg' and t1.ELEM_ID = P_ELEM_ID
    ) t1
    where rownum <= 1;
  end;
  else
  begin
    cXML_data := '<?xml version="1.0" encoding="utf-8"?><root name="get_load_control_bagg" result="ok"></root>';
  end;
  end if;

/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Информация для LoadControl baggage
-->
<root name="get_load_control_bagg" result="ok">
	    <Class
		Code="F"
		Booked="50"
		CheckIn="50"
		Transit="0"
	    />
	    <Class
		Code="C"
		Booked="150"
		CheckIn="120"
		Transit="0"
	    />
	    <Class
		Code="Y"
		Booked="1000"
		CheckIn="500"
		Transit="20"
	    />
</root>';
*/

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_BAGG;
/
