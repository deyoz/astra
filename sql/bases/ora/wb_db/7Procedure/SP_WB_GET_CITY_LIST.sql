create or replace PROCEDURE SP_WB_GET_CITY_LIST(cXML_in IN clob,
                                                  cXML_out OUT CLOB)
AS
lang varchar2(50):='';
cXML_data clob:='';
  BEGIN

    lang:=cXML_in;

    cXML_out:='<?xml version="1.0" ?><root>';


    select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.sity_name "CITY_NAME"))).getClobVal() into cXML_data
        from (select id,
                     case when lang='RUS' then NAME_RUS_FULL
                          else NAME_ENG_FULL
                     end sity_name
              from wb_ref_cities
              order by NAME_RUS_FULL) e;


    /*
    if lang='RUS' then
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.NAME_RUS_FULL "CITY_NAME"))).getClobVal() into cXML_data
        from (select id,
                     NAME_RUS_FULL
              from wb_ref_cities
              order by NAME_RUS_FULL) e;
      end;
    else
      begin
        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                         e.NAME_ENG_FULL "CITY_NAME"))).getClobVal() into cXML_data
        from (select id,
                     NAME_ENG_FULL
              from wb_ref_cities
              order by NAME_ENG_FULL) e;
      end;
    end if;
    */
    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_CITY_LIST;
/
