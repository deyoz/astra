define editor=vi

set serveroutput on size 1000000

set trimspool on
set long 5000
set linesize 100
set pagesize 9999
column plan_plus_exp format a80
column global_name new_value gname
set termout off
select lower(user) || '@' ||
lower(global_name) global_name from GLOBAL_NAME;
set sqlprompt '&gname SQL>> '
set termout on
