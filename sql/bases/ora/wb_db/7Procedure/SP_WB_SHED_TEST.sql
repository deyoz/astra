create or replace PROCEDURE SP_WB_SHED_TEST(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_Data XMLType;
P_ELEM_ID number := -1;
REC_COUNT number := 0;
TABLEVAR varchar(40) := 'AHMDOW';
P_ID_AC number := 0; -- авиакомпания
P_ID_WS number := 0; -- тип ВС
P_ID_BORT number := 0; -- борт
P_ID_SL number := 0; -- конфигурация
ULD_Weight number := 0;
ULD_Idx number := 0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

with tPax as
(
  select t1.ELEM_ID,
    EXTRACTVALUE(value(b), '/class/@code') code,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Adult'), 0)) Adult,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Male'), 0)) Male,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Female'), 0)) Female,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Child'), 0)) Child,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Infant'), 0)) Infant,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@CabinBaggage'), 0)) CabinBaggage,
    0 Tr
  from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/On/class'))) b
  where t1.DATA_NAME = 'PassengersDetails' -- t1.ELEM_ID = 3456 and
  group by t1.ELEM_ID, EXTRACTVALUE(value(b), '/class/@code')
  union
  select t1.ELEM_ID,
    EXTRACTVALUE(value(b), '/class/@code') code,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Adult'), 0)) Adult,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Male'), 0)) Male,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Female'), 0)) Female,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Child'), 0)) Child,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@Infant'), 0)) Infant,
    sum(nvl(EXTRACTVALUE(value(b), '/class/@CabinBaggage'), 0)) CabinBaggage,
    1 Tr
  from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/Tr/class'))) b
  where t1.DATA_NAME = 'PassengersDetails' -- t1.ELEM_ID = 3456 and
  group by t1.ELEM_ID, EXTRACTVALUE(value(b), '/class/@code')
)
    -- SWACodes
    select XMLAGG(
                    XMLELEMENT("elem",
                                XMLATTRIBUTES(
                                              t1.id,
                                              nvl(to_char(t1.S_DTL_1, 'HH24:MI'), ' ') "STD",
                                              nvl(to_char(t1.E_DTL_1, 'HH24:MI'), ' ') "ETD",
                                              t1.NR "Flight",
                                              t1.ID_AP_1 "DEP",
                                              t1.ID_WS "ID_WS",
                                              t3.NAME_ENG_SMALL "ACType",
                                              t1.ID_BORT, nvl(t4.BORT_NUM, ' ') "ACReg",
                                              ' ' "Gate",
                                              nvl((
                                                    select EXTRACTVALUE(value(b), '/CrewCode/@Name')
                                                    from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/CrewCode'))) b
                                                    where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
                                                  ), ' ') Crew,
                                              nvl((
                                                    select EXTRACTVALUE(value(b), '/CabinConfiguration/@Name')
                                                    from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/CabinConfiguration'))) b
                                                    where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
                                                  ), ' ') Config,
                                              nvl((
                                                    select EXTRACTVALUE(value(b), '/fuel_info/@block_fuel')
                                                    from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/fuel_info'))) b
                                                    where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'FuelInfo' and rownum <= 1
                                                  ), 0 ) BlockFuel,
                                              nvl((
                                                    select EXTRACTVALUE(value(b), '/fuel_info/@trip_fuel')
                                                    from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/fuel_info'))) b
                                                    where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'FuelInfo' and rownum <= 1
                                                  ), 0 ) TripFuel,
                                              0 AvBagPAX,
                                              ' ' FlightStatus,
                                              nvl((
                                                    select EXTRACTVALUE(value(b), '/Captain/@Name')
                                                    from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/Captain'))) b
                                                    where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
                                                  ), ' ') Captain
                                              ),
                                -- ARR
                                (
                                  select XMLAGG(
                                                  XMLELEMENT("ARR",
                                                              XMLATTRIBUTES(t1.ID_AP_2 id,
                                                                            nvl((select sum(tt1.Adult) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 0), 0) "CheckedPAXSex_A",
                                                                            nvl((select sum(tt1.Male) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 0), 0) "CheckedPAXSex_M",
                                                                            nvl((select sum(tt1.Female) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 0), 0) "CheckedPAXSex_F",
                                                                            nvl((select sum(tt1.Child) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 0), 0) "CheckedPAXSex_C",
                                                                            nvl((select sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 0), 0) "CheckedPAXSex_I",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'F' and tt1.Tr = 0), 0) "CheckedPAXClass_F",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'C' and tt1.Tr = 0), 0) "CheckedPAXClass_C",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'Y' and tt1.Tr = 0), 0) "CheckedPAXClass_Y",
                                                                            0 "BookedPAXClass_F",
                                                                            0 "BookedPAXClass_C",
                                                                            0 "BookedPAXClass_Y",
                                                                            0 "BookedPAXSex_M",
                                                                            0 "BookedPAXSex_F",
                                                                            0 "BookedPAXSex_C",
                                                                            0 "BookedPAXSex_I",
                                                                            nvl((select sum(tt1.Adult) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 1), 0) "TransitPAXSex_A",
                                                                            nvl((select sum(tt1.Male) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 1), 0) "TransitPAXSex_M",
                                                                            nvl((select sum(tt1.Female) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 1), 0) "TransitPAXSex_F",
                                                                            nvl((select sum(tt1.Child) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 1), 0) "TransitPAXSex_C",
                                                                            nvl((select sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.Tr = 1), 0) "TransitPAXSex_I",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'F' and tt1.Tr = 1), 0) "TransitPAXClass_F",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'C' and tt1.Tr = 1), 0) "TransitPAXClass_C",
                                                                            nvl((select sum(tt1.Adult) + sum(tt1.Male) + sum(tt1.Female) + sum(tt1.Child) + sum(tt1.Infant) from tPax tt1 where tt1.elem_id = t1.id and tt1.code = 'Y' and tt1.Tr = 1), 0) "TransitPAXClass_Y",
                                                                            nvl((select sum(tt1.CabinBaggage) from tPax tt1 where tt1.code = 'F' and tt1.Tr = 0), 0) "CheckedBagClass_F",
                                                                            nvl((select sum(tt1.CabinBaggage) from tPax tt1 where tt1.code = 'C' and tt1.Tr = 0), 0) "CheckedBagClass_C",
                                                                            nvl((select sum(tt1.CabinBaggage) from tPax tt1 where tt1.code = 'Y' and tt1.Tr = 0), 0) "CheckedBagClass_Y",
                                                                            0 "BookedCargo",
                                                                            0 "BookedMail",
                                                                            0 "LoadedCargo",
                                                                            0 "LoadedMail",
                                                                            'Y' "DG_SL"
                                                                            )
                                                            )
                                                )
                                  from dual
                                )
                              )
                  )
    into cXML_data
    from WB_SHED t1 inner join WB_REF_WS_AIR_TYPE t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS -- WB_SHED
                    inner join WB_REF_WS_TYPES t3 on t2.ID = t3.ID
                    left join WB_REF_AIRCO_WS_BORTS t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ID_BORT = t4.ID;

    cXML_out := cXML_Data.GetCLOBVal();

    if cXML_out is not null then
    begin
      cXML_out := '<root name="get_schedule" result="ok">' || cXML_out || '</root>';
    end;
    end if;
