create or replace PROCEDURE check_latin_columns
IS
  CURSOR cur IS
    SELECT table_name, column_name
    FROM user_tab_columns
    WHERE column_name LIKE '%_LAT' AND nullable<>'N' AND
          table_name||'.'||column_name NOT IN ('AGENCIES.DESCR_LAT',
                                               'AGENCIES.NAME_LAT',
                                               'AIRLINES.CODE_ICAO_LAT',
                                               'AIRLINES.CODE_LAT',
                                               'AIRLINES.NAME_LAT',
                                               'AIRLINES.SHORT_NAME_LAT',
                                               'AIRPS.CODE_ICAO_LAT',
                                               'AIRPS.CODE_LAT',
                                               'AIRPS.NAME_LAT',
                                               'BP_PRINT.SEAT_NO_LAT',
                                               'CITIES.CODE_LAT',
                                               'CITIES.NAME_LAT',
                                               'COUNTRIES.CODE_LAT',
                                               'COUNTRIES.NAME_LAT',
                                               'CRAFTS.CODE_ICAO_LAT',
                                               'CRAFTS.CODE_LAT',
                                               'CRAFTS.NAME_LAT',
                                               'DESK_GRP.DESCR_LAT',
                                               'HALLS2.NAME_LAT',
                                               'PAX_CATS.NAME_LAT',
                                               'PRN_TEST_TAGS.VALUE_LAT',
                                               'REM_TYPES.NAME_LAT',
                                               'SALE_POINTS.DESCR_LAT',
                                               'TERM_MODES.NAME_LAT',
                                               'TRIP_LITERS.NAME_LAT',
                                               'TYPEB_SENDERS.NAME_LAT',
                                               'VALIDATOR_TYPES.NAME_LAT');
i       INTEGER;
n       INTEGER;
ignore  INTEGER;
BEGIN
  FOR curRow IN cur LOOP
    i:=DBMS_SQL.OPEN_CURSOR;
    DBMS_SQL.PARSE(i,
                   'SELECT COUNT(*) FROM '||curRow.table_name||' WHERE '||curRow.column_name||' IS NULL AND rownum<2',
                   DBMS_SQL.NATIVE);
    DBMS_SQL.DEFINE_COLUMN(i,1,n);
    ignore:=DBMS_SQL.EXECUTE(i);
    ignore:=DBMS_SQL.FETCH_ROWS(i);
    DBMS_SQL.COLUMN_VALUE(i,1,n);
    DBMS_SQL.CLOSE_CURSOR(i);
    IF n>0 THEN
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name||'.'||curRow.column_name||':',33,' ')||'nulls found');
    END IF;
  END LOOP;
END;
/
