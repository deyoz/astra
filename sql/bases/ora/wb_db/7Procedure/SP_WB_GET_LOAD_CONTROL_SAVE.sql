create or replace PROCEDURE SP_WB_GET_LOAD_CONTROL_SAVE(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_ID number:=-1;
REC_COUNT number:=0;
vDATE varchar(20):='';
vTIME varchar(20):='';
vDT varchar(20):=''; -- date := sysdate;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  select to_char(extractValue(xmltype(cXML_in),'/root[1]/@date')) --, 'DD.MM.YYYY')
  into vDATE
  from dual;

  select to_char(extractValue(xmltype(cXML_in),'/root[1]/@time')) --, 'HH24:MI')
  into vTIME
  from dual;

  -- select to_date(vDATE || ' ' || vTIME, 'DD.MM.YYYY HH24:MI')
  select to_char(vDATE || ' ' || vTIME)
  into vDT
  from dual;

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_LOAD_CONTROL_XML t1
  where t1.ELEM_ID = P_ID and to_char(t1.DT, 'DD.MM.YYYY HH24:MI') = vDT;

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cXML_out
    from
    (
      select t1.XML_VALUE
      from WB_CALCS_LOAD_CONTROL_XML t1
      where t1.ELEM_ID = P_ID and to_char(t1.DT, 'DD.MM.YYYY HH24:MI') = vDT
    ) t1
    where rownum <= 1;
  end;
  else
  -- Нет данных
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_load_control" result="ok"></root>';
  end;
  end if;
END SP_WB_GET_LOAD_CONTROL_SAVE;
/
