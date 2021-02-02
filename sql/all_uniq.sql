select distinct '|' || substr(table_name,1,15) || ' | ' || substr(index_name, 1, 35) || '|'  from (SELECT i.index_name,
       c.column_position,
       c.column_name,
       i.table_name,
       i.uniqueness,
       i.table_owner
  FROM sys.all_indexes i,
       sys.all_ind_columns c,
       sys.all_tab_columns tc
 WHERE
   i.uniqueness  = 'UNIQUE'
   AND i.index_name  = c.index_name
   AND i.table_owner = c.table_owner
   AND i.table_name  = c.table_name
   AND tc.table_name = c.table_name
   AND tc.column_name = c.column_name
   AND tc.DATA_TYPE = 'VARCHAR2'
   and tc.NULLABLE = 'Y'
   AND i.owner       = c.index_owner
   AND c.index_name IN (SELECT index_name FROM sys.all_ind_columns WHERE column_position = 2)
)
where table_owner = 'ASTRA_TRUNK'
/
