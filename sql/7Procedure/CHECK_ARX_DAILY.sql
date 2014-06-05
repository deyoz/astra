create or replace PROCEDURE check_arx_daily(before IN BOOLEAN)
IS
  cur INTEGER;
  i INTEGER;
  n INTEGER;
  ignore    INTEGER;
  CURSOR curTab IS
    SELECT table_name FROM user_tables WHERE table_name LIKE 'ARX_%';
  row drop_check_arx_daily%ROWTYPE;
BEGIN
  IF before THEN
    DELETE FROM drop_check_arx_daily;
  END IF;
  FOR curTabRow IN curTab LOOP
    row.arx_table_name:=curTabRow.table_name;
    row.oper_table_name:=SUBSTR(curTabRow.table_name, 5);
    FOR i IN 1..2 LOOP
      cur:=DBMS_SQL.OPEN_CURSOR;
      IF i=1 THEN
        DBMS_SQL.PARSE(cur,
                       'SELECT COUNT(*) FROM '||row.oper_table_name,
                       DBMS_SQL.NATIVE);
      ELSE
        DBMS_SQL.PARSE(cur,
                       'SELECT COUNT(*) FROM '||row.arx_table_name,
                       DBMS_SQL.NATIVE);
      END IF;
      DBMS_SQL.DEFINE_COLUMN(cur,1,n);
      ignore:=DBMS_SQL.EXECUTE(cur);
      ignore:=DBMS_SQL.FETCH_ROWS(cur);
      DBMS_SQL.COLUMN_VALUE(cur,1,n);
      DBMS_SQL.CLOSE_CURSOR(cur);

      IF before THEN
        IF i=1 THEN
          row.oper_count_before:=n;
        ELSE
          row.arx_count_before:=n;
        END IF;
      ELSE
        IF i=1 THEN
          row.oper_count_after:=n;
        ELSE
          row.arx_count_after:=n;
        END IF;
      END IF;
    END LOOP;
    IF before THEN
      INSERT INTO drop_check_arx_daily(oper_table_name, arx_table_name,
                                       oper_count_before, arx_count_before)
      VALUES(row.oper_table_name, row.arx_table_name,
             row.oper_count_before, row.arx_count_before);
    ELSE
      UPDATE drop_check_arx_daily
      SET oper_count_after=row.oper_count_after,
          arx_count_after=row.arx_count_after
      WHERE oper_table_name=row.oper_table_name AND
            arx_table_name=row.arx_table_name;
    END IF;
    COMMIT;
  END LOOP;
EXCEPTION
  WHEN OTHERS THEN
    IF DBMS_SQL.IS_OPEN(cur) THEN
      DBMS_SQL.CLOSE_CURSOR(cur);
    END IF;
    RAISE;
END;
/
