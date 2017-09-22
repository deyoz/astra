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
  COMMIT;
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
  COMMIT;
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

/*
DROP TABLE masking_cards_progress;
CREATE TABLE masking_cards_progress
(
  table_name      VARCHAR2(30),
  last_part_key   DATE,
  last_id         NUMBER(9),
  step_part_key   NUMBER(5),
  step_id         NUMBER(9),
  updated         NUMBER(9)
);

ALTER TABLE masking_cards_progress
       ADD CONSTRAINT masking_cards_progress__PK PRIMARY KEY (table_name);

DELETE FROM masking_cards_progress;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'crs_pax_rem', NULL, MIN(pax_id), NULL, 50000, 0
FROM crs_pax_rem;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'crs_pax_fqt', NULL, MIN(pax_id), NULL, 50000, 0
FROM crs_pax_fqt;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'pax_rem', NULL, MIN(pax_id), NULL, 50000, 0
FROM pax_rem;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'pax_fqt', NULL, MIN(pax_id), NULL, 50000, 0
FROM pax_fqt;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'typeb_in_body', NULL, MIN(id), NULL, 20000, 0
FROM typeb_in_body;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'tlg_out', NULL, MIN(id), NULL, 20000, 0
FROM tlg_out;

INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'arx_pax_rem', MIN(part_key), NULL, 180, NULL, 0
FROM arx_pax_rem;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'arx_tlgs_in', MIN(part_key), NULL, 360, NULL, 0
FROM arx_tlgs_in;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'arx_tlg_out', MIN(part_key), NULL, 720, NULL, 0
FROM arx_tlg_out;
COMMIT;

RENAME masking_cards_progress TO masking_cards_progress2;

*/

FUNCTION masking_cards(src IN VARCHAR2) RETURN VARCHAR2
IS
pos INTEGER;
prior_pos INTEGER;
dest VARCHAR2(4000);
BEGIN
  pos:=0;
  prior_pos:=NULL;
  dest:=src;
  WHILE TRUE LOOP
   IF prior_pos IS NULL THEN
     pos:=REGEXP_INSTR(dest, card_instr_pattern);
   ELSE
     pos:=REGEXP_INSTR(dest, card_instr_pattern, prior_pos, 2);
   END IF;
   EXIT WHEN pos=0 OR pos IS NULL;
   dest:=REGEXP_REPLACE(dest, card_replace_pattern, card_replace_string, pos, 1);
--   DBMS_OUTPUT.PUT_LINE('prior_pos='||prior_pos||', pos='||pos||', dest='||SUBSTR(dest,pos));
   prior_pos:=pos;
  END LOOP;

  RETURN dest;
END masking_cards;

