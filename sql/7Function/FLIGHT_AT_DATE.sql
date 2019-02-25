create or replace FUNCTION FLIGHT_AT_DATE
(
  point DATE,
  days VARCHAR2,
  scdTIME DATE,
  DELTA NUMBER
) RETURN BOOLEAN AS
  offset NUMBER;
BEGIN
  offset := trunc(scdTIME) - to_date('30.12.1899 00:00', 'DD.MM.YYYY HH24:MI') - delta;

  return instr(days, to_char(point + offset, 'd')) != 0;
END FLIGHT_AT_DATE;
/
