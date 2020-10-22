create or replace PROCEDURE SP_WB_AIRCO_GET_ADV_INFO(cXML_in IN clob,
                                                       cXML_out OUT CLOB)
AS
rec_id number:=-1;
lang varchar2(50):='';
cXML_data clob:='';
r_count number:=0;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/id[1]')) into rec_ID from dual;

    select count(id) into r_count
    from WB_REF_AIRCOMPANY_ADV_INFO
    where id=rec_id;

    if (r_count)>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select  '<list IATA_CODE="'||WB_CLEAR_XML(i.IATA_CODE)||'" '||
                      'ICAO_CODE="'||WB_CLEAR_XML(i.ICAO_CODE)||'" '||
                      'OTHER_CODE="'||WB_CLEAR_XML(i.OTHER_CODE)||'" '||
                      'DATE_FROM="'||to_char(i.DATE_from, 'dd.mm.yyyy')||'" '||
                      'ID_CITY="'||to_char(i.id_city)||'" '||

                      'CITY_NAME="'||case when lang='RUS'
                                          then WB_CLEAR_XML(nvl(ct.NAME_RUS_FULL, '-'))
                                          else WB_CLEAR_XML(nvl(ct.NAME_ENG_FULL, '-'))
                                     end||'" '||

                      'COUNTRY_NAME="'||case when lang='RUS'
                                             then WB_CLEAR_XML(nvl(cn.NAME_RUS_FULL, 'NULL'))
                                             else WB_CLEAR_XML(nvl(cn.NAME_ENG_FULL, 'NULL'))
                                        end||'" '||

                      'NAME_RUS_SMALL="'||WB_CLEAR_XML(i.NAME_RUS_SMALL)||'" '||
                      'NAME_RUS_FULL="'||WB_CLEAR_XML(i.NAME_RUS_FULL)||'" '||
                      'NAME_ENG_SMALL="'||WB_CLEAR_XML(i.NAME_ENG_SMALL)||'" '||
                      'NAME_ENG_FULL="'||WB_CLEAR_XML(i.NAME_ENG_FULL)||'" '||
                      'REMARK="'||WB_CLEAR_XML(nvl(i.REMARK, 'NULL'))||'" '||
                      'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                      'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                      'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                      'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
               into cXML_data
           from WB_REF_AIRCOMPANY_ADV_INFO i left join WB_REF_CITIES ct
           on i.id=rec_id and
              i.id_city=ct.ID left join WB_REF_COUNTRY cn
              on cn.id=ct.ID_COUNTRY
           where i.id=rec_id;


        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_AIRCO_GET_ADV_INFO;
/
