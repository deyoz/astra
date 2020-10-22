create or replace PROCEDURE SP_WBA_AHM_ULD_TYPES(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  select t1.ID_AC, t1.ID_WS
  into vID_AC, vID_WS
  from WB_SHED t1
  where t1.ID = P_ID and rownum <= 1;

  select XMLAGG(
                  XMLELEMENT("uld_type",
                              XMLATTRIBUTES(tt1.ID "id", tt1.IATAType "IATAType", tt1.IsDefault "IsDefault", tt1.IsPallet "IsPallet", tt1.BeginSerial "BeginSerial",
                                            tt1.EndSerial "EndSerial", tt1.OwnerCode "OwnerCode", tt1.TareWeight "TareWeight", tt1.MaxWeight "MaxWeight", tt1.IATAType "Code")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ID, t3.NAME IATAType,
      case when t1.BY_DEFAULT = 1 then 'Y' else 'N' end IsDefault,
      case when t1.ULD_TYPE_ID = 0 then 'Y' else 'N' end IsPallet,
      t1.BEG_SER_NUM BeginSerial,
      case
        when t1.END_SER_NUM = 'EMPTY_STRING' then ' '
        when t1.END_SER_NUM is null then ' '
        else t1.END_SER_NUM
      end EndSerial,
      t1.OWNER_CODE OwnerCode, t1.TARE_WEIGHT TareWeight, t1.MAX_WEIGHT MaxWeight
    from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_TYPES t2 on t1.ULD_TYPE_ID = t2.ID
                              inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
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
END SP_WBA_AHM_ULD_TYPES;
/
