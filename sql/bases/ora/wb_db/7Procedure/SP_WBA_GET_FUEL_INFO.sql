create or replace PROCEDURE SP_WBA_GET_FUEL_INFO
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;
P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number; vBlockFuel number; vTaxiFuel number; vTripFuel number;
REC_COUNT number:=0;
cFuelInfo clob;
TABLEVAR varchar(40):='FuelInfo';
cXML_LoadControl clob;
vCount number;
vUnderloadBeforLMC number;
BEGIN
  -- ������ �室��� ��ࠬ���
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ������� ᯨ᮪ ��ࠬ��஢ ��� �᫮��� �롮ન �� �६����� ⠡���� �ᯨᠭ��
  select t1.ID_AC, t1.ID_WS, t1.ID_BORT
  into vID_AC, vID_WS, vID_BORT
  from WB_SHED t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

  -- ������ ���祭�� �� CALCS
  -- �᫨ � Calcs ���� ����� �� ⮯���� - ��६ ���㤠, ���� ��६ ����� �� 㬮�砭��
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cFuelInfo
    from
    (
      select t1.XML_VALUE
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
    ) t1
    where rownum <= 1;

    if cFuelInfo is not NULL then
    begin
      select to_number(extractValue(xmltype(cFuelInfo),'/root[1]/fuel_info[1]/@block_fuel'))
      into vBlockFuel
      from dual;

      select to_number(extractValue(xmltype(cFuelInfo),'/root[1]/fuel_info[1]/@taxi_fuel'))
      into vTaxiFuel
      from dual;

      select to_number(extractValue(xmltype(cFuelInfo),'/root[1]/fuel_info[1]/@trip_fuel'))
      into vTripFuel
      from dual;
    end;
    else
    -- ��६ ����� �� 㬮�砭��
    begin
      vBlockFuel := 0;
      vTaxiFuel := 0;
      vTripFuel := 0;
    end;
    end if;
  end;
  else
  -- ��६ ����� �� 㬮�砭��
  begin
    vBlockFuel := 0;
    vTaxiFuel := 0;
    vTripFuel := 0;
  end;
  end if;

  -- ���祭�� UNDERLOAD BEFORE LMC
  SP_WBA_GET_LOAD_CONTROL(cXML_in, cXML_LoadControl);

  select count(1)
  into vCount
  from table(XMLSequence(Extract(xmltype(cXML_LoadControl), '/root/wheit/list'))) b
  where (EXTRACTVALUE(value(b), '/list/@load') = 'Underload Befor LMC');

  if vCount > 0 then
  begin
    select to_number(EXTRACTVALUE(value(b), 'list/@Actual'))
    into vUnderloadBeforLMC
    from table(XMLSequence(Extract(xmltype(cXML_LoadControl), '/root/wheit/list'))) b
    where (EXTRACTVALUE(value(b), '/list/@load') = 'Underload Befor LMC') and rownum <= 1;

    if vUnderloadBeforLMC < 0 then
    begin
      vUnderloadBeforLMC := 0;
    end;
    end if;
  end;
  end if;

  select XMLAGG(
                  XMLELEMENT("fuel_info",
                              XMLATTRIBUTES(tt1.ID_AC "ID_AC", tt1.ID_WS "ID_WS", tt1.ProcName "ProcName", tt1.ProcType "ProcType", tt1.fuel_distrib "fuel_distrib",
                                            tt1.fuel_density "fuel_density", tt1.density "density", tt1.density_units "density_units", tt1.block_fuel "block_fuel", tt1.taxi_fuel "taxi_fuel", tt1.trip_fuel "trip_fuel",
                                            tt1.ballast_fuel "ballast_fuel", tt1.additional_fuel "additional_fuel", tt1.max_fuel "max_fuel", tt1.min_fuel "min_fuel", tt1.index_unit "index_unit",
                                            tt1.weight_units "weight_units")
                            )
                )
  INTO cXML_data
  from
  (
    select t1.ID_AC, t1.ID_WS, t1.PROC_NAME ProcName, case when t1.CH_STANDART = 1 then 'Standart' else 'Non-Standart' end ProcType,
      ' ' fuel_distrib, 'Non-Default' fuel_density, to_char(t1.DENSITY, 'FM990.999') density,
      t4.NAME_RUS_SMALL density_units, vBlockFuel block_fuel, vTaxiFuel taxi_fuel,
      vTripFuel trip_fuel, 0 ballast_fuel, vUnderloadBeforLMC additional_fuel, to_char(min(t2.WEIGHT), 'FM999990')  min_fuel, to_char(max(t2.WEIGHT), 'FM999990') max_fuel, -- 'FM99990.999' --
      to_char(max(t2.INDEX_UNIT), 'FM99990.000') index_unit,
      t5.NAME_RUS_SMALL weight_units, t1.CH_USE_BY_DEFAULT
    from WB_REF_WS_AIR_CFV_ADV t1 inner join WB_REF_WS_AIR_CFV_TBL t2 on t1.ID = t2.IDN and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- and t1.ID_BORT = t2.ID_BORT
                                  inner join WB_REF_WS_AIR_MEASUREMENT t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS -- and t1.ID_BORT = t3.ID_BORT
                                  inner join WB_REF_MEASUREMENT_DENSITY t4 on t3.DENSITY_ID_FUEL = t4.ID
                                  inner join WB_REF_MEASUREMENT_WEIGHT t5 on t3.WEIGHT_ID = t5.ID
                                  inner join (
                                                select ID_AC, ID_WS, max(tt1.CH_USE_BY_DEFAULT) CH_USE_BY_DEFAULT
                                                from WB_REF_WS_AIR_CFV_ADV tt1
                                                group by tt1.ID_AC, tt1.ID_WS
                                              ) t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t1.CH_USE_BY_DEFAULT = t6.CH_USE_BY_DEFAULT
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
    group by t1.ID_AC, t1.ID_WS, t1.PROC_NAME, t1.DENSITY, t4.NAME_RUS_SMALL, t5.NAME_RUS_SMALL, t1.CH_USE_BY_DEFAULT, (case when t1.CH_STANDART = 1 then 'Standart' else 'Non-Standart' end)
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_fuel_info" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WBA_GET_FUEL_INFO;
/
