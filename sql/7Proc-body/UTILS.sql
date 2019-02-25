create or replace PACKAGE BODY utils
AS

TYPE cur_row_type IS RECORD
(
table_name     user_tab_columns.table_name%TYPE,
column_name    user_tab_columns.column_name%TYPE,
num_rows       user_tables.num_rows%TYPE,
partition_name user_tab_partitions.partition_name%TYPE
);

TYPE cur_type IS REF CURSOR RETURN cur_row_type;

TYPE cons_cur_row_type IS RECORD
(
table_name        user_constraints.table_name%TYPE,
constraint_name   user_constraints.constraint_name%TYPE,
r_constraint_name user_constraints.r_constraint_name%TYPE
);

TYPE cons_cur_type IS REF CURSOR RETURN cons_cur_row_type;

al_codes   CONSTANT NUMBER(1):=1;
ap_codes   CONSTANT NUMBER(1):=2;
city_codes CONSTANT NUMBER(1):=3;

FUNCTION get_oper_cur(codes NUMBER) RETURN cur_type
IS
cur cur_type;
BEGIN
  CASE codes

  WHEN al_codes THEN
    OPEN cur FOR
    SELECT user_cons_columns.table_name, user_cons_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_constraints, user_cons_columns, user_tables, user_ind_columns, user_tab_partitions
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          user_cons_columns.table_name=user_ind_columns.table_name(+) AND
          user_cons_columns.column_name=user_ind_columns.column_name(+) AND
          user_ind_columns.column_position(+)=1 AND
          user_constraints.r_constraint_name='AIRLINES__PK' AND
          user_cons_columns.table_name not like 'DROP%' AND
          user_cons_columns.column_name not like 'DROP%' AND
       NOT(user_cons_columns.table_name like 'ARX%' OR
           user_cons_columns.table_name like 'HIST%' OR
           user_cons_columns.table_name IN ('TLG_STAT',
                                            'RFISC_STAT',
                                            'PAX_NORMS_TEXT'))
    UNION
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRL%' OR
           user_tab_columns.column_name like '%COMPAN%') AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name IN ('AODB_EVENTS',
                                          'AODB_PAX_CHANGE',
                                          'BI_PRINT_RULES',
                                          'CRS_TRANSFER',
                                          'PAID_RFISC',
                                          'PAX_SERVICES',
                                          'RFISC_RATES',
                                          'TLG_OUT',
                                          'UNACCOMP_BAG_INFO')
    ORDER BY num_rows, table_name, column_name;

  WHEN ap_codes THEN
    OPEN cur FOR
    SELECT user_cons_columns.table_name, user_cons_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_constraints, user_cons_columns, user_tables, user_ind_columns, user_tab_partitions
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          user_cons_columns.table_name=user_ind_columns.table_name(+) AND
          user_cons_columns.column_name=user_ind_columns.column_name(+) AND
          user_ind_columns.column_position(+)=1 AND
          user_constraints.r_constraint_name='AIRPS__PK' AND
          user_cons_columns.table_name not like 'DROP%' AND
          user_cons_columns.column_name not like 'DROP%' AND
       NOT(user_cons_columns.table_name like 'ARX%' OR
           user_cons_columns.table_name like 'HIST%' OR
           user_cons_columns.table_name IN ('TLG_STAT',
                                            'RFISC_STAT'))
    UNION
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRP%' OR
           user_tab_columns.column_name like '%PORT%') AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.column_name not like '%PASSPORT%' AND
          user_tab_columns.table_name IN ('AODB_PAX_CHANGE',
                                          'APPS_PAX_DATA',
                                          'BASEL_STAT',
                                          'CRS_PNR',
                                          'CRS_TRANSFER',
                                          'DCS_TAGS',
                                          'STATION_HALLS')
    ORDER BY num_rows, table_name, column_name;

  WHEN city_codes THEN
    OPEN cur FOR
    SELECT user_cons_columns.table_name, user_cons_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_constraints, user_cons_columns, user_tables, user_ind_columns, user_tab_partitions
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          user_cons_columns.table_name=user_ind_columns.table_name(+) AND
          user_cons_columns.column_name=user_ind_columns.column_name(+) AND
          user_ind_columns.column_position(+)=1 AND
          user_constraints.r_constraint_name='CITIES__PK' AND
          user_cons_columns.table_name not like 'DROP%' AND
          user_cons_columns.column_name not like 'DROP%' AND
       NOT(user_cons_columns.table_name like 'ARX%' OR
           user_cons_columns.table_name like 'HIST%')
    UNION
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%CITY%') AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name IN (NULL)
    ORDER BY num_rows, table_name, column_name;

  END CASE;
  RETURN cur;
