create or replace PROCEDURE SP_WB_AIRCO_GET_C_D_DTM_DATES(cXML_in IN clob,
                                                           cXML_out OUT CLOB)
AS
P_ID_AC number:=-1;
cXML_data clob:='';
begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.date_from_ "date_from",
                                                     e.date_from_d "date_from_d",
                                                       e.date_from_m "date_from_m",
                                                         e.date_from_y "date_from_y"))).getClobVal() into cXML_data
    from
    (select distinct to_char(DATE_FROM, 'dd.mm.yyyy') date_from_,
                     date_from,
                     to_char(EXTRACT(DAY FROM DATE_FROM)) date_from_d,
                     to_char(EXTRACT(MONTH FROM DATE_FROM)) date_from_m,
                     to_char(EXTRACT(YEAR FROM DATE_FROM)) date_from_y
    from WB_REF_AIRCO_C_DATA_DTM
    where id_ac=P_ID_AC
    order by date_from) e;

    if cXML_data  is not null
      then cXML_out:=cXML_out||cXML_data;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

    commit;
  END SP_WB_AIRCO_GET_C_D_DTM_DATES;
/
