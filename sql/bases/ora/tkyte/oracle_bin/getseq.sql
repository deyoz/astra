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
column min_value 99999999999999999;
column max_value 99999999999999999;
spool 3Seq/&1..sql

select
'create sequence '||sequence_name ||' minvalue '||min_value||
' maxvalue '||max_value||' increment by '||increment_by||
decode (substr(cycle_flag,1,2),'Y', ' cycle ') 
from user_sequences where sequence_name=upper('&1')
/
prompt /

spool off
set verify on
set feedback on
set termout on
set heading on
