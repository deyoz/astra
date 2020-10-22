create or replace procedure SP_WB_SAVE_LOADINSTRUCTION
(cXML_in in clob,
   cXML_out out clob)
as
-- сохранение текущих данных для Load Instruction в базе
P_ELEM_ID number := -1;
P_DATE varchar(20);
P_TIME varchar(20);
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'LoadInstruction';
DT_LAST date;
cXML_LI clob;
cXML_Data clob;
cXML_in_2 clob;

begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  -- Получение текущих данных
  cXML_in_2 := '<?xml version="1.0"?>'
              || '<root name="get_loadinstruction"'
              || ' elem_id="' || to_char(P_ELEM_ID) || '"'
              || '>'
              || '</root>';

  SP_WB_GET_LOADINSTRUCTION(cXML_in_2, cXML_Data);

  -- Получаем данные по последнему сохраненному LoadInstruction
  cXML_LI := '';

  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where (DATA_NAME = TABLEVAR) and (t1.ELEM_ID = P_ELEM_ID);

  -- Если есть данные
  if REC_COUNT > 0 then
  begin
    select max(t1.DT), to_char(max(t1.DT), 'DD.MM.YYYY'), to_char(max(t1.DT), 'HH24:MI:SS')
    into DT_LAST, P_DATE, P_TIME
    from WB_CALCS_XML t1
    where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID;

    select t1.XML_VALUE
    into cXML_LI
    from WB_CALCS_XML t1
    where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID and t1.DT = DT_LAST;
  end;
  end if;

  -- Сравнить данные из cXML_LS и cXML_Data
  if (cXML_LI is null) or (SYS.DBMS_LOB.COMPARE(cXML_Data, cXML_LI) != 0) then
  begin
    -- Сохранить данные в базу
    insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
    values (P_ELEM_ID, TABLEVAR, cXML_data);

    select to_char(max(t1.DT), 'DD.MM.YYYY'), to_char(max(t1.DT), 'HH24:MI:SS')
    into P_DATE, P_TIME
    from WB_CALCS_XML t1
    where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID;
  end;
  end if;

  cXML_out := '<?xml version="1.0"  encoding="utf-8"?>'
              || '<root name="save_instruction" result="ok"'
              || ' elem_id="' || to_char(P_ELEM_ID) || '"'
              || ' date="' || P_DATE || '"'
              || ' time="' || P_TIME || '"'
              || '>'
              || '</root>';
  commit;
end SP_WB_SAVE_LOADINSTRUCTION;
/
