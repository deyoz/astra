create or replace procedure SP_WB_REF_WS_AIR_WCC_DEL
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

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select f.P_ID,
             f.P_ID
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as P_ID
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ----------------------------ÇéáåéÜçõÖ èêéÇÖêäà------------------------------
    --ÇëíÄÇàíú èêéÇÖêäà!!!!
    select count(i.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_REG_WGT i
    on i.WCC_ID=t.id;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='In the value field "Weight Configuration Code Name" is referenced in the "Registration Weights" block. The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='ç† ß≠†Á•≠®Ô ØÆ´Ô "Weight Configuration Code Name" ®¨•Ó‚·Ô ··Î´™® ¢ °´Æ™• "Registration Weights". éØ•‡†Ê®Ô ß†Ø‡•È•≠†!';
          end;
        end if;
      end;
    end if;

    ----------------------------ÇéáåéÜçõÖ èêéÇÖêäà------------------------------
    ----------------------------------------------------------------------------

    if V_STR_MSG is null then
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
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_WCC_AC i
        on i.WCC_ID=t.id;

        delete from WB_REF_WS_AIR_WCC_AC
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where WB_REF_WS_AIR_WCC_AC.WCC_ID=t.id);

        insert into WB_REF_WS_AIR_WCC_HIST(ID_,
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
                                                                 CODE_NAME,
	                                                                 CREW_CODE_ID,
	                                                                   PANTRY_CODE_ID,
	                                                                     PORTABLE_WATER_CODE_ID,
	                                                                       U_NAME,
		                                                                       U_IP,
		                                                                         U_HOST_NAME,
		                                                                           DATE_WRITE)
        select SEC_WB_REF_WS_AIR_WCC_HIST.nextval,
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
                                     i.CODE_NAME,
	                                     i.CREW_CODE_ID,
	                                       i.PANTRY_CODE_ID,
	                                         i.PORTABLE_WATER_CODE_ID,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
		                                               i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_WCC i
        on i.id=t.id;

        delete from WB_REF_WS_AIR_WCC
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where WB_REF_WS_AIR_WCC.id=t.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_WCC_DEL;
/
