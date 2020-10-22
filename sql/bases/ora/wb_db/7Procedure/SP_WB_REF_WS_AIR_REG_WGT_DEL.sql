create or replace procedure SP_WB_REF_WS_AIR_REG_WGT_DEL
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



  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      insert into WB_REF_WS_AIR_REG_WGT_HST(ID_,
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
                                                                  ADV_ID,
	                                                                  DATE_FROM,
	                                                                    S_L_ADV_ID,
	                                                                      WCC_ID,
	                                                                        IS_ACARS,
	                                                                          IS_DEFAULT,
	                                                                            IS_APPROVED,
	                                                                              DOW,
	                                                                                DOI,
	                                                                                  ARM,
	                                                                                    PROC_MAC,
	                                                                                      REMARK,
	                                                                                        U_NAME,
		                                                                                        U_IP,
		                                                                                          U_HOST_NAME,
		                                                                                            DATE_WRITE)
    select SEC_WB_REF_WS_AIR_REG_WGT_HS.nextval,
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
	                               i.ADV_ID,
	                                 i.DATE_FROM,
	                                   i.S_L_ADV_ID,
	                                     i.WCC_ID,
	                                       i.IS_ACARS,
	                                         i.IS_DEFAULT,
	                                           i.IS_APPROVED,
	                                             i.DOW,
	                                               i.DOI,
	                                                 i.ARM,
	                                                   i.PROC_MAC,
	                                                     i.REMARK,
	                                                       i.U_NAME,
		                               	                       i.U_IP,
		                                	                       i.U_HOST_NAME,
		                                 	                         i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_REG_WGT i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_REG_WGT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_REG_WGT.id=t.id);

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_REG_WGT_DEL;
/
