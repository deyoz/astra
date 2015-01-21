create or replace PROCEDURE check_index_tablespace(alter_tablespace BOOLEAN,
                                 min_num_rows user_indexes.num_rows%TYPE,
                                 max_num_rows user_indexes.num_rows%TYPE,
                                 dest user_indexes.tablespace_name%TYPE DEFAULT NULL,
                                 src user_indexes.tablespace_name%TYPE DEFAULT NULL
                                )
IS
  CURSOR cur(pts_dest user_indexes.tablespace_name%TYPE,
             pts_src user_indexes.tablespace_name%TYPE) IS
    SELECT index_name, num_rows
    FROM user_indexes
    WHERE tablespace_name=pts_src AND tablespace_name<>pts_dest AND
          num_rows BETWEEN min_num_rows AND max_num_rows
    ORDER BY num_rows;

  CURSOR cur2 IS
    SELECT tablespace_name FROM user_indexes GROUP BY tablespace_name ORDER BY count(*) DESC, tablespace_name;

ts_dest   user_indexes.tablespace_name%TYPE;
ts_src    user_indexes.tablespace_name%TYPE;
ignore    INTEGER;
alter_cur INTEGER;
BEGIN
  IF min_num_rows IS NULL OR max_num_rows IS NULL THEN
    raise_application_error(-20000,'check_index_tablespace: min_num_rows or max_num_rows not defined');
  END IF;
  IF min_num_rows>max_num_rows THEN
    raise_application_error(-20000,'check_index_tablespace: min_num_rows>max_num_rows');
  END IF;

  IF dest IS NULL THEN
    OPEN cur2;
    FETCH cur2 INTO ts_dest;
    IF cur2%NOTFOUND THEN
      CLOSE cur2;
      raise_application_error(-20000,'check_index_tablespace: destination tablespace name not defined');
    END IF;
    CLOSE cur2;
  ELSE
    ts_dest:=dest;
  END IF;

  IF src IS NULL THEN
    SELECT default_tablespace INTO ts_src FROM user_users;
  ELSE
    ts_src:=src;
  END IF;

  IF alter_tablespace THEN
    FOR curRow IN cur(ts_dest, ts_src) LOOP
    BEGIN
      alter_cur:=DBMS_SQL.OPEN_CURSOR;
      DBMS_SQL.PARSE(alter_cur,
                     'ALTER INDEX '||curRow.index_name||' REBUILD TABLESPACE '||ts_dest,
                     DBMS_SQL.NATIVE);
      ignore:=DBMS_SQL.EXECUTE(alter_cur);
      DBMS_SQL.CLOSE_CURSOR(alter_cur);
    EXCEPTION
      WHEN OTHERS THEN
        DBMS_SQL.CLOSE_CURSOR(alter_cur);
        RAISE;
    END;
    END LOOP;
  ELSE
    FOR curRow IN cur(ts_dest, ts_src) LOOP
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.index_name||': ',30,' ')||curRow.num_rows||' rows');
    END LOOP;
  END IF;
END;
/