END get_oper_cur;

FUNCTION get_hist_cur(codes NUMBER) RETURN cur_type
IS
cur cur_type;
BEGIN
  CASE codes

  WHEN al_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRL%' OR
           user_tab_columns.column_name like '%COMPAN%' OR
           (user_tab_columns.column_name='SEGMENTS' AND user_tab_columns.table_name='TRFER_PAX_STAT')) AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          (user_tab_columns.table_name like 'HIST%' OR
           user_tab_columns.table_name IN ('TLG_STAT',
                                           'ROZYSK',
                                           'TRFER_PAX_STAT',
                                           'RFISC_STAT',
                                           'PAX_NORMS_TEXT'))
    ORDER BY num_rows, table_name, column_name;

  WHEN ap_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRP%' OR
           user_tab_columns.column_name like '%PORT%' OR
           (user_tab_columns.column_name='SEGMENTS' AND user_tab_columns.table_name='TRFER_PAX_STAT')) AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.column_name not like '%TRANSPORT%' AND
          (user_tab_columns.table_name like 'HIST%' OR
           user_tab_columns.table_name IN ('TLG_STAT',
                                           'RFISC_STAT',
                                           'ROZYSK',
                                           'TRFER_PAX_STAT',
                                           'PFS_STAT'))
    ORDER BY num_rows, table_name, column_name;

  WHEN city_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          user_tab_columns.column_name like '%CITY%' AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name <> 'HIST_CRYPT_REQ_DATA' AND
          user_tab_columns.table_name like 'HIST%'
    ORDER BY num_rows, table_name, column_name;

  END CASE;
  RETURN cur;
END get_hist_cur;

FUNCTION get_arx_cur(codes NUMBER) RETURN cur_type
IS
cur cur_type;
BEGIN
  CASE codes

  WHEN al_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRL%' OR
           user_tab_columns.column_name like '%COMPAN%' OR
           (user_tab_columns.column_name='SEGMENTS' AND user_tab_columns.table_name='ARX_TRFER_PAX_STAT')) AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name like 'ARX%'
    ORDER BY num_rows, table_name, column_name;

  WHEN ap_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          (user_tab_columns.column_name like '%AIRP%' OR
           user_tab_columns.column_name like '%PORT%' OR
           (user_tab_columns.column_name='SEGMENTS' AND user_tab_columns.table_name='ARX_TRFER_PAX_STAT')) AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name like 'ARX%'
    ORDER BY num_rows, table_name, column_name;

  WHEN city_codes THEN
    OPEN cur FOR
    SELECT user_tab_columns.table_name, user_tab_columns.column_name,
           DECODE(user_tab_partitions.table_name, NULL, user_tables.num_rows, user_tab_partitions.num_rows) AS num_rows,
           user_tab_partitions.partition_name
    FROM user_tab_columns, user_tables, user_tab_partitions
    WHERE user_tab_columns.table_name=user_tables.table_name AND
          user_tables.table_name=user_tab_partitions.table_name(+) AND
          user_tab_columns.column_name like '%CITY%' AND
          user_tab_columns.data_type='VARCHAR2' AND
          user_tab_columns.table_name not like 'DROP%' AND
          user_tab_columns.column_name not like 'DROP%' AND
          user_tab_columns.table_name like 'ARX%' AND
          user_tab_columns.table_name <> 'ARX_PAX_DOCA'
    ORDER BY num_rows, table_name, column_name;

  END CASE;
  RETURN cur;
END get_arx_cur;

FUNCTION get_other_cons_cur(codes NUMBER, curRow cur_row_type) RETURN cons_cur_type
IS
cur cons_cur_type;
cons_name user_constraints.r_constraint_name%TYPE;
BEGIN
  cons_name:=
    CASE codes
    WHEN al_codes THEN 'AIRLINES__PK'
    WHEN ap_codes THEN 'AIRPS__PK'
    WHEN city_codes THEN 'CITIES__PK'
    END;

  OPEN cur FOR
    SELECT user_constraints.table_name, user_constraints.constraint_name, user_constraints.r_constraint_name
    FROM user_constraints, user_cons_columns
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=curRow.table_name AND
          user_cons_columns.column_name=curRow.column_name AND
          user_constraints.r_constraint_name IS NOT NULL AND
          user_constraints.r_constraint_name<>cons_name;
  RETURN cur;
