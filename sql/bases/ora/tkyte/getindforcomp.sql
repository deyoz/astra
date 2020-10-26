SET verify off
SET feedback off
SET termout off
SET echo off;
SET pagesize 0
SET trimspool on
SET WRAP off
SET sqlprompt --==
set lines 3000
SET termout off
set serveroutput on
spool IndComp/&2..&1..sql


DECLARE

   CURSOR ind_cursor (c_tab VARCHAR2)
   IS
      SELECT table_name,uniqueness FROM user_indexes WHERE index_name = c_tab;

   CURSOR icol_cursor (c_ind VARCHAR2)
   IS
      SELECT column_name FROM user_ind_columns where index_name= c_ind ORDER BY column_position;

   lv_string                     VARCHAR2(2000);
   lv_index_name user_indexes.index_name%TYPE := UPPER('&1');
   lv_table_name                 user_tables.table_name%TYPE;
   lv_icolumn_name               user_ind_columns.column_name%TYPE;
   lv_uniqueness                 user_indexes.uniqueness%TYPE;
   lv_first_rec                  BOOLEAN;
   uni_name			varchar2(2000):='&2';

BEGIN

      OPEN ind_cursor(lv_index_name);
      FETCH ind_cursor INTO lv_table_name,lv_uniqueness;

--         lv_string := 'DROP INDEX ' || UPPER (lv_index_name) || ';';
         dbms_output.put_line(lv_string);

	 IF (lv_uniqueness='UNIQUE')
	 THEN
	   lv_string := 'CREATE UNIQUE INDEX ';
         ELSE
           lv_string := 'CREATE INDEX ';
	 END IF;

         lv_string := lv_string || UPPER (lv_index_name) || ' ON ' || UPPER (lv_table_name) ||' (';
	 OPEN icol_cursor(lv_index_name);
         lv_first_rec := TRUE;

	 LOOP
	   FETCH icol_cursor INTO lv_icolumn_name;
           EXIT WHEN icol_cursor%notfound;
	   uni_name:=uni_name ||'-'|| lv_icolumn_name;		
           IF  (lv_first_rec)
           THEN
              lv_first_rec := FALSE;
           ELSE
              lv_string := lv_string || ',';
           END IF;

	   lv_string := lv_string || lv_icolumn_name;
         END LOOP;

         CLOSE icol_cursor;
	 lv_string := lv_string || ');';
         uni_name:=uni_name ||'.sql';		
         dbms_output.put_line(lv_string);
         dbms_output.put_line('REM +-+-+ ' || uni_name);

      CLOSE ind_cursor;
END;
/

