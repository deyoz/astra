create or replace procedure BREAK_FLIGHT
(
  trip NUMBER,
  sdnum NUMBER,
  move NUMBER,
  rtnum NUMBER,
  point DATE
) AS
  newid NUMBER;
  NEXT_START DATE;
  PREV_END DATE;
  hour NUMBER;
BEGIN
  GET_FLIGHT_BREAK(move, rtnum, point, NEXT_START, PREV_END);
  SELECT routes_move_id.nextval into newid FROM dual;

  hour := to_date('13:00', 'HH24:MI') - to_date('12:00', 'HH24:MI');

  if rtnum = 0 then
      insert into routes select newid, NUM, AIRP, AIRP_FMT, AIRLINE, AIRLINE_FMT, FLT_NO, SUFFIX, SUFFIX_FMT,
            CRAFT, CRAFT_FMT, SCD_IN - hour, DELTA_IN, SCD_OUT - hour, DELTA_OUT, TRIP_TYPE, LITERA, PR_DEL, F, C, Y, UNITRIP
            from routes WHERE move_id = move;

      insert into sched_days select trip_id, (select max(num) + 1 from sched_days where trip_id = trip), newid, NEXT_START, last_day, days, PR_DEL, TLG, REFERENCE, region
        from sched_days
        where trip_id = trip and
              num = sdnum and
              move_id = move;

      update sched_days
      set last_day = PREV_END
      where trip_id = trip and
            num = sdnum and
            move_id = move;
  end if;
END BREAK_FLIGHT;
/