END get_other_cons_cur;

PROCEDURE other_constraints(cur IN cons_cur_type, venable BOOLEAN)
IS
sql_text VARCHAR2(4000);
curRow   cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;
    IF venable THEN
      sql_text:='ALTER TABLE '||curRow.table_name||' ENABLE CONSTRAINT '||curRow.constraint_name;
    ELSE
      sql_text:='ALTER TABLE '||curRow.table_name||' DISABLE CONSTRAINT '||curRow.constraint_name;
    END IF;
    EXECUTE IMMEDIATE sql_text;
    DBMS_OUTPUT.PUT_LINE(sql_text||';');
  END LOOP;
  CLOSE cur;
END other_constraints;

PROCEDURE disable_constraints(codes NUMBER, cur IN cur_type)
IS
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;
    other_constraints(get_other_cons_cur(codes, curRow), FALSE);
  END LOOP;
  CLOSE cur;
END disable_constraints;

PROCEDURE enable_constraints(codes NUMBER, cur IN cur_type)
IS
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;
    other_constraints(get_other_cons_cur(codes, curRow), TRUE);
  END LOOP;
  CLOSE cur;
END enable_constraints;

PROCEDURE disable_constraints(codes NUMBER)
IS
BEGIN
  disable_constraints(codes, get_oper_cur(codes));
END disable_constraints;

PROCEDURE enable_constraints(codes NUMBER)
IS
BEGIN
  enable_constraints(codes, get_oper_cur(codes));
END enable_constraints;

PROCEDURE print_other_constraints(cur IN cons_cur_type)
IS
curRow   cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;
    DBMS_OUTPUT.PUT_LINE(curRow.constraint_name||' -> '||curRow.r_constraint_name||'!!!');
  END LOOP;
  CLOSE cur;
END print_other_constraints;

PROCEDURE print_tab_num_rows(codes NUMBER, cur IN cur_type)
IS
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;
    DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(curRow.num_rows,12)||' '||curRow.partition_name);
    print_other_constraints(get_other_cons_cur(codes, curRow));
  END LOOP;
  CLOSE cur;
END print_tab_num_rows;

PROCEDURE print_tab_num_rows(codes NUMBER,
                             show_oper BOOLEAN,
                             show_hist BOOLEAN,
                             show_arx BOOLEAN)
IS
BEGIN
  IF show_oper THEN
    DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');
    print_tab_num_rows(codes, get_oper_cur(codes));
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_hist THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');
    print_tab_num_rows(codes, get_hist_cur(codes));
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_arx THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
    print_tab_num_rows(codes, get_arx_cur(codes));
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
END print_tab_num_rows;

PROCEDURE airline_tab_num_rows(show_oper BOOLEAN,
                               show_hist BOOLEAN,
                               show_arx BOOLEAN)
IS
BEGIN
  print_tab_num_rows(al_codes, show_oper, show_hist, show_arx);
END airline_tab_num_rows;

PROCEDURE airp_tab_num_rows(show_oper BOOLEAN,
                            show_hist BOOLEAN,
                            show_arx BOOLEAN)
IS
BEGIN
  print_tab_num_rows(ap_codes, show_oper, show_hist, show_arx);
END airp_tab_num_rows;

PROCEDURE city_tab_num_rows(show_oper BOOLEAN,
                            show_hist BOOLEAN,
                            show_arx BOOLEAN)
IS
BEGIN
  print_tab_num_rows(city_codes, show_oper, show_hist, show_arx);
END city_tab_num_rows;

FUNCTION get_where_text(curRow cur_row_type,
                        old_code VARCHAR2,
                        prefix VARCHAR2) RETURN VARCHAR2
