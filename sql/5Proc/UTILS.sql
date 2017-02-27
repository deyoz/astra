create or replace PACKAGE utils
AS

CURSOR oper_cur IS
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
         user_cons_columns.table_name IN (/*'AODB_EVENTS',*/
                                          'TLG_STAT',
                                          'RFISC_STAT'))
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
        user_tab_columns.table_name IN ('BI_PRINT_RULES',
                                        'CRS_TRANSFER',
                                        'RFISC_RATES',
--                                        'ROZYSK',
                                        'TLG_OUT',
                                        'UNACCOMP_BAG_INFO')
  ORDER BY num_rows, table_name;

CURSOR hist_cur IS
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
        (user_tab_columns.table_name like 'HIST%' OR
         user_tab_columns.table_name IN (/*'AODB_EVENTS',*/
                                         'TLG_STAT',
                                         'RFISC_STAT'))
  ORDER BY num_rows, table_name;

CURSOR progress_cur IS
  SELECT table_name, column_name, updated AS num_rows, partition_name
  FROM update_code_progress
  ORDER BY update_code_progress.update_priority,
           update_code_progress.num_rows,
           update_code_progress.table_name;

CURSOR arx_cur IS
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
        user_tab_columns.table_name like 'ARX%'
  ORDER BY num_rows, table_name;

CURSOR other_cons_cur(curRow oper_cur%ROWTYPE) IS
    SELECT user_constraints.table_name, user_constraints.constraint_name, user_constraints.r_constraint_name
    FROM user_constraints, user_cons_columns
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=curRow.table_name AND
          user_cons_columns.column_name=curRow.column_name AND
          user_constraints.r_constraint_name IS NOT NULL AND
          user_constraints.r_constraint_name<>'AIRLINES__PK';

PROCEDURE airline_tab_num_rows;
PROCEDURE airline_count(airline_code airlines.code%TYPE,
                        max_num_rows user_tables.num_rows%TYPE);
PROCEDURE airline_update_oper(old_airline_code airlines.code%TYPE,
                              new_airline_code airlines.code%TYPE,
                              max_rows user_tables.num_rows%TYPE);
PROCEDURE airline_update_arx(old_airline_code airlines.code%TYPE,
                             new_airline_code airlines.code%TYPE,
                             max_rows user_tables.num_rows%TYPE);
PROCEDURE view_update_progress;
PROCEDURE users_logoff(new_airline_code airlines.code%TYPE);

END utils;
/
