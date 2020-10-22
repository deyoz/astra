create or replace PROCEDURE SP_WB_REF_GET_AHM_AIRLINE_INFO
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC
  into vID_AC
  from WB_SCHED t1 -- WB_REF_WS_SCHED_TEMP
  where t1.ID = P_ID;

  select t1.ID_WS
  into vID_WS
  from WB_SCHED t1
  where t1.ID = P_ID;

  select t1.ID_BORT
  into vID_BORT
  from WB_SCHED t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

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

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_airline_info" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_airline_info" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_AHM_AIRLINE_INFO;
/