IS
where_text VARCHAR2(4000);
BEGIN
  --WHERE condition
  IF curRow.table_name like '%TRFER_PAX_STAT' AND curRow.column_name='SEGMENTS' THEN
    where_text:='WHERE '||prefix||curRow.column_name||' LIKE ''%'||old_code||'%''';
  ELSE
    where_text:='WHERE '||prefix||curRow.column_name||'='''||old_code||'''';
  END IF;

  IF curRow.table_name='ROZYSK' THEN
    where_text:=where_text||' AND time>SYSDATE-2';
  END IF;
  RETURN where_text;
END get_where_text;

FUNCTION get_sql_text(codes NUMBER,
                      curRow cur_row_type,
                      old_code VARCHAR2,
                      new_code VARCHAR2,
                      max_rows INTEGER) RETURN VARCHAR2
IS
update_text VARCHAR2(4000);
set_text VARCHAR2(4000);
prefix VARCHAR2(2);
BEGIN
  --UPDATE table
  update_text:='UPDATE '||curRow.table_name||' ';
  prefix:=NULL;
  IF curRow.partition_name IS NOT NULL THEN
    update_text:=update_text||'PARTITION ('||curRow.partition_name||') p ';
    prefix:='p.';
  END IF;

  --SET fields
  set_text:='SET '||prefix||curRow.column_name||'='''||new_code||''' ';
  IF curRow.table_name like '%TRFER_PAX_STAT' AND curRow.column_name='SEGMENTS' THEN
    CASE codes
    WHEN al_codes THEN
      set_text:='SET '||prefix||curRow.column_name||'=REGEXP_REPLACE('||prefix||curRow.column_name||', ''(;|^)'||old_code||'(,)'', ''\1'||new_code||'\2'') ';
    WHEN ap_codes THEN
      set_text:='SET '||prefix||curRow.column_name||'=REGEXP_REPLACE('||prefix||curRow.column_name||', ''(,)'||old_code||'([,;]|$)'', ''\1'||new_code||'\2'') ';
    END CASE;
  END IF;

  RETURN update_text||set_text||get_where_text(curRow, old_code, prefix)||' AND rownum<='||max_rows;
END get_sql_text;

PROCEDURE print_count(cur IN cur_type,
                      code VARCHAR2,
                      max_num_rows user_tables.num_rows%TYPE)
IS
selCur INTEGER;
n INTEGER;
ignore INTEGER;
sql_text VARCHAR2(4000);
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;

    IF curRow.partition_name IS NOT NULL THEN
      sql_text:='SELECT COUNT(*) FROM '||curRow.table_name||' PARTITION ('||curRow.partition_name||') p '||get_where_text(curRow, code, 'p.');
    ELSE
      sql_text:='SELECT COUNT(*) FROM '||curRow.table_name||' '||get_where_text(curRow, code, NULL);
    END IF;
    IF curRow.num_rows IS NULL OR curRow.num_rows<=max_num_rows THEN
      selCur:=DBMS_SQL.OPEN_CURSOR;
      DBMS_SQL.PARSE(selCur, sql_text, DBMS_SQL.NATIVE);
      DBMS_SQL.DEFINE_COLUMN(selCur,1,n);
      ignore:=DBMS_SQL.EXECUTE(selCur);
      ignore:=DBMS_SQL.FETCH_ROWS(selCur);
      DBMS_SQL.COLUMN_VALUE(selCur,1,n);
      DBMS_SQL.CLOSE_CURSOR(selCur);
      IF n>0 THEN
        DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(n,12)||' '||curRow.partition_name);
      END IF;
    ELSE
      DBMS_OUTPUT.PUT_LINE(sql_text);
    END IF;
  END LOOP;
  CLOSE cur;
END;

PROCEDURE print_count(codes NUMBER,
                      code VARCHAR2,
                      max_num_rows user_tables.num_rows%TYPE,
                      show_oper BOOLEAN,
                      show_hist BOOLEAN,
                      show_arx BOOLEAN)
IS
BEGIN
  IF show_oper THEN
    DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');
    print_count(get_oper_cur(codes), code, max_num_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_hist THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');
    print_count(get_hist_cur(codes), code, max_num_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_arx THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
    print_count(get_arx_cur(codes), code, max_num_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
END print_count;

PROCEDURE airline_count(airline_code airlines.code%TYPE,
                        max_num_rows user_tables.num_rows%TYPE,
                        show_oper BOOLEAN,
                        show_hist BOOLEAN,
                        show_arx BOOLEAN)
IS
BEGIN
  print_count(al_codes, airline_code, max_num_rows, show_oper, show_hist, show_arx);
END airline_count;

PROCEDURE airp_count(airp_code airps.code%TYPE,
                     max_num_rows user_tables.num_rows%TYPE,
                     show_oper BOOLEAN,
                     show_hist BOOLEAN,
                     show_arx BOOLEAN)
IS
BEGIN
  print_count(ap_codes, airp_code, max_num_rows, show_oper, show_hist, show_arx);
END airp_count;

PROCEDURE city_count(city_code cities.code%TYPE,
                     max_num_rows user_tables.num_rows%TYPE,
                     show_oper BOOLEAN,
                     show_hist BOOLEAN,
                     show_arx BOOLEAN)
IS
BEGIN
  print_count(city_codes, city_code, max_num_rows, show_oper, show_hist, show_arx);
END city_count;

PROCEDURE print_updates(codes NUMBER,
                        cur IN cur_type,
                        old_code VARCHAR2,
                        new_code VARCHAR2,
                        max_rows user_tables.num_rows%TYPE)
IS
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;

    DBMS_OUTPUT.PUT_LINE(get_sql_text(codes, curRow, old_code, new_code, max_rows)||';');
  END LOOP;
  CLOSE cur;
END;

PROCEDURE print_updates(codes NUMBER,
                        old_code VARCHAR2,
                        new_code VARCHAR2,
                        max_rows user_tables.num_rows%TYPE,
                        show_oper BOOLEAN,
                        show_hist BOOLEAN,
                        show_arx BOOLEAN)
IS
BEGIN
  IF show_oper THEN
    DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');
    print_updates(codes, get_oper_cur(codes), old_code, new_code, max_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_hist THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');
    print_updates(codes, get_hist_cur(codes), old_code, new_code, max_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
  IF show_arx THEN
    DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
    print_updates(codes, get_arx_cur(codes), old_code, new_code, max_rows);
    DBMS_OUTPUT.PUT_LINE('==========================================================================');
  END IF;
END print_updates;

PROCEDURE airline_print_updates(old_airline_code airlines.code%TYPE,
                                new_airline_code airlines.code%TYPE,
                                max_rows user_tables.num_rows%TYPE,
                                show_oper BOOLEAN,
                                show_hist BOOLEAN,
                                show_arx BOOLEAN)
IS
BEGIN
  print_updates(al_codes, old_airline_code, new_airline_code, max_rows, show_oper, show_hist, show_arx);
END airline_print_updates;

PROCEDURE airp_print_updates(old_airp_code airps.code%TYPE,
                             new_airp_code airps.code%TYPE,
                             max_rows user_tables.num_rows%TYPE,
                             show_oper BOOLEAN,
                             show_hist BOOLEAN,
                             show_arx BOOLEAN)
IS
BEGIN
  print_updates(ap_codes, old_airp_code, new_airp_code, max_rows, show_oper, show_hist, show_arx);
END airp_print_updates;

PROCEDURE city_print_updates(old_city_code cities.code%TYPE,
                             new_city_code cities.code%TYPE,
                             max_rows user_tables.num_rows%TYPE,
                             show_oper BOOLEAN,
                             show_hist BOOLEAN,
                             show_arx BOOLEAN)
IS
BEGIN
  print_updates(city_codes, old_city_code, new_city_code, max_rows, show_oper, show_hist, show_arx);
END city_print_updates;

PROCEDURE update_code(codes NUMBER,
                      cur IN cur_type,
                      old_code VARCHAR2,
                      new_code VARCHAR2,
                      max_rows INTEGER,
                      upd_priority NUMBER,
                      with_commit BOOLEAN)
IS
updCur INTEGER;
n INTEGER;
rows_processed INTEGER;
curRow cur%ROWTYPE;
BEGIN
  LOOP
    FETCH cur INTO curRow;
    EXIT WHEN cur%NOTFOUND;

    n:=0;
    updCur:=DBMS_SQL.OPEN_CURSOR;
    DBMS_SQL.PARSE(updCur, get_sql_text(codes, curRow, old_code, new_code, max_rows), DBMS_SQL.NATIVE);
    LOOP
      rows_processed:=DBMS_SQL.EXECUTE(updCur);
      n:=n+rows_processed;

      UPDATE update_code_progress
      SET updated=n
      WHERE table_name=curRow.table_name AND column_name=curRow.column_name AND
            (partition_name IS NULL AND curRow.partition_name IS NULL OR partition_name=curRow.partition_name);
      IF SQL%NOTFOUND THEN
        INSERT INTO update_code_progress(table_name, column_name, num_rows, partition_name, updated, update_priority)
        VALUES(curRow.table_name, curRow.column_name, curRow.num_rows, curRow.partition_name, n, upd_priority);
      END IF;
      IF with_commit THEN
        COMMIT;
      END IF;

      EXIT WHEN rows_processed<max_rows;
    END LOOP;
    DBMS_SQL.CLOSE_CURSOR(updCur);
  --  ROLLBACK;
    IF n>0 THEN
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(n,12)||' '||curRow.partition_name);
    END IF;
  END LOOP;
  CLOSE cur;
END update_code;

PROCEDURE update_oper(codes NUMBER,
                      old_code VARCHAR2,
                      new_code VARCHAR2,
                      max_rows user_tables.num_rows%TYPE,
                      with_commit BOOLEAN)
IS
BEGIN
  IF old_code=new_code THEN RETURN; END IF;

  DBMS_OUTPUT.PUT_LINE('Оперативные таблицы:');

  IF with_commit THEN
    disable_constraints(codes, get_oper_cur(codes));
  END IF;

  update_code(codes, get_oper_cur(codes), old_code, new_code, max_rows, 1, with_commit);

  IF with_commit THEN
    enable_constraints(codes, get_oper_cur(codes));
  END IF;

  DBMS_OUTPUT.PUT_LINE('==========================================================================');
END update_oper;

PROCEDURE airline_update_oper(old_airline_code airlines.code%TYPE,
                              new_airline_code airlines.code%TYPE,
                              max_rows user_tables.num_rows%TYPE,
                              with_commit BOOLEAN)
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
  IF with_commit THEN
    COMMIT;
  END IF;

  update_oper(al_codes, old_airline_code, new_airline_code, max_rows, with_commit);

  UPDATE convert_airlines SET code_external=old_airline_code WHERE code_internal=new_airline_code AND code_external=new_airline_code;
  IF with_commit THEN
    COMMIT;
  END IF;
END airline_update_oper;

PROCEDURE airp_update_oper(old_airp_code airps.code%TYPE,
                           new_airp_code airps.code%TYPE,
                           max_rows user_tables.num_rows%TYPE,
                           with_commit BOOLEAN)
IS
vid airps.id%TYPE;
BEGIN
  IF old_airp_code=new_airp_code THEN RETURN; END IF;

  vid:=NULL;
  DELETE FROM airps WHERE code=new_airp_code AND pr_del<>0 RETURNING id INTO vid;
  IF vid IS NULL THEN
    SELECT id__seq.nextval INTO vid FROM dual;
  END IF;

  INSERT INTO airps
    (code,code_lat,code_icao,code_icao_lat,name,name_lat,city,
     id,tid,tid_sync,pr_del)
  SELECT new_airp_code,code_lat,code_icao,code_icao_lat,name,name_lat,city,
     vid,tid__seq.nextval,NULL,0
  FROM airps
  WHERE code=old_airp_code;
  hist.synchronize_history('airps',vid,NULL,NULL);

  vid:=NULL;
  UPDATE airps
  SET code_lat=NULL, code_icao=NULL, code_icao_lat=NULL, tid=tid__seq.currval, pr_del=1
  WHERE code=old_airp_code RETURNING id INTO vid;
  hist.synchronize_history('airps',vid,NULL,NULL);
  IF with_commit THEN
    COMMIT;
  END IF;

--  SELECT id INTO vid FROM airps WHERE code=new_airp_code AND pr_del<>0;

--  UPDATE airps
--  SET tid=tid__seq.nextval, pr_del=0
--  WHERE id=vid;
--  hist.synchronize_history('airps',vid,NULL,NULL);
--  IF with_commit THEN
--    COMMIT;
--  END IF;

  update_oper(ap_codes, old_airp_code, new_airp_code, max_rows, with_commit);

  UPDATE convert_airps SET code_external=old_airp_code WHERE code_internal=new_airp_code AND code_external=new_airp_code;
  IF with_commit THEN
    COMMIT;
  END IF;
END airp_update_oper;

PROCEDURE city_update_oper(old_city_code cities.code%TYPE,
                           new_city_code cities.code%TYPE,
                           max_rows user_tables.num_rows%TYPE,
                           with_commit BOOLEAN)
IS
vid cities.id%TYPE;
BEGIN
  IF old_city_code=new_city_code THEN RETURN; END IF;

  vid:=NULL;
  DELETE FROM cities WHERE code=new_city_code AND pr_del<>0 RETURNING id INTO vid;
  IF vid IS NULL THEN
    SELECT id__seq.nextval INTO vid FROM dual;
  END IF;

  INSERT INTO cities
    (code,code_lat,country,name,name_lat,tz_region,
     id,tid,tid_sync,pr_del)
  SELECT new_city_code,code_lat,country,name,name_lat,tz_region,
     vid,tid__seq.nextval,NULL,0
  FROM cities
  WHERE code=old_city_code;
  hist.synchronize_history('cities',vid,NULL,NULL);

  vid:=NULL;
  UPDATE cities
  SET code_lat=NULL, tid=tid__seq.currval, pr_del=1
  WHERE code=old_city_code RETURNING id INTO vid;
  hist.synchronize_history('cities',vid,NULL,NULL);
  IF with_commit THEN
    COMMIT;
  END IF;

  update_oper(city_codes, old_city_code, new_city_code, max_rows, with_commit);

  UPDATE airps SET tid=tid__seq.currval WHERE city=new_city_code;
  IF with_commit THEN
    COMMIT;
  END IF;
END city_update_oper;

PROCEDURE update_arx(codes NUMBER,
                     old_code VARCHAR2,
                     new_code VARCHAR2,
                     max_rows user_tables.num_rows%TYPE)
IS
BEGIN
  IF old_code=new_code THEN RETURN; END IF;

  DBMS_OUTPUT.PUT_LINE('Таблицы истории и статистики:');

  disable_constraints(codes, get_hist_cur(codes));

  update_code(codes, get_hist_cur(codes), old_code, new_code, max_rows, 2, TRUE);

  enable_constraints(codes, get_hist_cur(codes));

  DBMS_OUTPUT.PUT_LINE('==========================================================================');

  DBMS_OUTPUT.PUT_LINE('Таблицы архива:');
  update_code(codes, get_arx_cur(codes), old_code, new_code, max_rows, 3, TRUE);
  DBMS_OUTPUT.PUT_LINE('==========================================================================');
  COMMIT;
END update_arx;

PROCEDURE airline_update_arx(old_airline_code airlines.code%TYPE,
                             new_airline_code airlines.code%TYPE,
                             max_rows user_tables.num_rows%TYPE)
IS
BEGIN
  update_arx(al_codes, old_airline_code, new_airline_code, max_rows);
END airline_update_arx;

PROCEDURE airp_update_arx(old_airp_code airps.code%TYPE,
                          new_airp_code airps.code%TYPE,
                          max_rows user_tables.num_rows%TYPE)
IS
BEGIN
  update_arx(ap_codes, old_airp_code, new_airp_code, max_rows);
END airp_update_arx;

PROCEDURE city_update_arx(old_city_code cities.code%TYPE,
                          new_city_code cities.code%TYPE,
                          max_rows user_tables.num_rows%TYPE)
IS
BEGIN
  update_arx(city_codes, old_city_code, new_city_code, max_rows);
END city_update_arx;

PROCEDURE view_update_progress
IS
CURSOR progress_cur IS
  SELECT table_name, column_name, updated AS num_rows, partition_name
  FROM update_code_progress
  ORDER BY update_code_progress.update_priority,
           update_code_progress.num_rows,
           update_code_progress.table_name,
           update_code_progress.column_name;
BEGIN
  FOR curRow IN progress_cur LOOP
    IF curRow.num_rows>0 THEN
      DBMS_OUTPUT.PUT_LINE(RPAD(curRow.table_name,30)||RPAD(curRow.column_name,15)||LPAD(curRow.num_rows,12)||' '||curRow.partition_name);
    END IF;
  END LOOP;
END view_update_progress;

PROCEDURE users_logoff_al(new_airline_code airlines.code%TYPE)
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
END users_logoff_al;

PROCEDURE users_logoff_ap(new_airp_code airps.code%TYPE)
IS
  CURSOR cur(code airps.code%TYPE) IS
    SELECT user_id
    FROM users2
    WHERE desk IS NOT NULL;
BEGIN
  FOR curRow IN cur(new_airp_code) LOOP
    UPDATE users2 SET desk = NULL WHERE user_id = curRow.user_id;
  END LOOP;
  COMMIT;
END users_logoff_ap;

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
SELECT 'typeb_in_body', NULL, MIN(id), NULL, 10000, 0
FROM typeb_in_body;
INSERT INTO masking_cards_progress(table_name, last_part_key, last_id, step_part_key, step_id, updated)
SELECT 'tlg_out', NULL, MIN(id), NULL, 10000, 0
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
--      ROLLBACK;
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
