create or replace FUNCTION getIfaceVersion(iface_id VARCHAR2)
return number
IS
  xml_stuff_type XML_STUFF.TYPE%TYPE;
  xml_stuff_id XML_STUFF.ID%TYPE;
  
  CURSOR cur IS SELECT LINK_NAME FROM IFACE_LINKS WHERE LINK_TYPE='ipart' AND IFACE=iface_id;
  curRow cur%ROWTYPE;
  CURSOR cur2(xml_stuff_id XML_STUFF.ID%TYPE) IS SELECT NVL(MAX(VERSION),-1) VER FROM XML_STUFF
    WHERE ID=xml_stuff_id AND TYPE='ipart' AND PAGE_N=1;
  cur2Row cur2%ROWTYPE;
  res_ver NUMBER;
BEGIN
  BEGIN
    SELECT NVL(MAX(VERSION),-1) INTO res_ver FROM XML_STUFF
      WHERE ID=iface_id AND TYPE='interface' AND PAGE_N=1;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RETURN -1;
  END;

  FOR curRow IN cur LOOP
    FOR cur2Row IN cur2(curRow.LINK_NAME) LOOP
      IF cur2Row.VER>res_ver THEN
        res_ver:=cur2Row.VER;
      END IF;
    END LOOP;
  END LOOP;
  RETURN res_ver;
END getIfaceVersion;
/
show errors
