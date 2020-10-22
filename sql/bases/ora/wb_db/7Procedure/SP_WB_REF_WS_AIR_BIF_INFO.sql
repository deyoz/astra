create or replace PROCEDURE SP_WB_REF_WS_AIR_BIF_INFO(cXML_in IN clob,
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
    from WB_REF_WS_AIR_BAS_IND_FORM
    where id=P_ID;

    if R_COUNT>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select '<list DATE_FROM="'||to_char(i.DATE_FROM, 'dd.mm.yyyy')||'" '||
                     'REF_ARM="'||to_char(i.REF_ARM)||'" '||
                     'K_CONST="'||to_char(i.K_CONST)||'" '||
                     'C_CONST="'||to_char(i.C_CONST)||'" '||
                     'LEN_MAC_RC="'||to_char(i.LEN_MAC_RC)||'" '||
                     'LEMAC_LERC="'||to_char(i.LEMAC_LERC)||'" '||
                     'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                     'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                     'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                     'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           into cXML_data
           from WB_REF_WS_AIR_BAS_IND_FORM i
           where i.id=P_ID;

        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_REF_WS_AIR_BIF_INFO;
/
