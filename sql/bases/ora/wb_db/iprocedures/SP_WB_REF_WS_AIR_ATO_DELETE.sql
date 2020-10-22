create or replace procedure SP_WB_REF_WS_AIR_ATO_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID, num)
    select f.id,
             f.id
    from (select to_number(extractValue(value(t),'list/id[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    insert into WB_REF_WS_AIR_ATO_HIST (ID_,
	                                        U_NAME_,
	                                          U_IP_,
	                                            U_HOST_NAME_,
	                                              DATE_WRITE_,
	                                                ACTION_,
	                                                  ID,
	                                                    ID_AC_OLD,
	                                                      ID_WS_OLD,
		                                                      ID_BORT_OLD,
		                                                        DATE_FROM_OLD,
		                                                          SORT_CONTAINER_LOADING_OLD,
	                                                              EZFW_THRESHOLD_LIMIT_OLD,
	                                                                EZFW_THRESHOLD_LIMIT_VAL_OLD,
	                                                                  CHECK_LATERAL_I_LIMITS_OLD,
	                                                                    PTO_CLASS_TRIM_OLD,
	                                                                      PTO_CABIN_AREA_TRIM_OLD,
	                                                                        PTO_SEAT_ROW_TRIM_OLD,
	                                                                          REMARK_OLD,
		                                                                          U_NAME_OLD,
		                                                                            U_IP_OLD,
		                                                                              U_HOST_NAME_OLD,
		                                                                                DATE_WRITE_OLD,
		                                                                                  ID_AC_NEW,
		                                                                                    ID_WS_NEW,
		                                                                                      ID_BORT_NEW,
		                                                                                        DATE_FROM_NEW,
		                                                                                          SORT_CONTAINER_LOADING_NEW,
	                                                                                              EZFW_THRESHOLD_LIMIT_NEW,
	                                                                                                EZFW_THRESHOLD_LIMIT_VAL_NEW,
	                                                                                                  CHECK_LATERAL_I_LIMITS_NEW,
	                                                                                                    PTO_CLASS_TRIM_NEW,
	                                                                                                      PTO_CABIN_AREA_TRIM_NEW,
	                                                                                                        PTO_SEAT_ROW_TRIM_NEW,
	                                                                                                          REMARK_NEW,
		                                                                                                          U_NAME_NEW,
		                                                                                                            U_IP_NEW,
		                                                                                                              U_HOST_NAME_NEW,
		                                                                                                                DATE_WRITE_NEW)
   select SEC_WB_REF_WS_AIR_ATO_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ID_WS,
		                         i.ID_BORT,
		                           i.DATE_FROM,
		                             i.SORT_CONTAINER_LOADING,
	                                 i.EZFW_THRESHOLD_LIMIT,
	                                   i.EZFW_THRESHOLD_LIMIT_VAL,
	                                     i.CHECK_LATERAL_IMBALANCE_LIMITS,
	                                       i.PTO_CLASS_TRIM,
	                                         i.PTO_CABIN_AREA_TRIM,
	                                           i.PTO_SEAT_ROW_TRIM,
	                                             i.REMARK,
		                                             i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                                   i.DATE_WRITE,
		                                                     i.ID_AC,
		                                                       i.ID_WS,
		                                                         i.ID_BORT,
		                                                           i.DATE_FROM,
		                                                             i.SORT_CONTAINER_LOADING,
	                                                                 i.EZFW_THRESHOLD_LIMIT,
	                                                                   i.EZFW_THRESHOLD_LIMIT_VAL,
	                                                                     i.CHECK_LATERAL_IMBALANCE_LIMITS,
	                                                                       i.PTO_CLASS_TRIM,
	                                                                         i.PTO_CABIN_AREA_TRIM,
	                                                                           i.PTO_SEAT_ROW_TRIM,
	                                                                             i.REMARK,
		                                                                             i.U_NAME,
		                                                                               i.U_IP,
		                                                                                 i.U_HOST_NAME,
		                                                                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_ATO i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_ATO
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_ATO.id);

    str_msg:='EMPTY_STRING';


    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_ATO_DELETE;
/
