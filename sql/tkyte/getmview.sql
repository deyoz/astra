REM getaview.sql
set heading off
set long 99999999
set longchunksize 1000
set feedback off
set linesize 1000
set trimspool on
SET sqlprompt --==
set verify off
set termout off
set pages
set embedded on

column column_name format a90
column text format a30000

spool 1Tmview/&1..sql

prompt create materialized view &1 
select 'refresh '||refresh_method||' on '||refresh_mode||' as'
  from user_mviews
 where mview_name = upper('&1')
/
select query
  from user_mviews
 where mview_name = upper('&1')
/
prompt /
spool off

set termout on
set heading on
set feedback on
set verify on
