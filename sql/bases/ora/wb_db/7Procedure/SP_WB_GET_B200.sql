create or replace PROCEDURE SP_WB_GET_B200 (cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_ahm_seatplan" result="ok">
  <seatplan Units="ft">
    <section Name="0A">
      <row RowNo="1" LengthOfArm="423.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="2" LengthOfArm="454.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="3" LengthOfArm="484.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="4" LengthOfArm="515.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="5" LengthOfArm="545.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="6" LengthOfArm="576.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
    </section>
    <section Name="0B">
      <row RowNo="7" LengthOfArm="606.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="8" LengthOfArm="637.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="9" LengthOfArm="667.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="10" LengthOfArm="698.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="11" LengthOfArm="728.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="12" LengthOfArm="759.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="13" LengthOfArm="789.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
    </section>
    <section Name="0C">
      <row RowNo="16" LengthOfArm="898.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="-" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="-" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="-" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="-" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="17" LengthOfArm="922.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="18" LengthOfArm="953.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="19" LengthOfArm="984.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="20" LengthOfArm="1014.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="21" LengthOfArm="1046.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="22" LengthOfArm="1077.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="23" LengthOfArm="1108.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="24" LengthOfArm="1139.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="25" LengthOfArm="1170.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="26" LengthOfArm="1201.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="27" LengthOfArm="1232.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="28" LengthOfArm="1263.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="29" LengthOfArm="1294.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="30" LengthOfArm="1325.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="31" LengthOfArm="1356.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="32" LengthOfArm="1387.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="33" LengthOfArm="1418.6">
        <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
      <row RowNo="34" LengthOfArm="1451.6">
        <seat Ident="A" Codes="-" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="B" Codes="-" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="D" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="E" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="F" Codes="-" AisleLeft="N" AisleRight="N"></seat>
        <seat Ident="G" Codes="N" AisleLeft="N" AisleRight="Y"></seat>
        <seat Ident="K" Codes="N" AisleLeft="Y" AisleRight="N"></seat>
        <seat Ident="L" Codes="N" AisleLeft="N" AisleRight="N"></seat>
      </row>
    </section>
  </seatplan>
</root>';

cXML_out:=cXML_data;
END SP_WB_GET_B200;
/
