create or replace procedure SP_WB_REF_WS_AIR_S_L_PLAN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

cXML_data clob;
V_R_COUNT number:=0;
V_STR_MSG clob:=null;
begin

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

  cXML_out:='<?xml version="1.0" ?><root>';

  insert into WB_TEMP_XML_ID (ID,
                                num)
  select distinct f.ID,
                    f.ID
  from (select to_number(extractValue(value(t),'list/P_ID[1]')) as ID
        from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------

   --èêéÇÖêäà!!!!!!

  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
    insert into WB_REF_WS_AIR_S_L_PLAN_HST(ID_,
	                                           U_NAME_,
	                                             U_IP_,
	                                               U_HOST_NAME_,
	                                                 DATE_WRITE_,
                                                     OPERATION_,
	                                                     ACTION_,
	                                                       ID,
	                                                         ID_AC,
	                                                           ID_WS,
		                                                           ID_BORT,
                                                                 IDN,
	                                                                 ADV_ID,
	                                                                   CABIN_SECTION,
	                                                                     ROW_NUMBER,
	                                                                       MAX_WEIGHT,
	                                                                         MAX_SEATS,
	                                                                           BALANCE_ARM,
	                                                                             INDEX_PER_WT_UNIT,
		                                                                             U_NAME,
		                                                                               U_IP,
		                                                                                 U_HOST_NAME,
		                                                                                   DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_PLAN_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
	                           i.ID_WS,
		                           i.ID_BORT,
                                 i.IDN,
	                                 i.ADV_ID,
	                                   i.CABIN_SECTION,
	                                     i.ROW_NUMBER,
	                                       i.MAX_WEIGHT,
	                                         i.MAX_SEATS,
	                                           i.BALANCE_ARM,
	                                             i.INDEX_PER_WT_UNIT,
                                                 i.U_NAME,
		                                        	     i.U_IP,
		                                                 i.U_HOST_NAME,
		                                 	                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_PLAN i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_S_L_PLAN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_PLAN.id=t.id);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_P_S_HST(ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
	                                                DATE_WRITE_,
                                                    OPERATION_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC,
	                                                          ID_WS,
		                                                          ID_BORT,
                                                                IDN,
	                                                                ADV_ID,
	                                                                  PLAN_ID,
	                                                                    SEAT_ID,
	                                                                      IS_SEAT,
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE,
                                                                                  MAX_WEIGHT)
    select SEC_WB_REF_WS_AIR_S_L_P_S_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
	                           i.ID_WS,
		                           i.ID_BORT,
                                 i.IDN,
	                                 i.ADV_ID,
	                                   i.PLAN_ID,
	                                     i.SEAT_ID,
	                                       i.IS_SEAT,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE,
                                                   i.MAX_WEIGHT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_P_S i
    on i.plan_id=t.id;

    delete from WB_REF_WS_AIR_S_L_P_S
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_S_L_P_S.PLAN_ID);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_P_S_P_HST(ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
                                                      OPERATION_,
	                                                      ACTION_,
	                                                        ID,
	                                                          ID_AC,
	                                                            ID_WS,
		                                                            ID_BORT,
                                                                  IDN,
	                                                                  ADV_ID,
	                                                                    PLAN_ID,
	                                                                      S_SEAT_ID,
	                                                                        PARAM_ID,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_P_S_P_HS.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
	                           i.ID_WS,
		                           i.ID_BORT,
                                 i.IDN,
	                                 i.ADV_ID,
	                                   i.PLAN_ID,
	                                     i.S_SEAT_ID,
	                                       i.PARAM_ID,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_P_S_P i
    on i.plan_id=t.id;

    delete from WB_REF_WS_AIR_S_L_P_S_P
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_S_L_P_S_P.PLAN_ID);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_S_L_PLAN_DEL;
/
