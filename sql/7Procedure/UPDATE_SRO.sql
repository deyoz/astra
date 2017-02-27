create or replace PROCEDURE UPDATE_SRO
(
  POINT IN DATE
)AS
  cursor c1 is
    select trip_id, sd.num sdnum, sd.move_id, rt.num rtnum, first_day
    from routes rt, sched_days sd
    where rt.move_id = sd.move_id and
      rt.airp = 'СРО'
      /* дата завершения     время вылета       с учетом перехода суток                  дельта      */
    and (trunc(sd.last_day) + (rt.scd_out - to_timestamp('30.12.1899', 'DD.MM.YYYY')) + rt.delta_out) > point;
BEGIN

  FOR i in c1
  LOOP
    if i.first_day < point then
      BREAK_FLIGHT(i.trip_id, i.sdnum, i.move_id, i.rtnum, point);
    else
      UPDATE_FLIGHT(i.move_id, i.rtnum, point);
    end if;
  END LOOP;
END UPDATE_SRO;
/
