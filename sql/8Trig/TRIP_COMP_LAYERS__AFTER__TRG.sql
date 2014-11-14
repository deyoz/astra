create or replace trigger TRIP_COMP_LAYERS__AFTER__TRG
AFTER
INSERT OR UPDATE
ON TRIP_COMP_LAYERS
FOR EACH ROW
DECLARE
vcrs_class crs_pnr.class%TYPE;
vclass     pax_grp.class%TYPE;
BEGIN
  vclass:=NULL;
  vcrs_class:=NULL;
  --читаем информацию по классу пассажира pax_grp.class
  IF :new.crs_pax_id IS NOT NULL THEN
    SELECT crs_pnr.class
    INTO vcrs_class
    FROM crs_pnr,crs_pax
    WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=:new.crs_pax_id;
  END IF;
  --читаем информацию по классу пассажира crs_pnr.class
  IF :new.pax_id IS NOT NULL THEN
    SELECT pax_grp.class
    INTO vclass
    FROM pax_grp,pax
    WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=:new.pax_id;
  END IF;

  --поиск мест в салоне, кот. удовлетворяют диапазону
  --если диапазон - не одно место и названия линий и рядов в диапазоне невалидные то выйти из процедуры
  IF (:new.first_xname=:new.last_xname) AND
     (:new.first_yname=:new.last_yname) THEN
    INSERT INTO trip_comp_ranges(point_id,num,x,y,range_id,layer_type)
    SELECT :new.point_id,num,x,y,:new.range_id,:new.layer_type
    FROM trip_comp_elems
    WHERE point_id = :new.point_id AND
          xname = :new.first_xname AND
          yname = :new.first_yname AND
          (class IS NOT NULL AND
           (vclass IS NULL OR vclass=class) AND
           (vcrs_class IS NULL OR vcrs_class=class));
  ELSE
    --проверить, является ли валидным место xname+yname
    --если да, то вставить строку
    INSERT INTO trip_comp_ranges(point_id,num,x,y,range_id,layer_type)
    SELECT :new.point_id,num,x,y,:new.range_id,:new.layer_type
    FROM trip_comp_elems
    WHERE point_id = :new.point_id AND
          xname BETWEEN :new.first_xname AND :new.last_xname AND
          yname BETWEEN :new.first_yname AND :new.last_yname AND
          salons.normalize_xname(xname) IS NOT NULL AND
          salons.normalize_yname(yname) IS NOT NULL AND
          (class IS NOT NULL AND
           (vclass IS NULL OR vclass=class) AND
           (vcrs_class IS NULL OR vcrs_class=class));
  END IF;
END;
/
