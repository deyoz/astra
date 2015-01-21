SET verify off
SET feedback off
SET termout off
SET echo off;
SET pagesize 0
SET line 300
SET trimspool on
SET termout off
SET sqlprompt --==
SET serveroutput on SIZE 9000
spool 1Tiot/&1..sql



DECLARE
   CURSOR col_cursor (c_tab VARCHAR2)
   IS
      SELECT column_name,
             data_type,
             data_length,
             data_precision,
             data_scale,
             nullable,
             data_default
        FROM user_tab_columns
       WHERE table_name = c_tab
       ORDER BY column_name;

    CURSOR constr_col_cursor (c_tab VARCHAR2, c_con VARCHAR2)
    IS
    SELECT column_name 
    FROM user_cons_columns where table_name = c_tab and
        constraint_name=c_con
    ORDER BY position;   

   lv_table_name user_tables.table_name%TYPE := UPPER('&1');
   lv_column_name                user_tab_columns.column_name%TYPE;
   lv_data_type                  user_tab_columns.data_type%TYPE;
   lv_data_length                user_tab_columns.data_length%TYPE;
   lv_data_precision             user_tab_columns.data_precision%TYPE;
   lv_data_scale                 user_tab_columns.data_scale%TYPE;
   lv_nullable                   user_tab_columns.nullable%TYPE;
   lv_default                    user_tab_columns.data_default%TYPE;
   lv_lineno                     NUMBER := 0;
   lv_string                     VARCHAR2(2000);
   lv_constr_name                user_constraints.constraint_name%TYPE;
   lv_constr_column              user_cons_columns.column_name%TYPE;
   cur_coln                      NUMBER;
   c_cols                        NUMBER;


BEGIN
	
      select count(*) into c_cols from user_tab_columns where table_name=lv_table_name ;
--      lv_string := 'DROP TABLE ' || UPPER (lv_table_name) || ';';
      dbms_output.put_line(lv_string);
      lv_string := 'CREATE TABLE ' || UPPER (lv_table_name) || ' (';
      dbms_output.put_line(lv_string);
      lv_string := NULL;
      cur_coln := 0;
      OPEN col_cursor (lv_table_name);


      LOOP
         cur_coln := cur_coln + 1;
         FETCH col_cursor INTO lv_column_name,
                               lv_data_type,
                               lv_data_length,
                               lv_data_precision,
                               lv_data_scale,
                               lv_nullable,
                               lv_default;
         EXIT WHEN col_cursor%notfound;

         lv_string :=  UPPER (lv_column_name) || ' ' || lv_data_type;

         IF  ( (lv_data_type = 'CHAR')
            OR  (lv_data_type = 'VARCHAR2')
            OR  (lv_data_type = 'RAW'))
         THEN
            lv_string := lv_string || '(' || lv_data_length || ')';
         END IF;

         IF   (lv_data_type = 'NUMBER' and lv_data_precision is not null)
         THEN
            lv_string := lv_string || '(' || lv_data_precision || ')';
         END IF;
         IF (lv_default is not null)
    	 THEN
	        lv_string := lv_string ||' DEFAULT '|| lv_default;
	     END IF;

         IF  (lv_nullable = 'N')
         THEN
            lv_string := rtrim(lv_string) ||' NOT NULL';
         END IF;

            lv_string := lv_string ||',';

         dbms_output.put_line(rtrim(lv_string,chr(10)));

      END LOOP;

      CLOSE col_cursor;
      select constraint_name into lv_constr_name from user_constraints where table_name=lv_table_name and constraint_type='P';

      lv_string := 'CONSTRAINT '||rtrim(lv_constr_name)||' PRIMARY KEY (';
      select count(*) into c_cols from user_cons_columns where table_name=lv_table_name and constraint_name=lv_constr_name ;
      cur_coln := 0;
      OPEN constr_col_cursor (lv_table_name,lv_constr_name);
      LOOP
        cur_coln := cur_coln + 1;
        FETCH constr_col_cursor INTO lv_constr_column;
        EXIT WHEN constr_col_cursor%NOTFOUND;
        lv_string := lv_string||rtrim(lv_constr_column);
        IF  (cur_coln!=c_cols) 
            THEN
            lv_string := lv_string ||',';
        END IF;
      END LOOP;
    lv_string := lv_string ||'))'||chr(10)||'ORGANIZATION INDEX;'||chr(10);
    dbms_output.put_line(lv_string);

END;
/

