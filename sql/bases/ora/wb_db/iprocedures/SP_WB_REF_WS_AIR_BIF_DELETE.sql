create or replace procedure SP_WB_REF_WS_AIR_BIF_DELETE
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

    insert into WB_REF_WS_AIR_BAS_IND_FORM_HST (ID_,
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
		                                                                  REF_ARM_OLD,
	                                                                      K_CONST_OLD,
	                                                                        C_CONST_OLD,
	                                                                          LEN_MAC_RC_OLD,
	                                                                            LEMAC_LERC_OLD,
		                                                                            U_NAME_OLD,
		                                                                              U_IP_OLD,
		                                                                                U_HOST_NAME_OLD,
		                                                                                  DATE_WRITE_OLD,
		                                                                                    ID_AC_NEW,
		                                                                                      ID_WS_NEW,
		                                                                                        ID_BORT_NEW,
		                                                                                          DATE_FROM_NEW,
		                                                                                            REF_ARM_NEW,
	                                                                                                K_CONST_NEW,
	                                                                                                  C_CONST_NEW,
	                                                                                                    LEN_MAC_RC_NEW,
	                                                                                                      LEMAC_LERC_NEW,
		                                                                                                      U_NAME_NEW,
		                                                                                                        U_IP_NEW,
		                                                                                                          U_HOST_NAME_NEW,
		                                                                                                            DATE_WRITE_NEW)
    select SEC_WB_REF_WS_AIR_B_I_FORM_HST.nextval,
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
		                             i.REF_ARM,
	                                 i.K_CONST,
	                                   i.C_CONST,
	                                     i.LEN_MAC_RC,
	                                       i.LEMAC_LERC,
		                                       i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                             i.DATE_WRITE,
		                                               i.ID_AC,
		                                                 i.ID_WS,
		                                                   i.ID_BORT,
		                                                     i.DATE_FROM,
		                                                       i.REF_ARM,
	                                                           i.K_CONST,
	                                                             i.C_CONST,
	                                                               i.LEN_MAC_RC,
	                                                                 i.LEMAC_LERC,
		                                                                 i.U_NAME,
		                                                                   i.U_IP,
		                                                                     i.U_HOST_NAME,
		                                                                       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_BAS_IND_FORM i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_BAS_IND_FORM
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_BAS_IND_FORM.id);

    str_msg:='EMPTY_STRING';


    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_BIF_DELETE;
/
