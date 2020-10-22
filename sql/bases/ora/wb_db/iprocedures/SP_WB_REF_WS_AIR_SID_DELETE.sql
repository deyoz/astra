create or replace procedure SP_WB_REF_WS_AIR_SID_DELETE
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

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select f.P_ID,
             f.P_ID
    from (select to_number(extractValue(value(t),'list/id[1]')) as P_ID
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    insert into WB_REF_WS_AIR_SUP_INFO_ADV_HST(ID_,
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
		                                                                 DATE_FROM,
                                                                       REMARK,
		                                                                     U_NAME,
		                                                                       U_IP,
		                                                                         U_HOST_NAME,
		                                                                           DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SUP_ADV_HST.nextval,
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
		                             i.DATE_FROM,		
		                               i.REMARK,
		                                 i.U_NAME,
		                                   i.U_IP,
		                                     i.U_HOST_NAME,
		                                       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SUPPL_INFO_ADV i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_SUPPL_INFO_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_ADV.id);

    insert into WB_REF_WS_AIR_SUP_INF_AD_I_HST(ID_,
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
		                                                                 DATE_FROM,
                                                                       ADV_ID,
	                                                                       ITEM_ID,
                                                                           REMARK,
		                                                                         U_NAME,
		                                                                           U_IP,
		                                                                             U_HOST_NAME,
		                                                                               DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SUP_AD_I_HST.nextval,
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
		                             i.DATE_FROM,
                                   i.ADV_ID,
	                                   i.ITEM_ID,
		                                   i.REMARK,
		                                     i.U_NAME,
		                                       i.U_IP,
		                                         i.U_HOST_NAME,
		                                           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SUPPL_INFO_ADV_I i
    on i.adv_id=t.id;

    delete from WB_REF_WS_AIR_SUPPL_INFO_ADV_I
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_ADV_I.adv_id);

   insert into WB_REF_WS_AIR_SUP_INF_DATA_HST (ID_,
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
		                                                                  ITEM_ID_OLD,
	                                                                      DET_COL_ID_OLD,
	                                                                        IS_CHECK_OLD,
                                                                            REMARK_OLD,
		                                                                          U_NAME_OLD,
		                                                                            U_IP_OLD,
		                                                                              U_HOST_NAME_OLD,
		                                                                                DATE_WRITE_OLD,
		                                                                                  ID_AC_NEW,
		                                                                                    ID_WS_NEW,
		                                                                                      ID_BORT_NEW,
		                                                                                        DATE_FROM_NEW,
		                                                                                          ITEM_ID_NEW,
	                                                                                              DET_COL_ID_NEW,
	                                                                                                IS_CHECK_NEW,
		                                                                                                REMARK_NEW,
		                                                                                                  U_NAME_NEW,
		                                                                                                    U_IP_NEW,
		                                                                                                      U_HOST_NAME_NEW,
		                                                                                                        DATE_WRITE_NEW,
                                                                                                              ADV_ID_OLD,
                                                                                                                ADV_ID_NEW)
    select SEC_WB_REF_WS_AIR_SUP_DATA_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.ID,
	                       i.ID_AC,
	                         i.ID_WS,
		                         i.ID_BORT,
		                           i.DATE_FROM,
		                             i.ITEM_ID,
	                                 i.DET_COL_ID,
	                                   i.IS_CHECK,
                                       i.REMARK,
		                                     i.U_NAME,
		                                       i.U_IP,
		                                         i.U_HOST_NAME,
		                                           i.DATE_WRITE,
		                                             i.ID_AC,
		                                               i.ID_WS,
		                                                 i.ID_BORT,
		                                                   i.DATE_FROM,
		                                                     i.ITEM_ID,
	                                                         i.DET_COL_ID,
	                                                           i.IS_CHECK,
		                                                           i.REMARK,
		                                                             i.U_NAME,
		                                                               i.U_IP,
		                                                                 i.U_HOST_NAME,
		                                                                   i.DATE_WRITE,
                                                                         i.adv_id,
                                                                           i.adv_id
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SUPPL_INFO_DATA i
    on i.id=t.id;

    delete from WB_REF_WS_AIR_SUPPL_INFO_DATA
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_DATA.id);

    str_msg:='EMPTY_STRING';


    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_SID_DELETE;
/