PROCEDURE masking_cards_update(vtab IN VARCHAR2)
IS
sets masking_cards_progress%ROWTYPE;
curr_part_key DATE;
fin_part_key DATE;
curr_id NUMBER(9);
fin_id NUMBER(9);
BEGIN
  SELECT * INTO sets FROM masking_cards_progress WHERE table_name=LOWER(vtab);
  IF sets.last_id IS NOT NULL THEN
    IF sets.table_name='crs_pax_rem' THEN
      SELECT MAX(pax_id) INTO fin_id FROM crs_pax_rem;
    END IF;
    IF sets.table_name='crs_pax_fqt' THEN
      SELECT MAX(pax_id) INTO fin_id FROM crs_pax_fqt;
    END IF;
    IF sets.table_name='pax_rem' THEN
      SELECT MAX(pax_id) INTO fin_id FROM pax_rem;
    END IF;
    IF sets.table_name='pax_fqt' THEN
      SELECT MAX(pax_id) INTO fin_id FROM pax_fqt;
    END IF;
    IF sets.table_name='typeb_in_body' THEN
      SELECT MAX(id) INTO fin_id FROM typeb_in_body;
    END IF;
    IF sets.table_name='tlg_out' THEN
      SELECT MAX(id) INTO fin_id FROM tlg_out;
    END IF;

    WHILE sets.last_id<=fin_id LOOP
      curr_id:=sets.last_id+sets.step_id;
      IF sets.table_name='crs_pax_rem' THEN
        UPDATE crs_pax_rem
        SET rem=masking_cards(rem)
        WHERE pax_id>=sets.last_id AND pax_id<curr_id AND
              REGEXP_LIKE(rem_code, rem_like_pattern) AND
              REGEXP_LIKE(rem, card_like_pattern);
      END IF;
      IF sets.table_name='crs_pax_fqt' THEN
        UPDATE crs_pax_fqt
        SET no=masking_cards(no)
        WHERE pax_id>=sets.last_id AND pax_id<curr_id AND
              REGEXP_LIKE(no, card_like_pattern);
      END IF;
      IF sets.table_name='pax_rem' THEN
        UPDATE pax_rem
        SET rem=masking_cards(rem)
        WHERE pax_id>=sets.last_id AND pax_id<curr_id AND
              REGEXP_LIKE(rem_code, rem_like_pattern) AND
              REGEXP_LIKE(rem, card_like_pattern);
      END IF;
      IF sets.table_name='pax_fqt' THEN
        UPDATE pax_fqt
        SET no=masking_cards(no)
        WHERE pax_id>=sets.last_id AND pax_id<curr_id AND
              REGEXP_LIKE(no, card_like_pattern);
      END IF;
      IF sets.table_name='typeb_in_body' THEN
        UPDATE typeb_in_body
        SET text=masking_cards(text)
        WHERE id>=sets.last_id AND id<curr_id AND
              REGEXP_LIKE(text, card_like_pattern);
      END IF;
      IF sets.table_name='tlg_out' THEN
        UPDATE tlg_out
        SET body=masking_cards(body)
        WHERE id>=sets.last_id AND id<curr_id AND
              REGEXP_LIKE(body, card_like_pattern);
      END IF;

      sets.updated:=SQL%ROWCOUNT;
      sets.last_id:=curr_id;
      ROLLBACK;
      UPDATE masking_cards_progress
      SET last_id=sets.last_id, updated=updated+sets.updated
      WHERE table_name=sets.table_name;
      COMMIT;
    END LOOP;
  END IF;

  IF sets.last_part_key IS NOT NULL THEN
    IF sets.table_name='arx_pax_rem' THEN
      SELECT MAX(part_key) INTO fin_part_key FROM arx_pax_rem;
    END IF;
    IF sets.table_name='arx_tlgs_in' THEN
      SELECT MAX(part_key) INTO fin_part_key FROM arx_tlgs_in;
    END IF;
    IF sets.table_name='arx_tlg_out' THEN
      SELECT MAX(part_key) INTO fin_part_key FROM arx_tlg_out;
    END IF;

    WHILE sets.last_part_key<=fin_part_key LOOP
      curr_part_key:=sets.last_part_key+(sets.step_part_key/1440);
      IF sets.table_name='arx_pax_rem' THEN
        UPDATE arx_pax_rem
        SET rem=masking_cards(rem)
        WHERE part_key>=sets.last_part_key AND part_key<curr_part_key AND
              REGEXP_LIKE(rem_code, rem_like_pattern) AND
              REGEXP_LIKE(rem, card_like_pattern);
      END IF;
      IF sets.table_name='arx_tlgs_in' THEN
        UPDATE arx_tlgs_in
        SET body=masking_cards(body)
        WHERE part_key>=sets.last_part_key AND part_key<curr_part_key AND
              REGEXP_LIKE(body, card_like_pattern);
      END IF;
      IF sets.table_name='arx_tlg_out' THEN
        UPDATE arx_tlg_out
        SET body=masking_cards(body)
        WHERE part_key>=sets.last_part_key AND part_key<curr_part_key AND
              REGEXP_LIKE(body, card_like_pattern);
      END IF;

      sets.updated:=SQL%ROWCOUNT;
      sets.last_part_key:=curr_part_key;
--      ROLLBACK;
      UPDATE masking_cards_progress
      SET last_part_key=sets.last_part_key, updated=updated+sets.updated
      WHERE table_name=sets.table_name;
      COMMIT;
    END LOOP;

  END IF;
EXCEPTION
  WHEN OTHERS THEN
  BEGIN
    ROLLBACK;
    RAISE;
  END;
END masking_cards_update;

END utils;
/
