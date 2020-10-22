create or replace PROCEDURE SP_WB_GET_CITY_INFO(cXML_in IN varchar2,
                                               cXML_out OUT CLOB)
AS
cXML_output clob:='';
cXML_data clob:='';
r_count number:=0;
city_id number;
  BEGIN
    cXML_output:='<?xml version="1.0" ?><root>';

    city_id:=to_number(cXML_in);

    select count(id) into r_count
    from WB_REF_CITIES
    where id=city_id;

    if (r_count)>0
      then select '<list NAME_RUS_SMALL="'||WB_CLEAR_XML(NAME_RUS_SMALL)||'" '||
                        'NAME_RUS_FULL="'||WB_CLEAR_XML(NAME_RUS_FULL)||'" '||
                        'NAME_ENG_SMALL="'||WB_CLEAR_XML(NAME_ENG_SMALL)||'" '||
                        'NAME_ENG_FULL="'||WB_CLEAR_XML(NAME_ENG_FULL)||'" '||
                        'REMARK="'||WB_CLEAR_XML(nvl(REMARK, 'NULL'))||'" '||
                        'U_NAME="'||WB_CLEAR_XML(U_NAME)||'" '||
                        'U_IP="'||WB_CLEAR_XML(U_IP)||'" '||
                        'U_HOST_NAME="'||WB_CLEAR_XML(U_HOST_NAME)||'" '||
                        'DATE_WRITE="'||to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
               into cXML_data
           from WB_REF_CITIES
           where id=city_id;
    end if;

    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

  END SP_WB_GET_CITY_INFO;
/
