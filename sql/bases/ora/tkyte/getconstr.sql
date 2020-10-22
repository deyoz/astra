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
spool 2Constr/&2..&4..sql



DECLARE

    CURSOR constr_col_cursor (c_tab VARCHAR2, c_con VARCHAR2)
    IS
    SELECT column_name 
    FROM user_cons_columns where table_name = c_tab and
        constraint_name=c_con
    ORDER BY position;   

   lv_constr_name                user_cons_columns.constraint_name%TYPE := UPPER('&1');
   lv_table_name                 user_cons_columns.table_name%TYPE := UPPER('&2');
   lv_index_name                 user_constraints.index_name%TYPE;
   lv_generated                  user_constraints.generated%TYPE;
   lv_lineno                     NUMBER := 0;
   lv_string                     VARCHAR2(2000);
   lv_constr_column              user_cons_columns.column_name%TYPE;
   cur_coln                      NUMBER;
   c_cols                        NUMBER;


BEGIN
	
      select constraint_name,generated,index_name into lv_constr_name,lv_generated,lv_index_name from user_constraints where table_name=lv_table_name and constraint_type='P';

      lv_string := 'ALTER TABLE '||lv_table_name||' ADD';
      IF (lv_generated = 'USER NAME') 
      THEN
        lv_string := lv_string||' CONSTRAINT '||lv_constr_name; 
      END IF;                                                                      
      lv_string := lv_string||' PRIMARY KEY (';

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
    lv_string := lv_string ||')';
    
    IF (lv_generated = 'USER NAME') 
    THEN 
      lv_string := lv_string||' USING INDEX '||lv_index_name;
    END IF;
      
    lv_string := lv_string||';'||chr(10);
      
    dbms_output.put_line(lv_string);

END;
/

