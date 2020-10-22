create or replace procedure SP_WB_REF_WS_GET_KEY_LIST_AC
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_ID_AC number;
cXML_output CLOB:='';
cXML_data CLOB:='';
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;

    cXML_output:=to_clob('<?xml version="1.0" ?><root>');

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.id "id",
                                                     e.ws_name "ws_name",
                                                       e.dop_ident "dop_ident"))).getClobVal() into cXML_data
    from (select ws.id,
                 case when P_LANG='RUS' then ws.NAME_RUS_SMALL
                      else ws.NAME_ENG_SMALL
                 end ws_name,
                 ws.DOP_IDENT
          from WB_REF_WS_TYPES ws
          where not exists(select 1
                           from WB_REF_AIRCO_WS_TYPES ac
                           where ac.id_ac=P_ID_AC and
                                 ac.id_ws=ws.id)
          order by case when P_LANG='RUS' then NAME_RUS_SMALL
                        else NAME_ENG_SMALL
                   end,
                   ws.DOP_IDENT) e;

    cXML_output:=cXML_output||cXML_data||'</root>';

    if cXML_output='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
      else cXML_out:=cXML_output;
    end if;

    commit;
  end SP_WB_REF_WS_GET_KEY_LIST_AC;
/
