create or replace trigger TRIP_COMP_LAYERS__BEFORE__TRG
BEFORE
INSERT OR UPDATE OR DELETE
ON TRIP_COMP_LAYERS
FOR EACH ROW
BEGIN
  IF UPDATING OR DELETING THEN
    DELETE FROM trip_comp_ranges WHERE range_id=:old.range_id AND point_id=:old.point_id;
  END IF;

  IF INSERTING OR UPDATING THEN
    --нормализовать first_xname, last_xname, first_yname, last_yname
    :new.first_xname:=NVL(salons.normalize_xname(:new.first_xname),TRIM(UPPER(:new.first_xname)));
    :new.last_xname:=NVL(salons.normalize_xname(:new.last_xname),TRIM(UPPER(:new.last_xname)));
    :new.first_yname:=NVL(salons.normalize_yname(:new.first_yname),TRIM(UPPER(:new.first_yname)));
    :new.last_yname:=NVL(salons.normalize_yname(:new.last_yname),TRIM(UPPER(:new.last_yname)));
  END IF;
END;
/
