create or replace PROCEDURE SP_WB_AIRCO_GET_NAMES(cXML_in IN VARCHAR2,
                                                    cXML_out OUT CLOB)
AS
cXML_data CLOB:='';
P_AIRCO_ID number:=-1;
  BEGIN
    cXML_out:=to_clob('<?xml version="1.0" ?><root>');

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;

     SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.NAME_RUS_full "TITLE_RUS",
                                                      e.NAME_ENG_full "TITLE_ENG"))).getClobVal() into cXML_data
     from (select i.NAME_RUS_full,
                    i.NAME_ENG_full
           from WB_REF_AIRCOMPANY_ADV_INFO i
           where i.id_ac=P_AIRCO_ID and
                 i.date_from=nvl((select max(ii.date_from)
                                  from WB_REF_AIRCOMPANY_ADV_INFO ii
                                  where ii.id_ac=i.id_ac and
                                        ii.date_from<=sysdate()), (select min(ii.date_from)
                                                                   from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                   where ii.id_ac=i.id_ac and
                                                                         ii.date_from>sysdate()))) e;


    if cXML_data is not null then
      begin
        cXML_out:=cXML_out||cXML_data;

      end;
    end if;

    cXML_out:=cXML_out||'</root>';

    commit;
  END SP_WB_AIRCO_GET_NAMES;
/
