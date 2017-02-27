create or replace procedure UPDATE_FLIGHT
(
  move NUMBER,
  rtnum NUMBER,
  point DATE
) AS
  hour NUMBER;
BEGIN
  hour := to_date('13:00', 'HH24:MI') - to_date('12:00', 'HH24:MI');

  if rtnum = 0 then
      update routes
      set scd_out = scd_out - hour, scd_in = scd_in - hour
      WHERE move_id = move;
  end if;
END UPDATE_FLIGHT;
/
