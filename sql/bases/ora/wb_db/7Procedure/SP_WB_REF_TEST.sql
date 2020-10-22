create or replace PROCEDURE SP_WB_REF_TEST
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;
XMLStr VARCHAR(20000);

BEGIN
  cXML_out := '<?xml version="1.0" ?><root>';

  SELECT XMLAGG(XMLELEMENT("weights", xmlattributes(
                                                 tt1.Units "Units", tt1.code "code", tt1.adult "Adult", tt1.male "Male", tt1.female "Female", tt1.child "Child", tt1.infant "Infant"
                                                )
                          )
                )
  INTO cXML_data
  from
  (
    SELECT t4.NAME_ENG_SMALL Units, t2.CLASS_CODE code, t1.adult, t1.child, t1.infant,
      decode(t1.adult, null, t1.male, null) male,
      decode(t1.adult, null, t1.female, null) female
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
    cXML_out := '<?xml version="1.0" ?><root>' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_REF_TEST;
/
