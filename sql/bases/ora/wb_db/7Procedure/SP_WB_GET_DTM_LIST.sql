create or replace PROCEDURE SP_WB_GET_DTM_LIST(cXML_in IN clob,
                                                  cXML_out OUT CLOB)
AS
lang varchar2(50):='';
cXML_data clob:='';
  BEGIN

    lang:=cXML_in;

    cXML_out:='<?xml version="1.0" ?><root>';

    if lang='RUS' then
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.NAME_RUS "DTM_NAME"))).getClobVal() into cXML_data
        from (select id,
                     NAME_RUS
              from WB_REF_DATA_TRANSFER_MODES
              order by NAME_RUS) e;
      end;
    else
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.NAME_ENG "DTM_NAME"))).getClobVal() into cXML_data
        from (select id,
                     NAME_ENG
              from WB_REF_DATA_TRANSFER_MODES
              order by NAME_ENG) e;
      end;
    end if;

    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_DTM_LIST;
/
