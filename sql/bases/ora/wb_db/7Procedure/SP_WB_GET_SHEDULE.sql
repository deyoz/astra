create or replace PROCEDURE SP_WB_GET_SHEDULE(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
/*
with tPax as
(
select EXTRACTVALUE(value(b), '/class/@code') code,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Adult'), 0)) Adult,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Male'), 0)) Male,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Female'), 0)) Female,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Child'), 0)) Child,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Infant'), 0)) Infant,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@CabinBaggage'), 0)) CabinBaggage,
  0 Tr
from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/On/class'))) b
where t1.ELEM_ID = 3456 and t1.DATA_NAME = 'PassengersDetails'
group by EXTRACTVALUE(value(b), '/class/@code')
union
select EXTRACTVALUE(value(b), '/class/@code') code,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Adult'), 0)) Adult,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Male'), 0)) Male,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Female'), 0)) Female,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Child'), 0)) Child,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@Infant'), 0)) Infant,
  sum(nvl(EXTRACTVALUE(value(b), '/class/@CabinBaggage'), 0)) CabinBaggage,
  1 Tr
from WB_CALCS_XML t1, table(XMLSequence(Extract(xmltype(t1.XML_VALUE), '/root/ARR/Tr/class'))) b
where t1.ELEM_ID = 3456 and t1.DATA_NAME = 'PassengersDetails'
group by EXTRACTVALUE(value(b), '/class/@code')
)
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
      (select sum(t1.CabinBaggage) from tPax t1 where t1.code = 'Y' and t1.Tr = 0) CheckedBagClass_Y
from WB_SHED t1 inner join WB_REF_WS_AIR_TYPE t2 on t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                inner join WB_REF_WS_TYPES t3 on t2.ID = t3.ID
                left join WB_REF_AIRCO_WS_BORTS t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ID_BORT = t4.ID
where t1.ID = 538;
*/

cXML_data := '<?xml version="1.0"  encoding="utf-8"?>
<root name="get_schedule" result="ok">
  <elem
    id="3455"
    STD="07:00"
    ETD="08:20"
    Flight="FV123"
    DEP="4"
    ACType="B737-800"
    ACReg="VP-BYW"
    Gate="5a"
    Config="F2C4Y62"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Петров"
  >

    <ARR id="1"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />

  </elem>

  <elem
    id="3456"
    STD="17:00"
    ETD="18:05"
    Flight="FV123"
    DEP="4"
    ACType="B767-200"
    ACReg="VP-BYW"
    Gate="5a"
    Config="F2C4Y62"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Петров"
  >

    <ARR id="1"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      CabinBaggage="135"
      CabinBaggageTransit="50"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />

    <ARR id="3"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      CabinBaggage="100"
      CabinBaggageTransit="20"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3457"
    STD="07:05"
    ETD="02.08.2016 08:00"
    Flight="FV124"
    DEP="4"
    ACType="B767-200"
    ACReg="VP-BYW"
    Gate="5a"
    Config="16C120Y"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Иванов"
  >

    <ARR id="1"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="N"
    />

    <ARR id="21"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3458"
    STD="07:05"
    ETD="08:05"
    Flight="FV124"
    DEP="4"
    ACType="B737-800"
    ACReg="VP-BYW"
    Gate="5a"
    Config="F2C4Y62"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Иванов"
  >

    <ARR id="3"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3459"
    STD="07:05"
    ETD="08:10"
    Flight="FV124"
    DEP="4"
    ACType="B737-800"
    ACReg="VP-BYW"
    Gate="5a"
    Config="14J350Y"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Иванов"
  >

    <ARR id="1"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3460"
    STD="07:10"
    ETD="08:10"
    Flight="FV125"
    DEP="4"
    ACType="B737-800"
    ACReg="VP-BYW"
    Gate="5a"
    Config="14J350Y"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="В.Иванов"
  >

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>


  <elem
    id="3461"
    STD="08:20"
    ETD="09:30"
    Flight="FV135"
    DEP="4"
    ACType="737-500"
    ACReg="VP-BYW"
    Gate="5a"
    Config="Y116"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="Е.Петров"
  >

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>


  <elem
    id="3462"
    STD="09:40"
    ETD="11:00"
    Flight="FV137"
    DEP="4"
    ACType="737-500"
    ACReg="VP-BYW"
    Gate="5a"
    Config="C8Y108"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="Е.Петров"
  >

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3463"
    STD="08:30"
    ETD="10:10"
    Flight="FV139"
    DEP="4"
    ACType="320-200"
    ACReg="VP-BYW"
    Gate="5a"
    Config="C8Y108"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="Е.Иванов"
  >

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3464"
    STD="08:30"
    ETD="10:10"
    Flight="FV141"
    DEP="1"
    ACType="777-200"
    ACReg="VP-BYW"
    Gate="5a"
    Config="C14Y350"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="Е.Иванов"
  >

    <ARR id="2"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
    <ARR id="3"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

  <elem
    id="3465"
    STD="07:30"
    ETD="10:10"
    Flight="FV141"
    DEP="4"
    ACType="777-200"
    ACReg="VP-BYW"
    Gate="5a"
    Config="Y364"
    Crew="2/5/3"
    BlockFuel="13400"
    TripFuel="6000"
    AvBagPAX="25"
    FlightStatus=""
    Captain="Е.Иванов"
  >

    <ARR id="1"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
    <ARR id="3"
      BookedPAXClass_F="1"
      BookedPAXClass_C="2"
      BookedPAXClass_Y="15"

      BookedPAXSex_M="10"
      BookedPAXSex_F="5"
      BookedPAXSex_C="2"
      BookedPAXSex_I="1"

      CheckedPAXClass_F=""
      CheckedPAXClass_C="0"
      CheckedPAXClass_Y="5"

      CheckedPAXSex_M="0"
      CheckedPAXSex_F="3"
      CheckedPAXSex_C="2"
      CheckedPAXSex_I="1"

      TransitPAXClass_F="1"
      TransitPAXClass_C="1"
      TransitPAXClass_Y="6"

      TransitPAXSex_M="2"
      TransitPAXSex_F="2"
      TransitPAXSex_C="0"
      TransitPAXSex_I="1"

      CheckedBagClass_F="100"
      CheckedBagClass_C="100"
      CheckedBagClass_Y="600"

      BookedCargo="1500"
      BookedMail="300"
      LoadedCargo="800"
      LoadedMail="200"
      DG_SL="Y"
    />
  </elem>

</root>';

cXML_out:=cXML_data;
END SP_WB_GET_SHEDULE;
/
