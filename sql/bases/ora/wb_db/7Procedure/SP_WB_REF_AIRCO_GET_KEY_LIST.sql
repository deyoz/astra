create or replace procedure SP_WB_REF_AIRCO_GET_KEY_LIST
(cXML_in in clob,
   cXML_out out clob)
as
lang_name varchar2(50):='';
cXML_output CLOB:='';
cXML_data CLOB:='';
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang_name from dual;

    cXML_output:=to_clob('<?xml version="1.0" ?><root>');

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.id "id",
                                                     e.airco_name "airco_name",
                                                       e.iata_code as "iata_code",
                                                         e.u_name "u_name",
                                                           e.date_write "date_write"))).getClobVal() into cXML_data
    from
    (select k.id,
             case when LANG_NAME='RUS' then i.NAME_RUS_FULL
                  else i.NAME_ENG_FULL
             end airco_name,
               case when i.IATA_CODE is null then 'NULL'
                    else i.IATA_CODE
               end IATA_CODE,
                 k.u_name,
                   to_char(k.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') date_write
    from WB_REF_AIRCOMPANY_KEY k join WB_REF_AIRCOMPANY_ADV_INFO i
    on k.id=i.id_ac and --k.id=-1 and
       i.date_from=nvl((select max(ii.date_from)
                        from WB_REF_AIRCOMPANY_ADV_INFO ii
                        where ii.id_ac=i.id_ac and
                              ii.date_from<=sysdate), (select min(ii.date_from)
                                                       from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                       where ii.id_ac=i.id_ac and
                                                             ii.date_from>sysdate))
    order by case when LANG_NAME='RUS' then i.NAME_RUS_FULL
                  else i.NAME_ENG_FULL
             end) e;


    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

    commit;
  end SP_WB_REF_AIRCO_GET_KEY_LIST;
/
