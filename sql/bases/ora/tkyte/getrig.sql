REM getrig.sql
set echo off
set verify off
set feedback off
set termout off
set heading off
set linesize 2000 
set trim on
set trims on
set pagesize 0
SET sqlprompt --==
set longchunksize 2000
set maxdata 999999
set arraysize 2
set long 200000
spool 8Trig/&1..sql

select
'create or replace trigger '||trigger_name ||' '|| chr(10)||
 decode( substr( trigger_type, 1, 1 ),
         'A', 'AFTER', 'B', 'BEFORE', 'I', 'INSTEAD OF' ) ||
              chr(10) ||
 triggering_event || chr(10) ||
 'ON '|| table_name || ' ' || chr(10) ||
 decode( instr( trigger_type, 'EACH ROW' ), 0, null,
            'FOR EACH ROW' ) || 
 decode(when_clause,null,null,chr(10)||'WHEN ('||when_clause||' )'),
 trigger_body
from user_triggers
where trigger_name = upper('&1')
/
prompt /

spool off
set verify on
set feedback on
set termout on
set heading on
