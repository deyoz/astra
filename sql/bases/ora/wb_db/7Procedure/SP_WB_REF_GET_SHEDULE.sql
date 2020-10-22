create or replace PROCEDURE SP_WB_REF_GET_SHEDULE(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0"  encoding="utf-8"?>
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
END SP_WB_REF_GET_SHEDULE;
/
