create or replace PROCEDURE SP_WB_AIRCO_GET_C_D_1_INFO(cXML_in IN clob,
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
    from WB_REF_AIRCO_C_DATA_1
    where id=rec_id;

    if (r_count)>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select  '<list DATE_FROM="'||to_char(i.DATE_from, 'dd.mm.yyyy')||'" '||
                      'H_ADRESS="'||WB_CLEAR_XML(i.H_ADRESS)||'" '||
                      'C_PERSONE="'||WB_CLEAR_XML(i.C_PERSONE)||'" '||
                      'E_MAIL_ADRESS="'||WB_CLEAR_XML(i.E_MAIL_ADRESS)||'" '||
                      'TELETYPE_ADRESS="'||WB_CLEAR_XML(i.TELETYPE_ADRESS)||'" '||
                      'PHONE_NUMBER="'||WB_CLEAR_XML(i.PHONE_NUMBER)||'" '||
                      'FAX_NUMBER="'||WB_CLEAR_XML(i.FAX_NUMBER)||'" '||
                      'REMARK="'||WB_CLEAR_XML(nvl(i.REMARK, 'NULL'))||'" '||
                      'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                      'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                      'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                      'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
               into cXML_data
           from WB_REF_AIRCO_C_DATA_1 i
           where i.id=rec_id;

        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_AIRCO_GET_C_D_1_INFO;
/
