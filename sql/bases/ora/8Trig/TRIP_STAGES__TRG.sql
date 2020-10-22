create or replace trigger TRIP_STAGES__TRG
BEFORE
INSERT OR UPDATE
ON TRIP_STAGES
FOR EACH ROW
BEGIN
  IF :new.pr_auto=1 AND :new.act IS NULL AND :new.ignore_auto=0 THEN
    :new.time_auto_not_act:=NVL(:new.est,:new.scd);
  ELSE
    :new.time_auto_not_act:=NULL;
  END IF;
END;
/
