create or replace procedure SP_WB_REF_AIRCO_LOGO_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]') into P_ID_AC from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    insert into WB_REF_AIRCOMPANY_LOGO_HISTORY (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      U_DATE_WRITE_,
	                                                        ACTION,
	                                                          ID,
	                                                            ID_AC_OLD,
	                                                              DATE_FROM_OLD,
	                                                                LOGO_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
	                                                                      U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,
	                                                                          ID_AC_NEW,
	                                                                            DATE_FROM_NEW,
	                                                                              LOGO_NEW,
	                                                                                U_NAME_NEW,
	                                                                                  U_IP_NEW,
	                                                                                    U_HOST_NAME_NEW,
	                                                                                      DATE_WRITE_NEW,
                                                                                          LOGO_TYPE_OLD,
                                                                                            LOGO_TYPE_NEW)
    select SEC_WB_REF_AIRCO_LOGO_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE,
                     'delete',
                       d.id,
                         d.ID_AC,
	                         d.DATE_FROM,
	                           d.LOGO,	
	                             d.U_NAME,
	                               d.U_IP,
	                                 d.U_HOST_NAME,
	                                   d.DATE_WRITE,
                                       d.ID_AC,
	                                       d.DATE_FROM,
	                                         d.LOGO,
                                             d.U_NAME,
	                                             d.U_IP,
	                                               d.U_HOST_NAME,
	                                                 d.DATE_WRITE,
                                                     d.LOGO_TYPE,
                                                       d.LOGO_TYPE

    from WB_REF_AIRCOMPANY_LOGO d
    where d.id_ac=P_ID_AC;

    delete from WB_REF_AIRCOMPANY_LOGO
    where id_ac=P_ID_AC;

    commit;
  end SP_WB_REF_AIRCO_LOGO_DELETE;
/
