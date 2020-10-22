create or replace PROCEDURE SP_WB_REF_GET_CONTROL(cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_load_control" elem_id="" result="ok">
  <pass name="get_load_control_pass" result="ok">
    <Class Code="F" Booked="1" CheckIn="1" Transit="0" />
    <Class Code="C" Booked="5" CheckIn="4" Transit="0" />
    <Class Code="Y" Booked="50" CheckIn="23" Transit="1" />
  </pass>
  <bagg name="get_load_control_bagg" result="ok">
    <Class Code="F" Booked="50" CheckIn="50" Transit="0" />
    <Class Code="C" Booked="150" CheckIn="120" Transit="0" />
    <Class Code="Y" Booked="1000" CheckIn="500" Transit="20" />
  </bagg>
  <wheit name="get_load_control_weight" result="ok">
    <list load="PAX" Estimated="5200" Actual="3250" Maximum="" />
    <list load="BAG" Estimated="1050" Actual="543" Maximum="" />
    <list load="Cargo" Estimated="250" Actual="143" Maximum="" />
    <list load="Mail" Estimated="70" Actual="57" Maximum="" />
    <list load="Transit Load" Estimated="0" Actual="0" Maximum="" />
    <list load="Total Traffic Load" Estimated="6570" Actual="3993" Maximum="" />
    <list load="Allowed Traffic Load" Estimated="8500" Actual="8500" Maximum="" />
    <list load="" Estimated="" Actual="" Maximum="" />
    <list load="Dry Operating Weight" Estimated="42500" Actual="42575" Maximum="" />
    <list load="Zero fuel Weight" Estimated="49070" Actual="46568" Maximum="61000" />
    <list load="Take off Fuel" Estimated="12000" Actual="12000" Maximum="" />
    <list load="Take off Weight" Estimated="61070" Actual="58568" Maximum="70000" />
    <list load="Trip Fuel" Estimated="8500" Actual="8500" Maximum="" />
    <list load="Landing Weight" Estimated="1050" Actual="543" Maximum="65000" />
    <list load="" Estimated="" Actual="" Maximum="" />
    <list load="Underload Befor LMC" Estimated="1930" Actual="4507" Maximum="" />
  </wheit>
  <seating name="get_load_control_seating" result="ok">
    <list Seat="0A" Estimated="5" Actual="1" Maximum="12" />
    <list Seat="0B" Estimated="15" Actual="3" Maximum="42" />
    <list Seat="0C" Estimated="20" Actual="7" Maximum="42" />
    <list Seat="0D" Estimated="15" Actual="12" Maximum="36" />
  </seating>
  <balance name="get_load_control_balance" result="ok">
    <Estimated DOI="45,1" LI_ZFW="45" LI_TOW="41" LI_LAW="44" ZFW_MAC="46568" TOW_MAC="58568" LAW_MAC="50068" />
    <Actual DOI="45,7" LI_ZFW="50" LI_TOW="56" LI_LAW="52" ZFW_MAC="47000" TOW_MAC="60000" LAW_MAC="51000" />
  </balance>
  <zfw name="get_load_control_zfw" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="13.45" Weight="61688" />
    <list index="76.98" Weight="61688" />
    <list index="61.18" Weight="39470" />
    <list index="55.73" Weight="35000" />
  </zfw>
  <tow name="get_load_control_tow" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="14.15" Weight="65543" />
    <list index="23.31" Weight="75000" />
    <list index="84.19" Weight="75000" />
    <list index="87.52" Weight="73055" />
    <list index="44.48" Weight="35000" />
  </tow>
  <law name="get_load_control_law" result="ok">
    <list index="30.08" Weight="35000" />
    <list index="13.19" Weight="60250" />
    <list index="14.11" Weight="65317" />
    <list index="37.04" Weight="66360" />
    <list index="84.24" Weight="66360" />
    <list index="65.5" Weight="40000" />
    <list index="59.43" Weight="35000" />
  </law>
</root>';
cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL;
/
