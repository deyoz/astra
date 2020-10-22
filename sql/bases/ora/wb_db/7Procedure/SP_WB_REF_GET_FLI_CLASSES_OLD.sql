create or replace PROCEDURE SP_WB_REF_GET_FLI_CLASSES_OLD
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;
XMLStr VARCHAR(20000);

BEGIN
  cXML_out := '<?xml version="1.0" ?><root>';

  SELECT XMLAGG(
                  XMLELEMENT(
                              "weights",
                              XMLATTRIBUTES(tt1.Units "Units"),

                              (SELECT XMLAGG(
                                  XMLELEMENT( "class",
                                      XMLATTRIBUTES(
                                                      t2.CLASS_CODE as "code",
                                                      to_char(t1.Adult) AS "Adult",
                                                      -- to_char(t1.Male) AS "Male",
                                                      -- to_char(t1.Female) AS "Female",
                                                      to_char(decode(t1.adult, null, t1.male, null)) AS "Male",
                                                      to_char(decode(t1.adult, null, t1.female, null)) AS "Female",
                                                      to_char(t1.Child) AS "Child",
                                                      nvl(to_char(t1.Infant), ' ') as "Infant"
                                                    )
                                  ) ORDER BY t1.ID
                                  ) --AS fwd_list
                                 FROM WB_REF_AIRCO_PAX_WEIGHTS t1 inner join (
                                                                                select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) DATE_FROM
                                                                                from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                                                group by t1.ID_AC, t1.ID_CLASS
                                                                              ) t5 on t1.ID_AC = t5.ID_AC and t1.ID_CLASS = t5.ID_CLASS and t1.DATE_FROM = t5.DATE_FROM
                                                                  inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_AC = t2.ID_AC and t1.ID_CLASS = t2.ID
                                                                  inner join WB_REF_AIRCO_MEAS t3 on t2.ID_AC = t3.ID_AC
                                                                  inner join WB_REF_MEASUREMENT_WEIGHT t4 on t3.WEIGHT_ID = t4.ID
                                 WHERE t4.ID = tt1.ID
                              ) as cls
                            )
            )
  INTO cXML_data
  from
  (
    SELECT distinct t4.ID, t4.NAME_ENG_SMALL Units
    FROM WB_REF_AIRCO_PAX_WEIGHTS t1 inner join (
                                                  select t1.ID_AC, t1.ID_CLASS, max(t1.DATE_FROM) DATE_FROM
                                                  from WB_REF_AIRCO_PAX_WEIGHTS t1
                                                  group by t1.ID_AC, t1.ID_CLASS
                                                ) t5 on t1.ID_AC = t5.ID_AC and t1.ID_CLASS = t5.ID_CLASS and t1.DATE_FROM = t5.DATE_FROM
                                    inner join WB_REF_AIRCO_CLASS_CODES t2 on t1.ID_AC = t2.ID_AC and t1.ID_CLASS = t2.ID
                                    inner join WB_REF_AIRCO_MEAS t3 on t2.ID_AC = t3.ID_AC
                                    inner join WB_REF_MEASUREMENT_WEIGHT t4 on t3.WEIGHT_ID = t4.ID
  ) tt1;

  if cXML_data is not NULL then
  begin
    select replace(cXML_data, 'Infant=" "', 'Infant=""') into cXML_out from dual;
    -- cXML_out := '<?xml version="1.0" ?><root name="get_ahm_passenger_and_baggage_weights" result="ok">' || cXML_data.getClobVal() || '</root>';
    cXML_out := '<?xml version="1.0" ?><root name="get_ahm_passenger_and_baggage_weights" result="ok">' || cXML_out || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_passenger_and_baggage_weights" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_FLI_CLASSES_OLD;
/
