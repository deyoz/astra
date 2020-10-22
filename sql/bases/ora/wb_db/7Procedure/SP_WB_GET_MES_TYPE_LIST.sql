create or replace PROCEDURE SP_WB_GET_MES_TYPE_LIST(cXML_in IN clob,
                                                      cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
cXML_data clob:='';
  BEGIN

    P_LANG:=cXML_in;

    cXML_out:='<?xml version="1.0" ?><root>';

    if P_LANG='RUS' then
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.MES_NAME "MES_NAME"))).getClobVal() into cXML_data
        from (select id,
                       NAME_RUS_SMALL||'  '||NAME_RUS_FULL MES_NAME
              from WB_REF_MES_TYPE_LIST
              order by NAME_RUS_SMALL) e;
      end;
    else
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.MES_NAME "MES_NAME"))).getClobVal() into cXML_data
        from (select id,
                       NAME_ENG_SMALL||'  '||NAME_ENG_FULL MES_NAME
              from WB_REF_MES_TYPE_LIST
              order by NAME_ENG_SMALL) e;
      end;
    end if;

    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_MES_TYPE_LIST;
/
