create or replace trigger POINTS__TIME_IN_OUT__TRG
BEFORE
INSERT OR UPDATE
ON POINTS
FOR EACH ROW
BEGIN
  :new.time_in:=TRUNC(NVL(:new.act_in,NVL(:new.est_in,:new.scd_in)));
  :new.time_out:=TRUNC(NVL(:new.act_out,NVL(:new.est_out,:new.scd_out)));
  IF :new.time_in IS NULL AND :new.time_out IS NULL THEN
    :new.time_in:=TO_DATE('01.01.0001','DD.MM.YYYY');
    :new.time_out:=TO_DATE('01.01.0001','DD.MM.YYYY');
  END IF;
END;
/
