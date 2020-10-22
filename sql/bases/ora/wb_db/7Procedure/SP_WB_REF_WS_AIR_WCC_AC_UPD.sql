create or replace procedure SP_WB_REF_WS_AIR_WCC_AC_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID number:=-1;

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

     insert into WB_TEMP_XML_ID (ID,
                                   num)
     select f.ADJ_CODE_ID,
              f.ADJ_CODE_ID
     from (select to_number(extractValue(value(t),'list/ADJ_CODE_ID[1]')) as ADJ_CODE_ID
           from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_WCC
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record "Weight Configuration Codes" block deleted!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись блока "Weight Configuration Codes" удалена!'; end if;
      end;
    end if;
    ----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_TEMP_XML_ID t
        where not exists(select 1
                         from WB_REF_WS_AIR_DOW_SWA_CODES c
                         where c.id=t.ID);

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Some records for "Adjustment Codes" removed from block "Service Weight Adjustment Codes"'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Некоторые записи для "Adjustment Codes" удалены из блока "Service Weight Adjustment Codes"!'; end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_WCC_AC_HIST(ID_,
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
                                                                   WCC_ID,
	                                                                   ADJ_CODE_ID,
	                                                                     U_NAME,
		                                                                     U_IP,
		                                                                       U_HOST_NAME,
		                                                                         DATE_WRITE)
        select SEC_WB_REF_WS_AIR_WCC_AC_HIST.nextval,
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
                                     i.WCC_ID,
	                                     i.ADJ_CODE_ID,
		                                     i.U_NAME,
		                                       i.U_IP,
		                                         i.U_HOST_NAME,
		                                           i.DATE_WRITE
        from WB_REF_WS_AIR_WCC_AC i
        where i.WCC_ID=P_ID;

        delete from WB_REF_WS_AIR_WCC_AC where WCC_ID=P_ID;

        insert into WB_REF_WS_AIR_WCC_AC (ID,
	                                          ID_AC,
	                                            ID_WS,
                                             	  ID_BORT,
	                                                WCC_ID,
	                                                  ADJ_CODE_ID,
	                                                    U_NAME,
	                                                      U_IP,
                                          	              U_HOST_NAME,
                                          	                DATE_WRITE)
        select SEC_WB_REF_WS_AIR_WCC_AC.nextval,
                 a.ID_AC,
	                 a.ID_WS,
                     a.ID_BORT,
	                     a.ID,
                         t.id,
                           P_U_NAME,
	                           P_U_IP,
                               P_U_HOST_NAME,
                                 sysdate()
        from WB_REF_WS_AIR_WCC a join WB_TEMP_XML_ID t
        on a.id=P_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_WCC_AC_UPD;
/
