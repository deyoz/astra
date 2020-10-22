create or replace PROCEDURE SP_WB_GET_LOADSHEET_TEXT_TEMP
(cXML_in in clob, cXML_out out clob)
AS
-- получение текста на основен данных LoadSheet
cXML_Data clob; cXML_LS clob;
f1 char(3); f2 char(3); f3a char(8); f3b char(2); f4 char(6); f5 char(11); f6 char(7); f7 char(7); f8 char(4);
f9a number; f9b varchar(3); f10 number; f11 varchar(1000); f11a char(32); f11b char(32); f12 number; f13 varchar(50); f14 varchar(50); f15 varchar(50);
f16 varchar(50); f13_14_15_16 char(15); f17 number; f18 number; f19 char(3); f20 char(2); f21 char(11); f22 char(8); f23 char(8); f24 varchar(50);
f25 varchar(50); f26 varchar(50); f27 varchar(50); f28 varchar(50); f29 varchar(50); f30 varchar(50); f31 varchar(50); f32 varchar(50); f33 varchar(50); f34a char(1);
f34b char(1); f34c char(1); f35 number; f36a varchar(100); f36b varchar(100); f36c varchar(100); f36d varchar(100); f36e varchar(100); f36f varchar(100); f36g varchar(100);
f36h varchar(100); f42 varchar(50); f47 char(12) := '            '; f48 varchar(50);
cLoadsheet_Text varchar(8000) := '';
DT_LAST date;
TABLEVAR varchar(40) := 'LoadSheet';
P_ELEM_ID number;
P_DATE varchar(20);
P_TIME varchar(20);
cXML_in_2 clob;
sTest varchar(1000) := '';
begin
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')), extractValue(xmltype(cXML_in),'/root[1]/@date'), extractValue(xmltype(cXML_in),'/root[1]/@time')
  into P_ELEM_ID, P_DATE, P_TIME
  from dual;

  cXML_Data := '';
  cLoadsheet_Text := '';

  if (nvl(P_DATE, ' ') != ' ') and (nvl(P_TIME, ' ') != ' ') then -- получаем сохраненные данные из базы
  begin
    cXML_in_2 := '<?xml version="1.0"?>'
                || '<root name="get_loadsheet_saved"'
                || ' elem_id="' || to_char(P_ELEM_ID) || '"'
                || ' date="' || P_DATE || '"'
                || ' time="' || P_TIME || '"'
                || '>'
                || '</root>';

    SP_WB_GET_LOADSHEET_SAVED(cXML_in_2, cXML_Data);
  end;
  end if;

  -- Сравнить cXML_LS и cXML_Data

  -- Если разные, то сохранить
  /*
  insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
  values (P_ELEM_ID, TABLEVAR, cXML_Data);
  */

  -- Нужна процедура, получающая из XML текст. Ее вызывать в SP_WB_GET_LOADSHEET_SAVED

  -- корневые теги
  cXML_out := cXML_Data;
  -- cXML_out := cXML_in_2;

  commit;
END SP_WB_GET_LOADSHEET_TEXT_TEMP;
/
