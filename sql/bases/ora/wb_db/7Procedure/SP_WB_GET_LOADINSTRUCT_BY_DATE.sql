create or replace PROCEDURE SP_WB_GET_LOADINSTRUCT_BY_DATE
(cXML_in in clob, cXML_out out clob)
AS
-- получение текста на основен данных LoadSheet
cXML_Data clob; cXML_LS clob;
cLoadsheet_Text varchar(8000) := '';
DT_LAST date;
TABLEVAR varchar(40) := 'LoadInstruction';
REC_COUNT number := 0;
P_ELEM_ID number;
P_DATE varchar(20) := '';
P_TIME varchar(20) := '';
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')), extractValue(xmltype(cXML_in),'/root[1]/@date'), extractValue(xmltype(cXML_in),'/root[1]/@time')
  into P_ELEM_ID, P_DATE, P_TIME
  from dual;

  cXML_out := '';

  if (P_DATE is not null) and (P_TIME is not null) then -- получаем сохраненные данные из базы
  begin
    select count(t1.ELEM_ID)
    into REC_COUNT
    from WB_CALCS_XML t1
    where (DATA_NAME = TABLEVAR) and (ELEM_ID = P_ELEM_ID) and (to_char(t1.DT, 'DD.MM.YYYY') = P_DATE) and (to_char(t1.DT, 'HH24:MI:SS') = P_TIME);

    if REC_COUNT > 0 then
    begin
      select t1.XML_VALUE
      into cXML_out
      from WB_CALCS_XML t1
      where (DATA_NAME = TABLEVAR) and (ELEM_ID = P_ELEM_ID) and (to_char(t1.DT, 'DD.MM.YYYY') = P_DATE) and (to_char(t1.DT, 'HH24:MI:SS') = P_TIME) and rownum <= 1;
    end;
    end if;
  end;
  end if;

  -- корневые теги
  -- cXML_out := '<root name="get_loadinstruct_by_date" result="ok">' || cXML_out || '</root>';
  -- cXML_out := '<?xml version="1.0" ?>' || cXML_out;

  select replace(cXML_out, 'name="get_loadinstruction"', 'name="get_loadinstruction_by_date"') into cXML_out from dual;

  commit;
END SP_WB_GET_LOADINSTRUCT_BY_DATE;
/
