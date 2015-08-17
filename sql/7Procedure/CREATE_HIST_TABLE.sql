create or replace PROCEDURE create_hist_table(vtable_name history_tables.code%TYPE)
IS
  CURSOR cur IS
    SELECT * FROM history_tables WHERE code=LOWER(vtable_name) ORDER BY code;
str VARCHAR2(4000);
vnullable user_tab_columns.nullable%TYPE;
BEGIN
  FOR curRow IN cur LOOP
    DBMS_OUTPUT.PUT('hist_'||curRow.code||' processed...');
    str:='CREATE TABLE hist_'||curRow.code||
         ' AS SELECT '||curRow.ident_field||', '||curRow.history_fields||' FROM '||curRow.code||' WHERE rownum<1';
    EXECUTE IMMEDIATE str;
    EXECUTE IMMEDIATE 'ALTER TABLE hist_'||curRow.code||' ADD hist_time DATE NOT NULL';
    EXECUTE IMMEDIATE 'ALTER TABLE hist_'||curRow.code||' ADD hist_order NUMBER(9) NOT NULL';
--    SELECT nullable INTO vnullable FROM user_tab_columns
--    WHERE table_name=UPPER('hist_'||curRow.code) AND column_name=UPPER(curRow.ident_field);
--    IF vnullable='Y' THEN
--      EXECUTE IMMEDIATE 'ALTER TABLE hist_'||curRow.code||' MODIFY '||curRow.ident_field||' NOT NULL';
--    END IF;
    DBMS_OUTPUT.PUT_LINE('ok');
  END LOOP;
END;
/
