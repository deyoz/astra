create or replace procedure SP_WB_REF_AC_MES_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      f.id
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;


    insert into WB_REF_AIRCO_AUTO_MES_ADV_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
	                                                              DATE_FROM_OLD,
                                                  	              REMARK_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
                                                  	                    U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,
	                                                                          ID_AC_NEW,
	                                                                            DATE_FROM_NEW,
	                                                                              REMARK_NEW,
	                                                                                U_NAME_NEW,
	                                                                                  U_IP_NEW,
	                                                                                    U_HOST_NAME_NEW,
	                                                                                       DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_AUT_MS_ADV_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.DATE_FROM,
                             i.REMARK,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,
                                       i.ID_AC,
	                                       i.DATE_FROM,
                                           i.REMARK,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_MES_ADV i
    on i.id=t.id;

    delete from WB_REF_AIRCO_AUTO_MES_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_AUTO_MES_ADV.id);

    insert into WB_REF_AIRCO_AUTO_MES_DET_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
	                                                              ADV_ID_OLD,
	                                                                MES_ID_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
	                                                                      U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,	
	                                                                          ID_AC_NEW,
	                                                                            ADV_ID_NEW,
	                                                                              MES_ID_NEW,
	                                                                                U_NAME_NEW,
	                                                                                  U_IP_NEW,
	                                                                                    U_HOST_NAME_NEW,
	                                                                                      DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_AUT_MS_DET_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ADV_ID,
	                           i.MES_ID,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,	
                                       i.ID_AC,
	                                       i.ADV_ID,
	                                         i.MES_ID,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_MES_DET i
    on i.adv_id=t.id;

    delete from WB_REF_AIRCO_AUTO_MES_DET
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_AUTO_MES_DET.adv_id);


      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_MES_DELETE;
/
