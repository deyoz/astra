create or replace PROCEDURE GET_FLIGHT_ENDS
(
  FLIGHT IN NUMBER, /* move_id */
  FIRST_DELTA OUT DATE,
  LAST_DELTA OUT DATE
) IS
  CURSOR ends is
    select scd_out, delta_out, scd_in, delta_in
    from routes
    where move_id = FLIGHT and
      num in ( 0,
        (select count(num) - 1 from ROUTES rt where move_id = FLIGHT)
      );
  delta_out NUMBER;
  delta_in NUMBER;
BEGIN

  open ends;
  fetch ends into FIRST_DELTA, delta_out, LAST_DELTA, delta_in;
  close ends;

  FIRST_DELTA := FIRST_DELTA + delta_out;
  LAST_DELTA := LAST_DELTA + delta_in;

END GET_FLIGHT_ENDS;
/
