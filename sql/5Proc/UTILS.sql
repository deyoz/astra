create or replace PACKAGE utils
AS

/*
DROP TABLE update_code_progress;
CREATE TABLE update_code_progress
(
  table_name VARCHAR2(50),
  column_name VARCHAR2(50),
  num_rows NUMBER,
  partition_name VARCHAR2(50),
  updated NUMBER,
  update_priority NUMBER(1)
);
*/

CURSOR oper_cur_al IS
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
        user_tab_columns.table_name IN ('AODB_EVENTS',
                                        'AODB_PAX_CHANGE',
                                        'BI_PRINT_RULES',
                                        'CRS_TRANSFER',
                                        'PAID_RFISC',
                                        'PAX_SERVICES',
                                        'RFISC_RATES',
--                                        'ROZYSK',
                                        'TLG_OUT',
                                        'UNACCOMP_BAG_INFO')
  ORDER BY num_rows, table_name;

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
--                                      'ROZYSK',
                                        'STATION_HALLS')
  ORDER BY num_rows, table_name;


CURSOR hist_cur_al IS
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
         user_tab_columns.table_name IN ('TLG_STAT',
                                         'RFISC_STAT'))
  ORDER BY num_rows, table_name;

CURSOR hist_cur IS
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
        user_tab_columns.column_name not like '%TRANSPORT%' AND
        (user_tab_columns.table_name like 'HIST%' OR
         user_tab_columns.table_name IN ('TLG_STAT',
                                         'RFISC_STAT',
                                         'PFS_STAT'))
  ORDER BY num_rows, table_name;

CURSOR progress_cur IS
  SELECT table_name, column_name, updated AS num_rows, partition_name
  FROM update_code_progress
  ORDER BY update_code_progress.update_priority,
           update_code_progress.num_rows,
           update_code_progress.table_name;

CURSOR arx_cur_al IS
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

CURSOR arx_cur IS
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
        user_tab_columns.table_name like 'ARX%'
  ORDER BY num_rows, table_name;

CURSOR other_cons_cur_al(curRow oper_cur_al%ROWTYPE) IS
    SELECT user_constraints.table_name, user_constraints.constraint_name, user_constraints.r_constraint_name
    FROM user_constraints, user_cons_columns
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=curRow.table_name AND
          user_cons_columns.column_name=curRow.column_name AND
          user_constraints.r_constraint_name IS NOT NULL AND
          user_constraints.r_constraint_name<>'AIRLINES__PK';

CURSOR other_cons_cur(curRow oper_cur%ROWTYPE) IS
    SELECT user_constraints.table_name, user_constraints.constraint_name, user_constraints.r_constraint_name
    FROM user_constraints, user_cons_columns
    WHERE user_constraints.constraint_name=user_cons_columns.constraint_name AND
          user_cons_columns.table_name=curRow.table_name AND
          user_cons_columns.column_name=curRow.column_name AND
          user_constraints.r_constraint_name IS NOT NULL AND
          user_constraints.r_constraint_name<>'AIRPS__PK';

PROCEDURE airline_tab_num_rows;
PROCEDURE airline_count(airline_code airlines.code%TYPE,
                        max_num_rows user_tables.num_rows%TYPE);
PROCEDURE airline_update_oper(old_airline_code airlines.code%TYPE,
                              new_airline_code airlines.code%TYPE,
                              max_rows user_tables.num_rows%TYPE);
PROCEDURE airp_update_oper(old_airp_code airps.code%TYPE,
                           new_airp_code airps.code%TYPE,
                           max_rows user_tables.num_rows%TYPE);
PROCEDURE airline_update_arx(old_airline_code airlines.code%TYPE,
                             new_airline_code airlines.code%TYPE,
                             max_rows user_tables.num_rows%TYPE);
PROCEDURE airp_update_arx(old_airp_code airps.code%TYPE,
                          new_airp_code airps.code%TYPE,
                          max_rows user_tables.num_rows%TYPE);
PROCEDURE view_update_progress;
PROCEDURE users_logoff_al(new_airline_code airlines.code%TYPE);
PROCEDURE users_logoff_ap(new_airp_code airps.code%TYPE);

card_like_pattern CONSTANT VARCHAR2(100)   :='([1-9][0-9][0-9]|[0-9][1-9][0-9]|[0-9][0-9][1-9])[0-9] *[0-9]{2}[0-9]{2} *[0-9]{4} *[0-9]{4}';
card_instr_pattern CONSTANT VARCHAR2(100)  :=card_like_pattern;
card_replace_pattern CONSTANT VARCHAR2(100):='([0-9]{4})( *)([0-9]{2})([0-9]{2})( *)([0-9]{4})( *)([0-9]{4})';
card_replace_string CONSTANT VARCHAR2(100) :='\1\2\300\50000\7\8';
rem_like_pattern CONSTANT VARCHAR2(20)     :='(FQT.|OTHS|FOID)';

FUNCTION masking_cards(src IN VARCHAR2) RETURN VARCHAR2;
PROCEDURE masking_cards_update(vtab IN VARCHAR2);

END utils;
/
