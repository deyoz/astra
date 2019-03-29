create or replace trigger TRIP_COMP_ELEMS__AFTER__TRG
AFTER
INSERT OR UPDATE
ON TRIP_COMP_ELEMS
FOR EACH ROW
BEGIN
  IF salons.normalize_xname(:new.xname) IS NULL OR
     salons.normalize_yname(:new.yname) IS NULL THEN
    --если не нормализованные xname или yname, то ищем точное совпадение границ диапазонов
    INSERT INTO trip_comp_ranges(point_id,num,x,y,range_id,layer_type)
    SELECT :new.point_id,:new.num,:new.x,:new.y,range_id,layer_type
    FROM trip_comp_layers,
         crs_pnr,crs_pax,
         pax_grp,pax
    WHERE trip_comp_layers.point_id = :new.point_id AND
          first_xname = :new.xname AND
          last_xname = :new.xname AND
          first_yname = :new.yname AND
          last_yname = :new.yname AND
          trip_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND
          crs_pax.pnr_id=crs_pnr.pnr_id(+) AND
          trip_comp_layers.pax_id=pax.pax_id(+) AND
          pax.grp_id=pax_grp.grp_id(+) AND
          (:new.class IS NOT NULL AND
           (pax_grp.class IS NULL OR pax_grp.class=:new.class) AND
           (crs_pnr.class IS NULL OR crs_pnr.class=:new.class));
  ELSE
    --проверить, является ли нормализованным место xname+yname и диапазон
    --если да, то вставить строку
    INSERT INTO trip_comp_ranges(point_id,num,x,y,range_id,layer_type)
    SELECT :new.point_id,:new.num,:new.x,:new.y,range_id,layer_type
    FROM trip_comp_layers,
         crs_pnr,crs_pax,
         pax_grp,pax
    WHERE :new.point_id = trip_comp_layers.point_id AND
          :new.xname BETWEEN first_xname AND last_xname AND
          :new.yname BETWEEN first_yname AND last_yname AND
          salons.normalize_xname(first_xname) IS NOT NULL AND
          salons.normalize_xname(last_xname) IS NOT NULL AND
          salons.normalize_yname(first_yname) IS NOT NULL AND
          salons.normalize_yname(last_yname) IS NOT NULL AND
          trip_comp_layers.crs_pax_id=crs_pax.pax_id(+) AND
          crs_pax.pnr_id=crs_pnr.pnr_id(+) AND
          trip_comp_layers.pax_id=pax.pax_id(+) AND
          pax.grp_id=pax_grp.grp_id(+) AND
          (:new.class IS NOT NULL AND
           (pax_grp.class IS NULL OR pax_grp.class=:new.class) AND
           (crs_pnr.class IS NULL OR crs_pnr.class=:new.class));
  END IF;
END;
/
