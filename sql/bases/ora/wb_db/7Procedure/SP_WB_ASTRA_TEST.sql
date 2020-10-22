create or replace PROCEDURE SP_WB_ASTRA_TEST(cXML_in IN clob, cXML_out OUT clob)
AS
l_XML xmltype;
l_temp_XML      clob;
BEGIN
  DBMS_LOB.CREATETEMPORARY(l_temp_XML, false, 2); --2 makes the temporary only available in this call
  DBMS_LOB.COPY (l_temp_XML, cXML_in, dbms_lob.getlength(cXML_in),1,1);
  l_XML := xmltype.createXML(l_temp_XML);
 cXML_out := cXML_in;
END;
/
