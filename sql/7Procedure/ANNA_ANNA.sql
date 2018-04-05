create or replace PROCEDURE anna_anna
IS
  CURSOR cur(vpart_key DATE) IS
    SELECT part_key, time, ev_order FROM arx_events
    WHERE part_key>vpart_key-1/1440 AND part_key<=vpart_key AND
          type IN ('„ˆ‘', '…‰', 'ƒ”', '‡„—', '€‘', 'Ž‹', '’‹ƒ', '—’');
vmax_part_key DATE;
vmin_part_key DATE;
vnum_rows NUMBER;
TYPE TRowidsTable IS TABLE OF ROWID;
rowids        TRowidsTable;
i             BINARY_INTEGER;
BEGIN
  vmin_part_key:=TO_DATE('01.06.16', 'DD.MM.YY');
  BEGIN
    SELECT max_part_key INTO vmax_part_key FROM drop_arx_events_progress;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      SELECT MAX(part_key) INTO vmax_part_key FROM arx_events;
  END;
  WHILE vmax_part_key>=vmin_part_key LOOP
    vnum_rows:=0;
    FOR curRow IN cur(vmax_part_key) LOOP
      SELECT rowid BULK COLLECT INTO rowids
      FROM events_bilingual
      WHERE time=curRow.time AND ev_order=curRow.ev_order FOR UPDATE;

      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_events
          (type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,part_key,part_num,lang)
        SELECT
           type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,curRow.part_key,part_num,lang
        FROM events_bilingual
        WHERE rowid=rowids(i);

      FORALL i IN 1..rowids.COUNT
        DELETE FROM events_bilingual WHERE rowid=rowids(i);

      vnum_rows:=vnum_rows+SQL%ROWCOUNT;
    END LOOP;
    ROLLBACK;
    vmax_part_key:=vmax_part_key-1/1440;
    UPDATE drop_arx_events_progress SET max_part_key=vmax_part_key, num_rows=num_rows+vnum_rows;
    IF SQL%NOTFOUND THEN
      INSERT INTO drop_arx_events_progress(max_part_key, num_rows) VALUES(vmax_part_key, vnum_rows);
    END IF;
    COMMIT;
  END LOOP;
END;

/
