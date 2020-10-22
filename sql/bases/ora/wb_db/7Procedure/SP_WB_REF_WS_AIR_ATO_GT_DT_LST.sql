create or replace procedure SP_WB_REF_WS_AIR_ATO_GT_DT_LST
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/id_ac[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/id_ws[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/id_bort[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.id "id",
                                                     e.date_from_ "date_from"))).getClobVal() into cXML_data
    from
    (select id,
            to_char(DATE_from, 'dd.mm.yyyy') date_from_
    from WB_REF_WS_AIR_ATO
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT
    order by date_from) e;

    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  end SP_WB_REF_WS_AIR_ATO_GT_DT_LST;
/
