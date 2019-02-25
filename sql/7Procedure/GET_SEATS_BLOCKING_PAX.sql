create or replace PROCEDURE get_seats_blocking_pax(vstart_pax_id crs_pax.pax_id%TYPE DEFAULT NULL) IS
curr_pax_id crs_pax.pax_id%TYPE;
max_pax_id crs_pax.pax_id%TYPE;
BEGIN
  IF vstart_pax_id IS NOT NULL THEN
    DELETE FROM drop_seats_blocking_progress;
    INSERT INTO drop_seats_blocking_progress(start_pax_id) VALUES(vstart_pax_id);
  END IF;

  SELECT start_pax_id INTO curr_pax_id FROM drop_seats_blocking_progress;

  SELECT MAX(pax_id) INTO max_pax_id FROM crs_pax;
  WHILE curr_pax_id<=max_pax_id LOOP
    INSERT INTO drop_seats_blocking_pax(pax_id, sender, rem_code, point_id)
    SELECT crs_pax_rem.pax_id, crs_pnr.sender, crs_pax_rem.rem_code, crs_pnr.point_id
    FROM crs_pax_rem, crs_pax, crs_pnr
    WHERE crs_pax_rem.pax_id=crs_pax.pax_id AND crs_pax.pnr_id=crs_pnr.pnr_id AND
          crs_pax_rem.pax_id>=curr_pax_id AND crs_pax_rem.pax_id<curr_pax_id+50000 AND
          crs_pax_rem.rem_code IN ('EXST','STCR','CBBG');
    UPDATE drop_seats_blocking_progress SET start_pax_id=curr_pax_id+50000;
    COMMIT;
    curr_pax_id:=curr_pax_id+50000;
  END LOOP;
END;
/
