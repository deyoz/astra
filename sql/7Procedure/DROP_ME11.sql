create or replace PROCEDURE drop_me11
IS
num BINARY_INTEGER;
BEGIN
  SELECT   COUNT(*) INTO num FROM pers_types WHERE name='ê?ê˙ë?ê?ë?ê?ë?ê?';
  DBMS_OUTPUT.PUT_LINE('ê ê?ê˙ë?ê?ë?ë?ê¯ë?: '||num);
END;
/
