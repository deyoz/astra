create or replace FUNCTION FLIGHT_END
(
  LAST_DAY IN DATE
, DAYS IN VARCHAR2
, DELTA IN NUMBER
, POINT IN DATE
) RETURN DATE AS

  pt DATE;
BEGIN

  pt := point - delta - 1;

  WHILE pt >= first_day
  LOOP
    /* ���室�� �� ��� �� DAYS */
    IF INSTR(days, TO_CHAR(pt, 'D')) != 0 THEN
      RETURN pt;
    END IF;

    pt := pt - 1;
  END LOOP;

  RETURN TO_DATE('30.12.1899', 'DD.MM.YYYY');
END FLIGHT_END;
/
