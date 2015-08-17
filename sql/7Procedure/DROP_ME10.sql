create or replace PROCEDURE drop_me10
IS
num BINARY_INTEGER;
BEGIN
  SELECT COUNT(*) INTO num FROM pers_types WHERE name='‚§ΰ®α«λ¥';
  DBMS_OUTPUT.PUT_LINE('¥§γ«μβ β: '||num);
END;
/
