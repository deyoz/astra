create or replace PROCEDURE SP_WBA_USER_SETTINGS(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с настройками пользователя
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
TABLEVAR varchar(40):='ElemIdUserSettings';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
    begin
      select t1.XML_DATA
      INTO cXML_out
      from
      (
        select t1.XML_DATA
        from WB_CALCS_XML t1
        where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
      ) t1
      where rownum <= 1;

      if cXML_out is null then
        cXML_out := '';
      end if;
    end;
  end if;

  if (REC_COUNT <= 0) or (cXML_out is null) then
    -- Берем данные по умолчанию
    begin
      cXML_out := '';

      select XMLAGG(
                      XMLELEMENT("deadload_image",
                                  XMLATTRIBUTES(tt1.scale "scale")
                                )
                    )
      INTO cXML_data
      from
      (
        select 9 scale
        from dual
      ) tt1;

      if cXML_Data is not null then
      begin
        cXML_out := cXML_data.getClobVal();
      end;
      else
      begin
        cXML_out := '';
      end;
      end if;
    end;
  end if;
END SP_WBA_USER_SETTINGS;
/
