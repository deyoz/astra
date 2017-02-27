create or replace PACKAGE BODY utils
AS
PROCEDURE other_constraints(curRow oper_cur%ROWTYPE, venable BOOLEAN)
IS
sql_text VARCHAR2(4000);
BEGIN
  FOR cRow IN other_cons_cur(curRow) LOOP
    IF venable THEN
      sql_text:='ALTER TABLE '||cRow.table_name||' ENABLE CONSTRAINT '||cRow.constraint_name;
    ELSE
      sql_text:='ALTER TABLE '||cRow.table_name||' DISABLE CONSTRAINT '||cRow.constraint_name;
    END IF;
    EXECUTE IMMEDIATE sql_text;
    DBMS_OUTPUT.PUT_LINE(sql_text);
  END LOOP;
END other_constraints;

PROCEDURE print_tab_num_rows(curRow oper_cur%ROWTYPE)
IS

BEGIN
  DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(curRow.num_rows,12)||' '||curRow.partition_name);
  FOR cRow IN other_cons_cur(curRow) LOOP
    DBMS_OUTPUT.PUT_LINE(cRow.constraint_name||' -> '||cRow.r_constraint_name||'!!!');
  END LOOP;
END print_tab_num_rows;

PROCEDURE airline_tab_num_rows
IS
BEGIN
  DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');
  FOR curRow IN oper_cur LOOP
    print_tab_num_rows(curRow);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
  DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');
  FOR curRow IN hist_cur LOOP
    print_tab_num_rows(curRow);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
  DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
  FOR curRow IN arx_cur LOOP
    print_tab_num_rows(curRow);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
END airline_tab_num_rows;

PROCEDURE print_count(curRow oper_cur%ROWTYPE,
                      airline_code airlines.code%TYPE,
                      max_num_rows user_tables.num_rows%TYPE)
