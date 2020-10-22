create or replace procedure SP_WB_REF_WS_AIR_TYPE_DELETE
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

    insert into WB_REF_WS_AIR_TYPE_HIST (ID_,
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
		                                                           REVIZION_OLD,
		                                                             ID_WS_TRANSP_KATEG_OLD,
		                                                               ID_WS_TYPE_OF_LOADING_OLD,
		                                                                 IS_UPPER_DECK_OLD,
		                                                                   IS_MAIN_DECK_OLD,
		                                                                     IS_LOWER_DECK_OLD,
	                                                       	                 REMARK_OLD,
	                                                       	                   U_NAME_OLD,
                                                                               U_IP_OLD,
	                                                       	                       U_HOST_NAME_OLD,
		                                                                               DATE_WRITE_OLD,
		                                                                                 ID_AC_NEW,
	                                                       	                             ID_WS_NEW,
	                                                       	                               ID_BORT_NEW,
	                                                       	                                 DATE_FROM_NEW,
	                                                       	                                   REVIZION_NEW,
	                                                       	                                     ID_WS_TRANSP_KATEG_NEW,
	                                                       	                                       ID_WS_TYPE_OF_LOADING_NEW,
	                                                       	                                         IS_UPPER_DECK_NEW,
	                                                       	                                           IS_MAIN_DECK_NEW,
		                                                                                                   IS_LOWER_DECK_NEW,
	                                                       	                                               REMARK_NEW,
	                                                       	                                                 U_NAME_NEW,
		                                                                                                         U_IP_NEW,
		                                                                                                           U_HOST_NAME_NEW,
		                                                                                                             DATE_WRITE_NEW)
    select SEC_WB_REF_WS_AIR_TYPE_HIST.nextval,
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
		                             i.REVIZION,
		                               i.ID_WS_TRANSP_KATEG,
		                                 i.ID_WS_TYPE_OF_LOADING,
		                                   i.IS_UPPER_DECK,
		                                     i.IS_MAIN_DECK,
		                                       i.IS_LOWER_DECK,
	                                           i.REMARK,
	                                             i.U_NAME,
                                                 i.U_IP,
	                                                 i.U_HOST_NAME,
		                                                 i.DATE_WRITE,
		                                                   i.ID_AC,
	                                                       i.ID_WS,
	                                                       	 i.ID_BORT,
	                                                       	   i.DATE_FROM,
	                                                       	     i.REVIZION,
	                                                       	       i.ID_WS_TRANSP_KATEG,
	                                                       	         i.ID_WS_TYPE_OF_LOADING,
	                                                       	           i.IS_UPPER_DECK,
	                                                       	             i.IS_MAIN_DECK,
		                                                                     i.IS_LOWER_DECK,
	                                                       	                 i.REMARK,
	                                                       	                   i.U_NAME,
		                                                                           i.U_IP,
		                                                                             i.U_HOST_NAME,
		                                                                               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_TYPE i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_TYPE
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_TYPE.id);

    str_msg:='EMPTY_STRING';


    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_TYPE_DELETE;
/
