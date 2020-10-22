create or replace procedure SP_WB_GET_LOADSHEET_SAVED
(cXML_in in clob,
   cXML_out out clob)
as
P_ELEM_ID number := -1;
P_DATE varchar(20);
P_TIME varchar(20);
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'LoadSheet';
DT_LAST date;
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')), extractValue(xmltype(cXML_in),'/root[1]/@date'), extractValue(xmltype(cXML_in),'/root[1]/@time')
  into P_ELEM_ID, P_DATE, P_TIME
  from dual;

  cXML_out := '';

  if (P_DATE = '') or (P_TIME = '') then -- берем последний сохраненный документ
  begin
    select max(t1.DT)
    into DT_LAST
    from WB_CALCS_XML t1
    where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ELEM_ID;

    select t1.XML_VALUE
    into cXML_out
    from WB_CALCS_XML t1
    where t1.DATA_NAME = 'LoadSheet' and t1.ELEM_ID = P_ELEM_ID and t1.DT = DT_LAST and rownum <= 1;
  end;
  else
  begin -- документ с нужной датой сохранения
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

  if (cXML_out = '') or (cXML_out is null) then
  begin
    cXML_out := '<?xml version="1.0"?>'
              || '<root name="get_loadsheet_saved" result="ok"'
              || '</root>';
  end;
  end if;

  commit;
end SP_WB_GET_LOADSHEET_SAVED;
/
