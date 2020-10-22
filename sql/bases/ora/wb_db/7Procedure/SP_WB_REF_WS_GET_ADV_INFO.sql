create or replace PROCEDURE SP_WB_REF_WS_GET_ADV_INFO(cXML_in IN clob,
                                                        cXML_out OUT CLOB)
AS
P_ID number:=-1;
P_LANG varchar2(50):='';
cXML_data clob:='';
R_COUNT number:=0;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID;

    if (R_COUNT)>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select  '<list IATA="'||WB_CLEAR_XML(IATA)||'" '||
                      'ICAO="'||WB_CLEAR_XML(ICAO)||'" '||
                      'DOP_IDENT="'||WB_CLEAR_XML(DOP_IDENT)||'" '||
                      'NAME_RUS_SMALL="'||WB_CLEAR_XML(NAME_RUS_SMALL)||'" '||
                      'NAME_RUS_FULL="'||WB_CLEAR_XML(NAME_RUS_FULL)||'" '||
                      'NAME_ENG_SMALL="'||WB_CLEAR_XML(NAME_ENG_SMALL)||'" '||
                      'NAME_ENG_FULL="'||WB_CLEAR_XML(NAME_ENG_FULL)||'" '||
                      'REMARK="'||WB_CLEAR_XML(nvl(REMARK, 'NULL'))||'" '||
                      'U_NAME="'||WB_CLEAR_XML(U_NAME)||'" '||
                      'U_IP="'||WB_CLEAR_XML(U_IP)||'" '||
                      'U_HOST_NAME="'||WB_CLEAR_XML(U_HOST_NAME)||'" '||
                      'DATE_WRITE="'||to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
               into cXML_data
           from WB_REF_WS_TYPES
           where id=P_ID;


        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_REF_WS_GET_ADV_INFO;
/
