create or replace PROCEDURE SP_WBA_AHM_AIRLINE_INFO(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по типу ВС
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  select t1.ID_AC, t1.ID_WS, t1.ID_BORT
  into vID_AC, vID_WS, vID_BORT
  from WB_SHED t1
  where t1.ID = P_ID;

  select XMLAGG(
                  XMLELEMENT("units_of_measure",
                              XMLATTRIBUTES('' "ID"),
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "weight",
                                                          XMLATTRIBUTES(tt1.weight as "units")
                                                        )
                                            ) from dual
                              ) as cls,
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "length",
                                                          XMLATTRIBUTES(tt1.length as "units")
                                                        )
                                            ) from dual
                              ) as cls,
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "volume",
                                                          XMLATTRIBUTES(tt1.volume as "units")
                                                        )
                                            ) from dual
                              ) as cls,
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "liguid_volume",
                                                          XMLATTRIBUTES(tt1.liguid_volume as "units")
                                                        )
                                            ) from dual
                              ) as cls,
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "fuel_density",
                                                          XMLATTRIBUTES(tt1.fuel_density as "units")
                                                        )
                                            ) from dual
                              ) as cls,
                              (SELECT XMLAGG(
                                              XMLELEMENT(
                                                          "moment",
                                                          XMLATTRIBUTES(tt1.moment as "units")
                                                        )
                                            ) from dual
                              ) as cls
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ID, t2.NAME_ENG_SMALL weight, t4.NAME_ENG_SMALL length, t3.NAME_ENG_SMALL volume, '1' liguid_volume, t5.NAME_ENG_SMALL fuel_density, t6.NAME_ENG_SMALL moment
    from WB_REF_WS_AIR_MEASUREMENT t1 inner join WB_REF_MEASUREMENT_WEIGHT t2 on t1.WEIGHT_ID = t2.ID
                                      inner join WB_REF_MEASUREMENT_VOLUME t3 on t1.VOLUME_ID = t3.ID
                                      inner join WB_REF_MEASUREMENT_LENGTH t4 on t1.LENGTH_ID = t4.ID
                                      inner join WB_REF_MEASUREMENT_DENSITY t5 on t1.DENSITY_ID_FUEL = t5.ID
                                      inner join WB_REF_MEASUREMENT_MOMENTS t6 on t1.MOMENTS_ID = t6.ID
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
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
END SP_WBA_AHM_AIRLINE_INFO;
/
