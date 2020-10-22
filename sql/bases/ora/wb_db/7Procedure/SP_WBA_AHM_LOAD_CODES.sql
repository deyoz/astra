create or replace PROCEDURE SP_WBA_AHM_LOAD_CODES(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  select t1.ID_AC
  into vID_AC
  from WB_SHED t1
  where t1.ID = P_ID and rownum <= 1;

  select XMLAGG(
                  XMLELEMENT("code",
                              XMLATTRIBUTES(tt1.ID "id",
                                            tt1.CODE_NAME "Code",
                                            tt1.DESCRIPTION "Description")
                            ) order by tt1.CODE_NAME
                )
  INTO cXML_data
  from
  (
    select t1.ID, t1.CODE_NAME, t1.DESCRIPTION
    from WB_REF_AIRCO_LOAD_CODES t1
    where t1.ID_AC = vID_AC
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
END SP_WBA_AHM_LOAD_CODES;
/
