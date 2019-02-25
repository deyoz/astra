create or replace PROCEDURE GET_FLIGHT_BREAK
(
  MOVE_SET IN NUMBER, /* routes.move_id */
  MOVE_NUM IN NUMBER, /* routes.num */
  point IN DATE,
  NEXT_START OUT DATE,
  PREV_END OUT DATE
) IS
  CURSOR ends is
    select rt.num, first_day, last_day, scd_out, delta_out, scd_in, delta_in, days
    from routes rt, sched_days sd
    where
      rt.move_id = sd.move_id and
      rt.move_id = MOVE_SET and
      rt.num in ( 0, MOVE_NUM/*,
        (select count(num) - 1 from ROUTES where move_id = MOVE_SET)*/
      );
      /* � १-� ����� ���� ���� 3 ���� 2 (�᫨ move_num = (0 ��� last)) ����� */
  pt DATE;
  fd NUMBER;
  ld DATE;
  TIME_ DATE;
  delta NUMBER;
  days VARCHAR2(7);
BEGIN
  pt := point;

  /*open ends;*/

  for i in ends
  LOOP
    if i.num = 0 then
      fd := GET_WALLCLOCK_TIME(i.scd_out); /* ⮫쪮 �६� + base */
      days := i.days;
    end if;

    if i.num = MOVE_NUM then
      GET_OFFSETS(i.num, i.scd_out, i.delta_out, i.scd_in, i.delta_in, time_, delta);
      pt := GET_FLIGHT_DATE(pt, time_, delta, days);
    end if;
  END LOOP;


  NEXT_START := pt + fd;
  PREV_END := GET_PREV_FLIGHT_DATE(pt + fd, days);
END GET_FLIGHT_BREAK;
/
