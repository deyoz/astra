create or replace procedure SP_WB_REF_WS_AIR_CFV_GET_LIST
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                     e.table_name "table_name"))).getClobVal() into cXML_data
    from (select a.id,
                   a.table_name
          from WB_REF_WS_AIR_CFV_IDN a
          where a.id_ac=P_ID_AC and
                a.id_ws=P_ID_WS and
                a.id_bort=P_ID_BORT
          order by a.table_name,
                   a.id) e
    order by e.table_name,
             e.id;

    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;


    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_WS_AIR_CFV_GET_LIST;
/
