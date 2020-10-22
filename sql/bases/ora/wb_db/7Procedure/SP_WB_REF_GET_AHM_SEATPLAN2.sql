create or replace PROCEDURE SP_WB_REF_GET_AHM_SEATPLAN2 (cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.XML_VALUE
  INTO cXML_data
  from
  (
    select t1.XML_VALUE
    from WB_TMP_XML t1
    where t1.TABLENAME = 'AHMSeatPlan' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Актуальный план расположения сидений из AHM для данного
    элемента расписания.
-->
<root name="get_ahm_seatplan" result="ok">
  <seatplan Units="in">
    <section Name="0A">
        <row RowNo="2" LengthOfArm="-351.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="3" LengthOfArm="-317.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
    </section>
    <section Name="0B">
        <row RowNo="4" LengthOfArm="-283.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="5" LengthOfArm="-249.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="-" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="6" LengthOfArm="-215.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="7" LengthOfArm="-181.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="8" LengthOfArm="-147.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="9" LengthOfArm="-113.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="10" LengthOfArm="-79.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
    </section>
    <section Name="0C">
        <row RowNo="11" LengthOfArm="-41.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="12" LengthOfArm="-3.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="13" LengthOfArm="28.80">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="14" LengthOfArm="60.80">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="15" LengthOfArm="92.80">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="16" LengthOfArm="124.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="17" LengthOfArm="154.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
    </section>
    <section Name="0D">
        <row RowNo="18" LengthOfArm="184.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="19" LengthOfArm="214.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="20" LengthOfArm="244.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="21" LengthOfArm="274.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="22" LengthOfArm="304.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="23" LengthOfArm="334.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="24" LengthOfArm="364.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
        <row RowNo="25" LengthOfArm="394.20">
            <seat Ident="A" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="B" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="C" Codes="NA" AisleLeft="N" AisleRight="Y"/>
            <seat Ident="D" Codes="NA" AisleLeft="Y" AisleRight="N"/>
            <seat Ident="E" Codes="N" AisleLeft="N" AisleRight="N"/>
            <seat Ident="F" Codes="N" AisleLeft="N" AisleRight="N"/>
        </row>
    </section>
  </seatplan>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_AHM_SEATPLAN2;
/
