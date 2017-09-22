create or replace PROCEDURE events_by_agent(first_date IN DATE, last_date IN DATE, vuser IN events_bilingual.ev_user%TYPE)
IS
start_time DATE;
finish_time   DATE;
BEGIN
  IF first_date IS NULL OR last_date IS NULL OR first_date>=last_date THEN RETURN; END IF;
  start_time:=first_date;
  WHILE start_time<last_date LOOP
    finish_time:=start_time+1.0/24;
--    DBMS_OUTPUT.PUT_LINE('first_date='||TO_CHAR(first_date, 'DD.MM.YY HH24:MI')||
--                         ' last_date='||TO_CHAR(last_date, 'DD.MM.YY HH24:MI')||
--                         ' start_time='||TO_CHAR(start_time, 'DD.MM.YY HH24:MI'));
    INSERT INTO drop_events_by_agent
    SELECT * FROM events_bilingual
    WHERE time>=start_time AND time<finish_time AND lang='RU' AND ev_user=vuser;
    start_time:=finish_time;
  END LOOP;
  start_time:=first_date-10;
  WHILE start_time<last_date+10 LOOP
    finish_time:=start_time+1.0/24;
    INSERT INTO drop_events_by_agent(TYPE, TIME, EV_ORDER, MSG, SCREEN, EV_USER, STATION, ID1, ID2, ID3, LANG, PART_NUM)
    SELECT TYPE, TIME, EV_ORDER, MSG, SCREEN, EV_USER, STATION, ID1, ID2, ID3, LANG, PART_NUM FROM arx_events
    WHERE part_key>=start_time AND part_key<finish_time AND lang='RU' AND ev_user=vuser;
    start_time:=finish_time;
  END LOOP;
END events_by_agent;
/
