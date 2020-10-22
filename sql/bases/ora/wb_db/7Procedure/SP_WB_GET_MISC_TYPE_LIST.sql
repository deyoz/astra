create or replace PROCEDURE SP_WB_GET_MISC_TYPE_LIST(cXML_in IN clob,
                                                      cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
cXML_data clob:='';
  BEGIN

    P_LANG:=cXML_in;

    cXML_out:='<?xml version="1.0" ?><root>';

    select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(qq.id) "id",
                                                     qq.MISC_NAME "MISC_NAME"))).getClobVal() into cXML_data
    from (select q.id,
                   q.misc_name
          from (select id,
                         case when P_LANG='RUS' then RUS_NAME else ENG_NAME end MISC_NAME
                from WB_REF_AIRCO_MISC_ITEMS) q
          order by q.MISC_NAME) qq;

    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_MISC_TYPE_LIST;
/
