create or replace PROCEDURE xaxa(vpoint_id IN points.point_id%TYPE)
IS
  CURSOR cur IS
    SELECT counters2.point_dep,
           counters2.class,
           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(crs_tranzit)-SUM(crs_ok) AS avail,
           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(GREATEST(crs_tranzit, tranzit))-SUM(ok)-SUM(goshow) AS free_ok,
           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(GREATEST(crs_tranzit, tranzit))-SUM(GREATEST(crs_ok, ok))-SUM(goshow) AS free_goshow,
           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(tranzit)-SUM(ok)-SUM(goshow) AS nooccupy,
           SUM(NVL(jmp_tranzit,0))+SUM(NVL(jmp_ok,0))+SUM(NVL(jmp_goshow,0)) AS jmp
    FROM counters2, trip_classes
    WHERE counters2.point_dep=trip_classes.point_id(+) AND
          counters2.class=trip_classes.class(+) AND
          counters2.point_dep=vpoint_id
    GROUP BY counters2.point_dep, counters2.class;
jmp  NUMBER(6);
BEGIN
  jmp:=0;
  FOR curRow IN cur LOOP
    UPDATE counters2
    SET avail=curRow.avail,
        free_ok=curRow.free_ok,
        free_goshow=curRow.free_goshow,
        nooccupy=curRow.nooccupy
    WHERE point_dep=curRow.point_dep AND class=curRow.class;
    jmp:=jmp+curRow.jmp;
  END LOOP;
  UPDATE counters2 SET jmp_nooccupy=jmp WHERE point_dep=vpoint_id;
END xaxa;
/
