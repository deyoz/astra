create or replace FUNCTION FLIGHT_START
(
  FIRST_DAY IN DATE
, DAYS IN VARCHAR2
, POINT IN DATE
) RETURN DATE AS
  PT DATE;
BEGIN

  pt := point - 1;

  WHILE pt >= first_day
  LOOP
    /* подходит ли дата по DAYS */
    IF INSTR(days, TO_CHAR(pt, 'D')) != 0 THEN
      RETURN pt;
    END IF;

    pt := pt - 1;
  END LOOP;

  RETURN TO_DATE('30.12.1899', 'DD.MM.YYYY');
END FLIGHT_START;
/