/*
select t1.id, nvl(to_char(t1.S_DTL_1, 'HH24:MI'), ' ') STD, nvl(to_char(t1.E_DTL_1, 'HH24:MI'), ' ') ETD, t1.NR Flight, t1.ID_AP_1 DEP, t1.ID_WS, t3.NAME_ENG_SMALL ACType, t1.ID_BORT, nvl(t4.BORT_NUM, ' ') ACReg,
  ' ' Gate,
  nvl((
        select EXTRACTVALUE(value(b), '/CrewCode/@Name')
        from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/CrewCode'))) b
        where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
      ), ' ') Crew,
  nvl((
        select EXTRACTVALUE(value(b), '/CabinConfiguration/@Name')
        from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/CabinConfiguration'))) b
        where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
      ), ' ') Config,
  nvl((
        select EXTRACTVALUE(value(b), '/Captain/@Name')
        from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/Captain'))) b
        where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'DOWData' and rownum <= 1
      ), ' ') Captain,
  nvl((
        select EXTRACTVALUE(value(b), '/fuel_info/@block_fuel')
        from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/fuel_info'))) b
        where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'FuelInfo' and rownum <= 1
      ), ' ' ) BlockFuel,
  nvl((
        select EXTRACTVALUE(value(b), '/fuel_info/@trip_fuel')
        from WB_CALCS_XML tt1, table(XMLSequence(Extract(xmltype(tt1.XML_VALUE), '/root/fuel_info'))) b
        where tt1.ELEM_ID = t1.ID and tt1.DATA_NAME = 'FuelInfo' and rownum <= 1
      ), ' ' ) TripFuel,
  0 AvBagPAX,
  ' ' FlightStatus,
  (select sum(t1.Adult) from tPax t1 where t1.Tr = 0) CheckedPAXSex_A,
  (select sum(t1.Male) from tPax t1 where t1.Tr = 0) CheckedPAXSex_M,
  (select sum(t1.Female) from tPax t1 where t1.Tr = 0) CheckedPAXSex_F,
  (select sum(t1.Child) from tPax t1 where t1.Tr = 0) CheckedPAXSex_C,
  (select sum(t1.Infant) from tPax t1 where t1.Tr = 0) CheckedPAXSex_I,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'F' and t1.Tr = 0) CheckedPAXClass_F,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'C' and t1.Tr = 0) CheckedPAXClass_C,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'Y' and t1.Tr = 0) CheckedPAXClass_Y,
  0 BookedPAXClass_F,
  0 BookedPAXClass_C,
  0 BookedPAXClass_Y,
  0 BookedPAXSex_M,
  0 BookedPAXSex_F,
  0 BookedPAXSex_C,
  0 BookedPAXSex_I,
  (select sum(t1.Adult) from tPax t1 where t1.Tr = 1) TransitPAXSex_A,
  (select sum(t1.Male) from tPax t1 where t1.Tr = 1) TransitPAXSex_M,
  (select sum(t1.Female) from tPax t1 where t1.Tr = 1) TransitPAXSex_F,
  (select sum(t1.Child) from tPax t1 where t1.Tr = 1) TransitPAXSex_C,
  (select sum(t1.Infant) from tPax t1 where t1.Tr = 1) TransitPAXSex_I,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'F' and t1.Tr = 1) TransitPAXClass_F,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'C' and t1.Tr = 1) TransitPAXClass_C,
  (select sum(t1.Adult) + sum(t1.Male) + sum(t1.Female) + sum(t1.Child) + sum(t1.Infant) from tPax t1 where t1.code = 'Y' and t1.Tr = 1) TransitPAXClass_Y,
  (select sum(t1.CabinBaggage) from tPax t1 where t1.code = 'F' and t1.Tr = 0) CheckedBagClass_F,
  (select sum(t1.CabinBaggage) from tPax t1 where t1.code = 'C' and t1.Tr = 0) CheckedBagClass_C,
  (select sum(t1.CabinBaggage) from tPax t1 where t1.code = 'Y' and t1.Tr = 0) CheckedBagClass_Y,
  0 BookedCargo,
  0 BookedMail,
  0 LoadedCargo,
  0 LoadedMail,
  'Y' DG_SL
from WB_SHED t1 inner join WB_REF_WS_AIR_TYPE t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                inner join WB_REF_WS_TYPES t3 on t2.ID = t3.ID
                left join WB_REF_AIRCO_WS_BORTS t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ID_BORT = t4.ID
where t1.ID = 538;
*/
END SP_WB_SHED_TEST;
/
