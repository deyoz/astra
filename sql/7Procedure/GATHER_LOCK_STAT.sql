create or replace PROCEDURE gather_lock_stat(start_time DATE, finish_time DATE, min_wait_msec NUMBER)
IS
  CURSOR cur IS
    SELECT * FROM points_lock_events ORDER BY point_id, lock_order;
prev cur%ROWTYPE;
locks_between NUMBER;
now_utc       DATE;
true_wait_msec NUMBER;
BEGIN
  now_utc:=SYSTEM.UTCSYSDATE;
  FOR curr IN cur LOOP
    IF prev.point_id=curr.point_id AND
       curr.lock_time>=start_time AND
       curr.lock_time<=finish_time AND
       curr.wait_msec>=min_wait_msec THEN
      SELECT COUNT(*)
      INTO locks_between
      FROM points_lock_events
      WHERE lock_order BETWEEN prev.lock_order AND curr.lock_order;
      true_wait_msec:=curr.lock_msec-prev.lock_msec;

      IF locks_between=curr.lock_order-prev.lock_order+1 AND
         curr.wait_msec+2>=true_wait_msec THEN
        UPDATE points_lock_stat
        SET wait_total=wait_total+true_wait_msec,
            lock_count=lock_count+1
        WHERE whence=prev.whence AND point_id=prev.point_id;
        IF SQL%NOTFOUND THEN
          INSERT INTO points_lock_stat(whence, point_id, wait_total, lock_count)
          VALUES(prev.whence, prev.point_id, true_wait_msec, 1);
        END IF;
        COMMIT;
      END IF;
    END IF;
    prev:=curr;
  END LOOP;
END;
/