IS
cur INTEGER;
n INTEGER;
ignore INTEGER;
sql_text VARCHAR2(4000);
BEGIN
  IF curRow.partition_name IS NOT NULL THEN
    sql_text:='SELECT COUNT(*) FROM '||curRow.table_name||' PARTITION ('||curRow.partition_name||') p WHERE p.'||curRow.column_name||'='''||airline_code||'''';
  ELSE
    sql_text:='SELECT COUNT(*) FROM '||curRow.table_name||' WHERE '||curRow.column_name||'='''||airline_code||'''';
  END IF;
  IF curRow.num_rows IS NULL OR curRow.num_rows<=max_num_rows THEN
    cur:=DBMS_SQL.OPEN_CURSOR;
    DBMS_SQL.PARSE(cur, sql_text, DBMS_SQL.NATIVE);
    DBMS_SQL.DEFINE_COLUMN(cur,1,n);
    ignore:=DBMS_SQL.EXECUTE(cur);
    ignore:=DBMS_SQL.FETCH_ROWS(cur);
    DBMS_SQL.COLUMN_VALUE(cur,1,n);
    DBMS_SQL.CLOSE_CURSOR(cur);
    IF n>0 THEN
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(n,12)||' '||curRow.partition_name);
    END IF;
  ELSE
    DBMS_OUTPUT.PUT_LINE(sql_text);
  END IF;
END;

PROCEDURE update_code(curRow oper_cur%ROWTYPE,
                      old_airline_code airlines.code%TYPE,
                      new_airline_code airlines.code%TYPE,
                      max_rows INTEGER,
                      upd_priority NUMBER)
IS
cur INTEGER;
n INTEGER;
rows_processed INTEGER;
sql_text VARCHAR2(4000);
BEGIN
  IF curRow.partition_name IS NOT NULL THEN
    sql_text:='UPDATE '||curRow.table_name||' PARTITION ('||curRow.partition_name||') p '||
              'SET p.'||curRow.column_name||'='''||new_airline_code||''' '||
              'WHERE p.'||curRow.column_name||'='''||old_airline_code||''' AND rownum<='||max_rows;
  ELSE
    sql_text:='UPDATE '||curRow.table_name||' '||
              'SET '||curRow.column_name||'='''||new_airline_code||''' '||
              'WHERE '||curRow.column_name||'='''||old_airline_code||''' AND rownum<='||max_rows;
  END IF;
  n:=0;
  cur:=DBMS_SQL.OPEN_CURSOR;
  DBMS_SQL.PARSE(cur, sql_text, DBMS_SQL.NATIVE);
  LOOP
    rows_processed:=DBMS_SQL.EXECUTE(cur);
    n:=n+rows_processed;

    UPDATE update_code_progress
    SET updated=n
    WHERE table_name=curRow.table_name AND column_name=curRow.column_name AND
          (partition_name IS NULL AND curRow.partition_name IS NULL OR partition_name=curRow.partition_name);
    IF SQL%NOTFOUND THEN
      INSERT INTO update_code_progress(table_name, column_name, num_rows, partition_name, updated, update_priority)
      VALUES(curRow.table_name, curRow.column_name, curRow.num_rows, curRow.partition_name, n, upd_priority);
    END IF;
    COMMIT;

    EXIT WHEN rows_processed<max_rows;
  END LOOP;
  DBMS_SQL.CLOSE_CURSOR(cur);
--  ROLLBACK;
  IF n>0 THEN
    DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(n,12)||' '||curRow.partition_name);
  END IF;
END update_code;

PROCEDURE airline_count(airline_code airlines.code%TYPE,
                        max_num_rows user_tables.num_rows%TYPE)
IS
BEGIN
  DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');
  FOR curRow IN oper_cur LOOP
    print_count(curRow, airline_code, max_num_rows);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
  DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');
  FOR curRow IN hist_cur LOOP
    print_count(curRow, airline_code, max_num_rows);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
  DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
  FOR curRow IN arx_cur LOOP
    print_count(curRow, airline_code, max_num_rows);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
END airline_count;

PROCEDURE airline_update_oper(old_airline_code airlines.code%TYPE,
                              new_airline_code airlines.code%TYPE,
                              max_rows user_tables.num_rows%TYPE)
IS
vid airlines.id%TYPE;
BEGIN
  IF old_airline_code=new_airline_code THEN RETURN; END IF;

  vid:=NULL;
  DELETE FROM airlines WHERE code=new_airline_code AND pr_del<>0 RETURNING id INTO vid;
  IF vid IS NULL THEN
    SELECT id__seq.nextval INTO vid FROM dual;
  END IF;

  INSERT INTO airlines
    (code,code_lat,code_icao,code_icao_lat,name,name_lat,short_name,short_name_lat,aircode,city,
     id,tid,tid_sync,pr_del)
  SELECT new_airline_code,code_lat,code_icao,code_icao_lat,name,name_lat,short_name,short_name_lat,aircode,city,
     vid,tid__seq.nextval,NULL,0
  FROM airlines
  WHERE code=old_airline_code;
  hist.synchronize_history('airlines',vid,NULL,NULL);

  vid:=NULL;
  UPDATE airlines
  SET code_lat=NULL, code_icao=NULL, code_icao_lat=NULL, aircode=NULL, tid=tid__seq.currval, pr_del=1
  WHERE code=old_airline_code RETURNING id INTO vid;
  hist.synchronize_history('airlines',vid,NULL,NULL);
  COMMIT;

  DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');

  FOR curRow IN oper_cur LOOP
    other_constraints(curRow, FALSE);
  END LOOP;

  FOR curRow IN oper_cur LOOP
    update_code(curRow, old_airline_code, new_airline_code, max_rows, 1);
  END LOOP;

  FOR curRow IN oper_cur LOOP
    other_constraints(curRow, TRUE);
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('==========================================================================');

  UPDATE aodb_airlines SET aodb_code=old_airline_code WHERE airline=new_airline_code AND aodb_code=new_airline_code;
END airline_update_oper;

PROCEDURE airline_update_arx(old_airline_code airlines.code%TYPE,
                             new_airline_code airlines.code%TYPE,
                             max_rows user_tables.num_rows%TYPE)
IS
BEGIN
  IF old_airline_code=new_airline_code THEN RETURN; END IF;

  DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');

  FOR curRow IN hist_cur LOOP
    other_constraints(curRow, FALSE);
  END LOOP;

  FOR curRow IN hist_cur LOOP
    update_code(curRow, old_airline_code, new_airline_code, max_rows, 2);
  END LOOP;

  FOR curRow IN hist_cur LOOP
    other_constraints(curRow, TRUE);
  END LOOP;

  DBMS_OUTPUT.PUT_LINE('==========================================================================');

  DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
  FOR curRow IN arx_cur LOOP
    update_code(curRow, old_airline_code, new_airline_code, max_rows, 3);
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
END airline_update_arx;

PROCEDURE view_update_progress
IS
BEGIN
  FOR curRow IN progress_cur LOOP
    IF curRow.num_rows>0 THEN
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(curRow.num_rows,12)||' '||curRow.partition_name);
    END IF;
  END LOOP;
END view_update_progress;

PROCEDURE users_logoff(new_airline_code airlines.code%TYPE)
IS
  CURSOR cur(code airlines.code%TYPE) IS
    SELECT user_id
    FROM users2
    WHERE desk IS NOT NULL AND
          adm.check_airline_access(code, user_id)<>0;
BEGIN
  FOR curRow IN cur(new_airline_code) LOOP
    UPDATE users2 SET desk = NULL WHERE user_id = curRow.user_id;
  END LOOP;
  COMMIT;
END users_logoff;

END utils;
/
