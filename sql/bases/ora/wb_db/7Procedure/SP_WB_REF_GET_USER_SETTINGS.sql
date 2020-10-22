create or replace PROCEDURE SP_WB_REF_GET_USER_SETTINGS(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
TABLEVAR varchar(40):='ElemIdUserSettings';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
    begin
      select t1.XML_VALUE
      INTO cXML_out
      from
      (
        select t1.XML_VALUE
        from WB_CALCS_XML t1
        where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
      ) t1
      where rownum <= 1;
    end;
    else
    -- Берем данные по умолчанию
    begin
      cXML_out := '<?xml version="1.0" ?><root>';

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

      if cXML_data is not NULL then
      begin
        cXML_out := '<?xml version="1.0" ?><root name="get_user_settings" result="ok">' || cXML_data.getClobVal() || '</root>';
      end;
      else
      begin
        cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_user_settings" result="ok1"></root>';
      end;
      end if;
    end;
  end if;
END SP_WB_REF_GET_USER_SETTINGS;
/
