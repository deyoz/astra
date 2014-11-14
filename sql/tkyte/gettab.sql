SET verify off
SET feedback off
SET termout off
SET echo off;
SET pagesize 0
SET line 300
SET sqlprompt --==
SET trimspool on
SET termout off
SET serveroutput on SIZE 9000
spool 1Tab/&1..sql



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
   cur_coln                      NUMBER;
   c_cols                        NUMBER;
   lv_partitioned                user_tables.partitioned%TYPE;


   CURSOR part_columns_cursor (c_tab VARCHAR2)
   IS
      SELECT column_name
        FROM user_part_key_columns
       WHERE name = c_tab and UPPER(object_type) = 'TABLE'
       ORDER BY column_position;

   lv_part_column_name           user_part_key_columns.column_name%TYPE;
   


   CURSOR part_cursor (c_tab VARCHAR2)
   IS
      SELECT partition_name,
             high_value
        FROM user_tab_partitions
       WHERE table_name = c_tab
       ORDER BY partition_position;

   lv_partition_name             user_tab_partitions.partition_name%TYPE;
   lv_high_value                 user_tab_partitions.high_value%TYPE;
   
   lv_partitioning_type          user_part_tables.partitioning_type%TYPE;
   
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
            OR  (lv_data_type = 'NVARCHAR2')
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

         IF  (cur_coln!=c_cols)
         THEN
            lv_string := lv_string ||',';
         END IF;

         dbms_output.put_line(rtrim(lv_string,chr(10)));

      END LOOP;
      CLOSE col_cursor;

      select UPPER(partitioned) into lv_partitioned from user_all_tables where table_name=lv_table_name;
      IF ( lv_partitioned = 'YES')
      THEN
        select UPPER(partitioning_type) into lv_partitioning_type from user_part_tables where table_name=lv_table_name;
        lv_string := '/*'||CHR(10)||') PARTITION BY ' || lv_partitioning_type || ' ';
        
        -- COLUMNS
        lv_string := lv_string || '(';
        cur_coln := 0;
        OPEN part_columns_cursor (lv_table_name);

        LOOP
          FETCH part_columns_cursor INTO lv_part_column_name;
          EXIT WHEN part_columns_cursor%notfound;      

          IF (cur_coln!=0)
          THEN
            lv_string := lv_string || ',';
          END IF;
          cur_coln := 1;

          lv_string := lv_string || UPPER(lv_part_column_name);
        END LOOP;  
        CLOSE  part_columns_cursor;
        lv_string := lv_string || ') (';
        -- END COLUMNS
        
        dbms_output.put_line(lv_string);

        select count(*) into c_cols from user_tab_partitions where table_name=lv_table_name ;
        lv_string := NULL;
        cur_coln := 0;
        OPEN part_cursor (lv_table_name);

        LOOP
          cur_coln := cur_coln + 1;
          FETCH part_cursor INTO lv_partition_name,
                                 lv_high_value;
          EXIT WHEN part_cursor%notfound;      

          lv_string :=  'PARTITION ' || UPPER(lv_partition_name);
          IF ( lv_partitioning_type != 'HASH' )
          THEN
            lv_string := lv_string || ' VALUES';
          END IF;  
          IF ( lv_partitioning_type = 'RANGE')
          THEN
            lv_string := lv_string || ' LESS THAN';
          END IF; 
          
          lv_string := lv_string || '(' || lv_high_value || ')';

          IF  (cur_coln!=c_cols)
          THEN
             lv_string := lv_string ||',';
          END IF;

          dbms_output.put_line(rtrim(lv_string,chr(10)));
          
        END LOOP;  
        CLOSE part_cursor;

        dbms_output.put_line('*/');

      END IF; -- lv_partitioned = 'YES'
      
      lv_string := ');';
      dbms_output.put_line(lv_string);

END;
/
--show err
--SET heading off
 --SPOOL tab_rct2.sql
 --SELECT text
   --FROM t_temp
  --ORDER BY tb_name, lineno;
 --SPOOL ind_rct2.sql
 --SELECT text
   --FROM i_temp
  --ORDER BY tb_name, i_name, lineno;
-- 
 --SPOOL off
 --SET verify on
 --SET feedback on
 --SET termout on
 --SET pagesize 22
 --TTITLE OFF
 --SET ECHO OFF
 --PROMPT Table re-build script created.

