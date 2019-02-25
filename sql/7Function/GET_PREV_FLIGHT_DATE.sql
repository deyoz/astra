create or replace function GET_PREV_FLIGHT_DATE
(
  point IN DATE,
  days in VARCHAR2
) RETURN DATE AS
  offset NUMBER;
BEGIN
  offset := 1;

  while offset <= 7 AND instr(days, to_char(point - offset, 'D')) = 0
  LOOP
    offset := offset + 1;
  end LOOP;

  RETURN point - offset;
END GET_PREV_FLIGHT_DATE;
/
