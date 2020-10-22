create or replace procedure SP_WB_REF_WS_GET_KEY_LIST
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
                                                     e.ws_name "ws_name",
                                                       e.dop_ident "dop_ident",
                                                         e.u_name "u_name",
                                                           e.u_ip,
                                                             e.u_host_name,
                                                               e.date_write "date_write"))).getClobVal() into cXML_data
    from (select id,
                 case when LANG_NAME='RUS' then NAME_RUS_SMALL
                      else NAME_ENG_SMALL
                 end ws_name,
                 DOP_IDENT,
                 u_name,
                 u_ip,
                 u_host_name,
                 to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') date_write
          from WB_REF_WS_TYPES
          order by case when LANG_NAME='RUS' then NAME_RUS_SMALL||'...['||NAME_RUS_FULL||']'
                        else NAME_ENG_SMALL||'...['||NAME_ENG_FULL||']'
                   end) e;

    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

    commit;
  end SP_WB_REF_WS_GET_KEY_LIST;
/
