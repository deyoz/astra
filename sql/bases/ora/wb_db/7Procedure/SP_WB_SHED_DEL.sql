create or replace procedure SP_WB_SHED_DEL
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
      insert into WB_SHED_HIST (ID_,
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
	                                                  BORT_NUM,
	                                                    NR,
	                                                      ID_AP_1,
	                                                        ID_AP_2,
	                                                          TERM_1,
	                                                            TERM_2,
	                                                              MVL_TYPE,
	                                                                S_DTU_1,
	                                                                  S_DTU_2,
	                                                                    S_DTL_1,
	                                                                      S_DTL_2,
	                                                                        E_DTU_1,
	                                                                          E_DTU_2,
	                                                                            E_DTL_1,
	                                                                              E_DTL_2,
	                                                                                F_DTU_1,
	                                                                                  F_DTU_2,
	                                                                                    F_DTL_1,
	                                                                                      F_DTL_2,
	                                                                                        U_NAME,
	                                                                                          U_IP,
	                                                                                            U_HOST_NAME,
	                                                                                              DATE_WRITE)
    select SEC_WB_SHED_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.ID,
	                         i.ID_AC,
	                           i.ID_WS,
	                             i.ID_BORT,
	                               i.BORT_NUM,
	                                 i.NR,
	                                   i.ID_AP_1,
	                                     i.ID_AP_2,
	                                       i.TERM_1,
	                                         i.TERM_2,
	                                           i.MVL_TYPE,
	                                             i.S_DTU_1,
	                                               i.S_DTU_2,
	                                                 i.S_DTL_1,
	                                                   i.S_DTL_2,
	                                                     i.E_DTU_1,
	                                                       i.E_DTU_2,
	                                                         i.E_DTL_1,
	                                                           i.E_DTL_2,
	                                                             i.F_DTU_1,
	                                                               i.F_DTU_2,
	                                                                 i.F_DTL_1,
	                                                                   i.F_DTL_2,
	                                                                     i.U_NAME,
	                                                                       i.U_IP,
	                                                                         i.U_HOST_NAME,
	                                                                           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_SHED i
    on i.id=t.id;

    delete from WB_SHED
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_SHED.id=t.id);

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_SHED_DEL;
/
