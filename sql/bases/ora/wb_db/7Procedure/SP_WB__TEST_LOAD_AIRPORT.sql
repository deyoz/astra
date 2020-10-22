create or replace PROCEDURE SP_WB__TEST_LOAD_AIRPORT(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0"  encoding="utf-8"?>
<root name="get_schedule" result="ok">
  <elem
    id="3455"
    STD="hh:mm"
    ETD="hh:01"
    Flight="FV123"
    DEP="34"
    ACType="ATR-72-200"
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

    <ARR id="26"
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
    STD="hh:mm"
    ETD="hh:02"
    Flight="FV123"
    DEP="34"
    ACType="ATR-72-200"
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

    <ARR id="26"
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

    <ARR id="27"
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

    <ARR id="34"
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
    DEP="34"
    ACType="Boeing-737-400"
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

    <ARR id="27"
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

    <ARR id="26"
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
    DEP="34"
    ACType="Boeing-737-500"
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

    <ARR id="26"
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
    DEP="34"
    ACType="Boeing-777-200"
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

    <ARR id="26"
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
    DEP="34"
    ACType="Boeing-767-200"
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

    <ARR id="26"
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
END SP_WB__TEST_LOAD_AIRPORT;
/
