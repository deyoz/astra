create or replace PROCEDURE SP_WB_REF_GET_FUEL_INFO_OLD
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;
P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC
  into vID_AC
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  select t1.ID_WS
  into vID_WS
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  select t1.ID_BORT
  into vID_BORT
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("fuel_info",
                              XMLATTRIBUTES(tt1.ID_AC "ID_AC", tt1.ID_WS "ID_WS", tt1.ProcName "ProcName", tt1.ProcType "ProcType", tt1.fuel_distrib "fuel_distrib",
                                            tt1.density "density", tt1.density_units "density_units", tt1.block_fuel "block_fuel", tt1.taxi_fuel "taxi_fuel", tt1.trip_fuel "trip_fuel",
                                            tt1.ballast_fuel "ballast_fuel", tt1.additional_fuel "additional_fuel", tt1.max_fuel "max_fuel", tt1.min_fuel "min_fuel", tt1.index_unit "index_unit",
                                            tt1.weight_units "weight_units")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ID_AC, t1.ID_WS, t1.PROC_NAME ProcName, ' ' ProcType, ' ' fuel_distrib, to_char(t1.DENSITY, 'FM990.999') density, t4.NAME_RUS_SMALL density_units, 14000 block_fuel, 200 taxi_fuel, -- 0d90
        5000 trip_fuel, 5000 ballast_fuel, 1000 additional_fuel, min(t2.WEIGHT) min_fuel, max(t2.WEIGHT) max_fuel, to_char(max(t2.INDEX_UNIT), 'FM99990.999') index_unit,
        t5.NAME_RUS_SMALL weight_units
    from WB_REF_WS_AIR_CFV_ADV t1 inner join WB_REF_WS_AIR_CFV_TBL t2 on t1.ID = t2.IDN and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS and t1.ID_BORT = t2.ID_BORT
                                  inner join WB_REF_WS_AIR_MEASUREMENT t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.ID_BORT = t3.ID_BORT
                                  inner join WB_REF_MEASUREMENT_DENSITY t4 on t3.DENSITY_ID_FUEL = t4.ID
                                  inner join WB_REF_MEASUREMENT_WEIGHT t5 on t3.WEIGHT_ID = t5.ID
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT
    group by t1.ID_AC, t1.ID_WS, t1.PROC_NAME, t1.DENSITY, t4.NAME_RUS_SMALL, t5.NAME_RUS_SMALL --, t2.INDEX_UNIT
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_fuel_info" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_FUEL_INFO_OLD;
/
