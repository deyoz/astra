create or replace PROCEDURE SP_WB_REF_GET_AIRCO_LOGO(cXML_in IN varchar2,
                                                       cXML_out OUT CLOB)
AS
cXML_output clob:='';
cXML_data clob:='';
r_count number:=0;
p_id_ac number;
  BEGIN
    cXML_output:='<?xml version="1.0" ?><root>';

    p_id_ac:=to_number(cXML_in);

    select count(id) into r_count
    from WB_REF_AIRCOMPANY_LOGO
    where id_ac=p_id_ac;

    if (r_count)>0
      then select '<list LOGO="'||WB_CLEAR_XML(nvl(LOGO, 'NULL'))||'" '||
                        'LOGO_TYPE="'||WB_CLEAR_XML(LOGO_TYPE)||'" '||
                        'U_NAME="'||WB_CLEAR_XML(U_NAME)||'" '||
                        'U_IP="'||WB_CLEAR_XML(U_IP)||'" '||
                        'U_HOST_NAME="'||WB_CLEAR_XML(U_HOST_NAME)||'" '||
                        'DATE_WRITE="'||to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
               into cXML_data
           from WB_REF_AIRCOMPANY_LOGO
           where id_ac=p_id_ac;
    end if;

    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

  END SP_WB_REF_GET_AIRCO_LOGO;
/
