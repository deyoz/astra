set echo off
set verify off
set feedback off
set termout off
set heading off
set linesize 200 
set trim on
set trims on
set pagesize 0
SET sqlprompt --==
set longchunksize 2000
set maxdata 999999
set arraysize 2
set long 200000
spool 2Comment/&1..sql

select
'comment on '||lower(table_type)||' '||table_name||' is '||''''||
comments||''''||';'
from user_tab_comments where table_name=upper('&1')
/

select
'comment on column '||table_name||'.'||column_name||' is '||''''||
comments||''''||';'
from user_col_comments where table_name=upper('&1')
/


spool off
set verify on
set feedback on
set termout on
set heading on
