create or replace function GET_FLIGHT_DATE
(
  point IN DATE,
  time IN DATE,
  delta IN NUMBER,
  days IN VARCHAR2
) RETURN DATE
AS
  pt DATE;
  found boolean;
BEGIN
  found := false;
  pt := point;
  /* Цикл по pt */
  while (found != true)
  LOOP
    pt := pt + 1;
    found := FLIGHT_AT_DATE(pt, days, time, delta);
  end LOOP;

  RETURN to_date('30.12.1899', 'DD.MM.YYYY') + delta + (pt - trunc(time));
END GET_FLIGHT_DATE;
/
