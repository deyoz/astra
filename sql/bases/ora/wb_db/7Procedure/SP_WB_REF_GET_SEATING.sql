create or replace PROCEDURE SP_WB_REF_GET_SEATING (cXML_in IN clob,
                                           cXML_out OUT CLOB)
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
    where t1.TABLENAME = 'SeatingDetails' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
 /*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_seating_details" elem_id="3457" result="ok">
  <section Name="0A">
    <row RowNo="2">
      <seat Ident="A" ClassCode="C" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="C" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="C" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="C" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="3">
      <seat Ident="A" ClassCode="C" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="C" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="C" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="C" OccupiedBy="AVAILABLE"/>
    </row>
  </section>
  <section Name="0B">
    <row RowNo="4">
      <seat Ident="A" ClassCode="C" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="C" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="C" OccupiedBy="PAX_CHILD"/>
      <seat Ident="F" ClassCode="C" OccupiedBy="PAX_CHILD"/>
    </row>
    <row RowNo="5">
      <seat Ident="A" ClassCode="C" OccupiedBy="PAX_CHILD"/>
      <seat Ident="C" ClassCode="C" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="C" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="C" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="6">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_MALE"/>
    </row>
    <row RowNo="7">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="8">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="9">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="10">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
  </section>
  <section Name="0C">
    <row RowNo="11">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_MALE"/>
    </row>
    <row RowNo="12">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_MALE"/>
    </row>
    <row RowNo="13">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="14">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="15">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
    </row>
    <row RowNo="16">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_CHILD"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="17">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
  </section>
  <section Name="0D">
    <row RowNo="18">
      <seat Ident="A" ClassCode="Y" OccupiedBy="BLOCKED"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="PAX_ADULT"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="PAX_ADULT"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="PAX_MALE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
    </row>
    <row RowNo="19">
      <seat Ident="A" ClassCode="Y" OccupiedBy="PAX_FEMALE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="SOC"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="XCR"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="20">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="21">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="22">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="23">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="24">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
    <row RowNo="25">
      <seat Ident="A" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="B" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="C" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="D" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="E" ClassCode="Y" OccupiedBy="AVAILABLE"/>
      <seat Ident="F" ClassCode="Y" OccupiedBy="AVAILABLE"/>
    </row>
  </section>
</root>';
*/

cXML_out:=cXML_data;
END SP_WB_REF_GET_SEATING;
/
