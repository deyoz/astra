create or replace PROCEDURE SP_WB_GET_LOAD_CONTROL_MENU(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
-- Возвращает набор
cXML_data XMLType;
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
REC_COUNT2 number:=0;
TABLEVAR varchar(40):='DOWData';
vDATE varchar(20):='';
vTIME varchar(20):='';
vDT date := sysdate;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("list",
                              XMLATTRIBUTES(tt1.id_open "id_open", tt1.ELEM_ID "elem_id", tt1.data "data", tt1.time "time")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ELEM_ID, to_char(t1.DT, 'DD.MM.YYYY') data, to_char(t1.DT, 'HH24:MI') time, ROW_NUMBER() OVER (ORDER BY t1.DT) AS id_open -- (PARTITION BY t1.DT ORDER BY t1.DT)
    from WB_CALCS_LOAD_CONTROL_XML t1
    where t1.ELEM_ID = P_ELEM_ID
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_load_control-open-save" elem_id="' || to_char(P_ELEM_ID) || '" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

/*
<?xml version="1.0" encoding="utf-8"?>
<root name="get_load_control-open-save" elem_id="3457" result="ok">
<list id_open="10" elem_id="3457" data="10.12.2016" time="10:58"/>
<list id_open="11" elem_id="3457" data="11.12.2016" time="11:40"/>
<list id_open="12" elem_id="3457" data="12.12.2016" time="12:40"/>
</root>
*/
END SP_WB_GET_LOAD_CONTROL_MENU;
/
