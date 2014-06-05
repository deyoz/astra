set pages 
set line 200
set trims on
set trim on
set echo off;
spool data_types.lst
select type_name from sys.user_types order by type_name;
spool off
spool tables.lst
select table_name from user_tables where table_name not in (select mview_name from user_mviews where mview_name=table_name) and iot_type is null and table_name not like '%$%' order by table_name;
spool off
spool indexes.lst
select ui.index_name, ui.table_name from user_indexes ui,user_tables ut where ut.table_name=ui.table_name and ut.iot_type is null and (ui.funcidx_status is NULL) and ut.table_name not in (select mview_name from user_mviews where mview_name=ui.table_name)  and not exists (select 1 from user_constraints where table_name=ui.table_name and index_name=ui.index_name and constraint_type='P' and constraint_name=index_name and generated='GENERATED NAME')   and ui.table_name not like '%$%' and ui.index_name not like '%$%' order by index_name;
spool off
spool mv_indexes.lst
select ui.index_name, ui.table_name from user_indexes ui,user_tables ut where ut.table_name=ui.table_name and ut.iot_type is null and (ui.funcidx_status is NULL) and ut.table_name in (select mview_name from user_mviews where mview_name=ui.table_name) and not exists (select 1 from user_constraints where table_name=ui.table_name and index_name=ui.index_name and constraint_type='P' and constraint_name=index_name and generated='GENERATED NAME') and ui.table_name not like '%$%' and ui.index_name not like '%$%' order by index_name;
spool off
spool func_indexes.lst
select ui.index_name, ui.table_name from user_indexes ui,user_tables ut where ut.table_name=ui.table_name and ut.iot_type is null and (ui.funcidx_status='ENABLED') and not exists (select 1 from user_constraints where table_name=ui.table_name and index_name=ui.index_name and constraint_type='P' and constraint_name=index_name and generated='GENERATED NAME')   and ui.table_name not like '%$%' and ui.index_name not like '%$%' order by index_name;
spool off
spool iots.lst
select table_name from user_tables where table_name not in (select mview_name from user_mviews where mview_name=table_name) and iot_type is not null and table_name not like '%$%' and nvl(iot_name,' ') not like '%$%' order by table_name;
spool off
spool views.lst
select view_name from user_views order by view_name;
spool off
spool mviews.lst
select mview_name from user_mviews order by mview_name;
spool off
spool constr.lst
select uc.constraint_name,uc.table_name,uc.index_name,decode(uc.generated,'USER NAME',uc.constraint_name,'_GENERATED_') from user_constraints uc,user_tables ut where ut.iot_type is null and uc.constraint_type='P' and uc.table_name=ut.table_name and uc.table_name not in (select queue_table from user_queues) and ut.iot_type is null and ut.table_name not like '%$%' and nvl(ut.iot_name,' ') not like '%$%' and uc.constraint_name not like '%$%' order by constraint_name,table_name;
spool off
spool triggers.lst
select trigger_name from user_triggers order by trigger_name;
spool off
spool sequences.lst
select sequence_name from user_sequences order by sequence_name;
spool off
spool functions.lst
select object_name from user_objects where object_type='FUNCTION'
order by object_name;
spool off
spool packages.lst
select object_name from user_objects where object_type='PACKAGE'
order by object_name;
spool off
spool procedures.lst
select object_name from user_objects where object_type='PROCEDURE'
order by object_name;
spool off

exit;
