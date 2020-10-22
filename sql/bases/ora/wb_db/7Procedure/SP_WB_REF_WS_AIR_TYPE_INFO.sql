create or replace PROCEDURE SP_WB_REF_WS_AIR_TYPE_INFO(cXML_in IN clob,
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
    from WB_REF_WS_AIR_TYPE
    where id=P_ID;

    if R_COUNT>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select '<list DATE_FROM="'||to_char(i.DATE_FROM, 'dd.mm.yyyy')||'" '||
                     'REVIZION="'||WB_CLEAR_XML(i.REVIZION)||'" '||
                     'TRANSP_KATEG="'||WB_CLEAR_XML(tk.NAME)||'" '||
                     'TYPE_OF_LOADING="'||WB_CLEAR_XML(tl.NAME)||'" '||

                     'IS_UPPER_DECK="'||to_char(i.is_upper_deck)||'" '||
                     'IS_MAIN_DECK="'||to_char(i.is_main_deck)||'" '||
                     'IS_LOWER_DECK="'||to_char(i.is_lower_deck)||'" '||

                     'REMARK="'||WB_CLEAR_XML(nvl(i.REMARK, 'NULL'))||'" '||
                     'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                     'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                     'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                     'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           into cXML_data
           from WB_REF_WS_AIR_TYPE i join WB_REF_WS_TRANSP_KATEG tk
           on i.id=P_ID and
              i.ID_WS_TRANSP_KATEG=tk.id join WB_REF_WS_TYPE_OF_LOADING tl
              on i.ID_WS_TYPE_OF_LOADING=tl.id;

        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_REF_WS_AIR_TYPE_INFO;
/
