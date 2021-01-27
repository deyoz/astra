create or replace trigger COMP_ELEMS__BEFORE__TRG
BEFORE
INSERT OR UPDATE
ON COMP_ELEMS
FOR EACH ROW
BEGIN
  --нормализовать xname, yname
  :new.xname:=NVL(salons.normalize_xname(:new.xname),TRIM(UPPER(:new.xname)));
  :new.yname:=NVL(salons.normalize_yname(:new.yname),TRIM(UPPER(:new.yname)));
END;
/
