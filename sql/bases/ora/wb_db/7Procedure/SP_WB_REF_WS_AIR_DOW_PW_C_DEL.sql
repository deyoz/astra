create or replace procedure SP_WB_REF_WS_AIR_DOW_PW_C_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      f.id
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(ws.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_WCC ws
    on ws.PORTABLE_WATER_CODE_ID=t.id;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='In the value field "Portable Water Code Name" is referenced in the "Weight Configuration Codes" block. The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='На значения поля "Portable Water Code Name" имеются ссылки в блоке "Weight Configuration Codes". Операция запрещена!';
          end;
        end if;
      end;
    end if;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_DOW_PW_CODES_HST(ID_,
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
		                                                                     PW_CODE_NAME,
	                                                                         TOTAL_WEIGHT,	
	                                                                           WEIHGT_DIFF,
	                                                                             ARM_DIFF,
	                                                                               INDEX_DIFF,
	                                                                                 CODE_TYPE,
                                                                                     BY_DEFAULT,
	                                                                                     REMARKS,
		                                                                                     U_NAME,
		                                                                                       U_IP,
		                                                                                         U_HOST_NAME,
		                                                                                           DATE_WRITE)
        select SEC_WB_REF_WS_AIR_DOW_PW_C_HST.nextval,
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
                                     i.PW_CODE_NAME,
	                                     i.TOTAL_WEIGHT,	
	                                       i.WEIHGT_DIFF,
	                                         i.ARM_DIFF,
	                                           i.INDEX_DIFF,
    	                                         i.CODE_TYPE,
                                                 i.BY_DEFAULT,
	                                                 i.REMARKS,
		                                                 i.U_NAME,
		                                                   i.U_IP,
		                                                     i.U_HOST_NAME,
		                                                       i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_PW_CODES i
        on i.id=t.id;

        delete from WB_REF_WS_AIR_DOW_PW_CODES
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where WB_REF_WS_AIR_DOW_PW_CODES.id=t.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_DOW_PW_C_DEL;
/
