create or replace PROCEDURE drop_me11
IS
num BINARY_INTEGER;
BEGIN
  SELECT   COUNT(*) INTO num FROM pers_types WHERE name='�?���?�?�?�?�?�?';
  DBMS_OUTPUT.PUT_LINE('� �?���?�?�?�?���?: '||num);
END;
/
