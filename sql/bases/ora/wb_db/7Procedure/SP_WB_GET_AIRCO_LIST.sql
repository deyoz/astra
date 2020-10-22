create or replace PROCEDURE SP_WB_GET_AIRCO_LIST(cXML_in IN CLOB,
                                                    cXML_out OUT CLOB)
AS
cXML_data XMLType;

BEGIN
  select XMLAGG(
                  XMLELEMENT("rec",
                              XMLATTRIBUTES(e.ID_AC "id",
                                            e.NAME_RUS_full "name",
                                            e.NAME_ENG_full "name_eng")
                            )
                )
  into cXML_data
  from (
          select i.ID_AC, i.NAME_RUS_full, i.NAME_ENG_full
          from WB_REF_AIRCOMPANY_ADV_INFO i
          where i.date_from = nvl(
                                    (
                                      select max(ii.date_from)
                                      from WB_REF_AIRCOMPANY_ADV_INFO ii
                                      where ii.id_ac = i.id_ac and ii.date_from <= sysdate
                                    ),
                                    (
                                      select min(ii.date_from)
                                      from WB_REF_AIRCOMPANY_ADV_INFO ii
                                      where ii.id_ac = i.id_ac and ii.date_from > sysdate
                                    )
                                  )
        ) e;

  if cXML_data is not NULL then
  begin
    cXML_out := '<root name="get_airco_list" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  --commit;
END SP_WB_GET_AIRCO_LIST;
/
