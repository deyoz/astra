set feedback off
set heading off
set termout off
set linesize 1000
set trimspool on
SET sqlprompt --==
set pages
set verify off
spool 5Proc/&1..sql
---prompt set define off
select decode(line,1,'create or replace ', '' ) ||
       text text 
  from user_source
  where type='PACKAGE' and 
  name = upper('&&1')
 order by type, line;
prompt /
---prompt set define on
spool off
set feedback on
set heading on
set termout on
set linesize 100
