create or replace trigger TRIP_COMP_ELEMS__BEFORE__TRG
BEFORE
INSERT OR UPDATE OR DELETE
ON TRIP_COMP_ELEMS
FOR EACH ROW
BEGIN
  IF UPDATING OR DELETING THEN
    DELETE FROM trip_comp_ranges
    WHERE point_id=:old.point_id AND num=:old.num AND x=:old.x AND y=:old.y;
  END IF;

  IF INSERTING OR UPDATING THEN
    --нормализовать xname, yname
    :new.xname:=NVL(salons.normalize_xname(:new.xname),TRIM(UPPER(:new.xname)));
    :new.yname:=NVL(salons.normalize_yname(:new.yname),TRIM(UPPER(:new.yname)));
  END IF;
END;
/
