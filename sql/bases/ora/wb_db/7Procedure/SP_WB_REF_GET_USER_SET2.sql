create or replace PROCEDURE SP_WB_REF_GET_USER_SET2(cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.XML_VALUE
  INTO cXML_data
  from
  (
    select t1.XML_VALUE
    from WB_TMP_XML t1
    where t1.TABLENAME = 'ElemIdUserSettings' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_user_settings" elem_id="3458" result="ok">
  <deadload_image scale="16" HiddenULDTypes=""/>
  <application mode="0"/>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_USER_SET2;
/
