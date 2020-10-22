create or replace PROCEDURE SP_WB_COUNTRY_CITY_PORT_TREE(cXML_in IN VARCHAR2,
                                                               cXML_out OUT CLOB)
AS
cXML_output CLOB:='';
cXML_data CLOB:='';
Lang_NAME varchar2(100);
  BEGIN
    cXML_output:=to_clob('<?xml version="1.0" ?><root>');

    LANG_NAME:=cXML_in;

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.country_id "country_id",
                                                     e.country_name "country_name",
                                                       e.city_id "city_id",
                                                         e.city_name "city_name",
                                                           e.port_id "port_id",
                                                             e.port_name "port_name"), xmlelement("flag", (select cc.flag
                                                                                                           from WB_REF_COUNTRY cc
                                                                                                           where cc.id=e.id)))).getClobVal() into cXML_data
    from (select distinct c.id,
                          to_char(c.id) country_id,
                          case when LANG_NAME='RUS' then c.name_rus_full
                               else c.name_eng_full
                          end country_name,

                          nvl(to_char(ct.id), 'NULL') city_id,

                          case when LANG_NAME='RUS' then nvl(ct.name_rus_full, 'NULL')
                               else nvl(ct.name_eng_full, 'NULL')
                          end city_name,
                          nvl(to_char(a.id), 'NULL') port_id,
                          case when LANG_NAME='RUS'
                               then nvl(a.name_rus_small, 'NULL')
                               else nvl(a.name_eng_small, 'NULL')
                          end port_name
          from WB_REF_COUNTRY c left outer join WB_REF_CITIES ct
          on c.id=ct.ID_COUNTRY left outer join WB_REF_AIRPORTS a
             on a.ID_CITY=ct.id
         order by country_name,
                  city_name,
                  port_name) e;

    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

  END SP_WB_COUNTRY_CITY_PORT_TREE;
/
