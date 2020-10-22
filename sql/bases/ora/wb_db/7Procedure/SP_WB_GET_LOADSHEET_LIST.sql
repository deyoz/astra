create or replace PROCEDURE SP_WB_GET_LOADSHEET_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_ID number := -1;
P_NEW number := 1;
BEGIN
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id')) --, to_number(extractValue(xmltype(cXML_in),'/root[1]/@new'))
  into P_ID --, P_NEW
  from dual;

  -- Получит входной параметр
  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID "id",
                                            e.DT "nodename",
                                            e.ImageIndex "imageindex",
                                            e.SelectedIndex "selectedindex",
                                            e.parent_id "parent_id",
                                            e.Ext0 "ext0",
                                            e.Ext1 "ext1")
                            )
                            order by e.Ext2 desc
                )
  into cXML_data
  from (
          /*
          select '0' ID, 'New document' DT, 0 ImageIndex, 0 SelectedIndex, '' parent_id, '0' Ext0, '0' Ext1,
            to_char(P_ID) || ' ' || to_char(sysdate, 'YYYY.MM.DD') || ' ' || to_char(sysdate, 'HH24:MI') Ext2
          from dual
          where P_NEW = 1
          union
          */
          select to_char(t1.ELEM_ID) || ' ' || to_char(t1.DT, 'YYYY.MM.DD') || ' ' || to_char(t1.DT, 'HH24:MI:SS') ID, to_char(t1.DT, 'DD.MM.YYYY') || ' ' || to_char(t1.DT, 'HH24:MI:SS') DT,
            0 ImageIndex, 0 SelectedIndex, '' parent_id,
            to_char(t1.DT, 'DD.MM.YYYY') Ext0, to_char(t1.DT, 'HH24:MI:SS') Ext1,
            to_char(t1.ELEM_ID) || ' ' || to_char(t1.DT, 'YYYY.MM.DD') || ' ' || to_char(t1.DT, 'HH24:MI:SS') Ext2
          from WB_CALCS_XML t1
          where DATA_NAME = 'LoadSheet' and ELEM_ID = P_ID
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_loadsheet_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_GET_LOADSHEET_LIST;
/
