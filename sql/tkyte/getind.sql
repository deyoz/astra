SET verify off
set long 99999999
set feedback off
set linesize 1500
set longchunksize 1500
SET termout off
SET echo off;
SET pagesize 0
SET trimspool on
SET sqlprompt --==
SET termout off
set serveroutput on
spool &3/&2..&1..sql


DECLARE

   CURSOR ind_cursor (c_tab VARCHAR2)
   IS
      SELECT table_name,uniqueness,index_type,funcidx_status,partitioned,table_owner FROM user_indexes WHERE index_name = c_tab;

   CURSOR icol_cursor (c_ind VARCHAR2)
   IS
     SELECT column_name,descend FROM user_ind_columns where index_name= c_ind ORDER BY column_position;
 
   CURSOR iexp_cursor (c_ind VARCHAR2)
   IS
     SELECT column_expression FROM user_ind_expressions where index_name=c_ind ORDER BY column_position;

   lv_string                     VARCHAR2(2000);
   lv_index_name user_indexes.index_name%TYPE := UPPER('&1');
   lv_table_name                 user_tables.table_name%TYPE;
   lv_icolumn_name               user_ind_columns.column_name%TYPE;
   lv_uniqueness                 user_indexes.uniqueness%TYPE;
   lv_index_type                 user_indexes.index_type%TYPE;
   lv_funcidx_status             user_indexes.funcidx_status%TYPE;
   lv_partitioned                user_indexes.partitioned%TYPE;
   lv_table_owner                user_indexes.table_owner%TYPE; 
   lv_first_rec                  BOOLEAN;
   lv_column_name                user_ind_columns.column_name%TYPE;
   lv_descend                    user_ind_columns.descend%TYPE;
   lv_column_expression          user_ind_expressions.column_expression%TYPE;

BEGIN

      OPEN ind_cursor(lv_index_name);
      FETCH ind_cursor INTO lv_table_name,lv_uniqueness,lv_index_type,
                            lv_funcidx_status,lv_partitioned,lv_table_owner;
      
      IF (lv_index_type='LOB')
      THEN
         dbms_output.put_line('-- '||lv_index_name|| ' is LOB index. Skipped.');
      ELSE   
         lv_string := 'CREATE';
         IF (lv_uniqueness != 'NONUNIQUE') THEN
            lv_string := lv_string || ' ' || lv_uniqueness;
         END IF;
         lv_index_type := trim(replace(replace(lv_index_type,'NORMAL',' '),'FUNCTION-BASED ',' '));
         IF ( lv_index_type IS NOT NULL) THEN
           lv_string :=lv_string || ' ' || lv_index_type;
         END IF;  
         lv_string :=lv_string ||' INDEX ';

         lv_string := lv_string || UPPER (lv_index_name) || ' ON ' || UPPER (lv_table_name) ||' (';

         lv_first_rec := TRUE;

         OPEN iexp_cursor(lv_index_name);
         OPEN icol_cursor(lv_index_name);
         LOOP
           FETCH icol_cursor INTO lv_icolumn_name,lv_descend;
           EXIT WHEN icol_cursor%notfound;

           IF  (lv_first_rec)
           THEN
             lv_first_rec := FALSE;
           ELSE
             lv_string := lv_string || ',';
           END IF;

           IF (lv_funcidx_status='ENABLED' AND INSTR(lv_icolumn_name,'$')>0) --FUNC
           THEN
             FETCH iexp_cursor INTO lv_column_expression;
             lv_string := lv_string || replace(
               replace(lv_column_expression,'"'||UPPER(lv_table_owner)||'"."',''),'"','');
           ELSE
             lv_string := lv_string || lv_icolumn_name;
           END IF;
           lv_string := lv_string || REPLACE(' ' || UPPER(lv_descend),' ASC','');
         END LOOP;
         CLOSE icol_cursor;
         CLOSE iexp_cursor;

         lv_string := lv_string || ')';
         
         if (lv_partitioned='YES')
         THEN
            lv_string := lv_string ||CHR(10)|| '-- LOCAL'||CHR(10);
         END IF; --lv_partitioned='YES'
         
         lv_string := lv_string || ';';
         dbms_output.put_line(lv_string);
      END IF; --LOB

      CLOSE ind_cursor;
END;
/

