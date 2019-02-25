create or replace PROCEDURE GET_OFFSETS
(
  MOVE_NUM IN NUMBER,
  TAKEOFF IN DATE,
  TF_DELTA IN NUMBER,
  LAND IN DATE,
  LN_DELTA IN NUMBER,
  RES OUT DATE,
  RES_DELTA OUT NUMBER
) IS
BEGIN
  if mod(MOVE_NUM, 2) = 0 then
    RES := TAKEOFF;
    RES_DELTA := TF_DELTA;
  else
    RES := LAND;
    RES_DELTA := LN_DELTA;
  end if;

  if RES is null then
    RES := to_date('30.12.1899', 'DD.MM.YYYY');
  end if;

  if RES_DELTA is null then
    RES_DELTA := 0;
  end if;

END GET_OFFSETS;
/
