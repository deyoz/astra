create or replace PROCEDURE SP_WB_GET_RESERVED_PHRASE(cXML_in IN clob,
                                                        cXML_out OUT CLOB)
AS
cXML_data clob:='';
  BEGIN

    cXML_out:='<?xml version="1.0" ?><root>';

    select XMLAGG(XMLELEMENT("list", xmlattributes(e.PHRASE "phrase"))).getClobVal() into cXML_data
        from (select PHRASE
              from WB_REF_RESERVED_PHRASE
              order by PHRASE) e;

    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_RESERVED_PHRASE;
/
