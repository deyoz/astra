create or replace PROCEDURE SP_WB_REF_GET_DOW_DATA_OLD(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
REC_COUNT2 number:=0;
TABLEVAR varchar(40):='DOWData';
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ELEM_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cXML_data
    from
    (
      select t1.XML_VALUE
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID
    ) t1
    where rownum <= 1;
  end;
  else
  begin
    select count(t1.ELEM_ID)
    into REC_COUNT2
    from WB_TMP_XML t1
    where t1.ELEM_ID = P_ELEM_ID and t1.TABLENAME = TABLEVAR;

    if REC_COUNT2 > 0 then
    begin
      select t1.XML_VALUE
      INTO cXML_data
      from
      (
        select t1.XML_VALUE
        from WB_TMP_XML t1
        where t1.TABLENAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID
      ) t1
      where rownum <= 1;
    end;
    else
    begin
      cXML_data := '<?xml version="1.0" encoding="utf-8"?><root name="get_dow_data" result="ok"></root>';
    end;
    end if;
  end;
  end if;

  cXML_out:=cXML_data;
END SP_WB_REF_GET_DOW_DATA_OLD;
/
