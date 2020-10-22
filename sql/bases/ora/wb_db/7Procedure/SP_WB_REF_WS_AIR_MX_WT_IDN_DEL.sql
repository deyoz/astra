create or replace procedure SP_WB_REF_WS_AIR_MX_WT_IDN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_MX_WT_IDN_DEL';
P_ACTION_DATE date:=sysdate();
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


    P_ACTION_NAME:=P_ACTION_NAME||P_U_NAME||P_U_IP;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID_EX (ID,
                                     ACTION_NAME,
                                       ACTION_DATE)
    select distinct f.id,
                      P_ACTION_NAME,
                        P_ACTION_DATE
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_MAX_WGHT_IDN_HS(ID_,
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
                                                                    TABLE_NAME,
	                                                                    CONDITION,
	                                                                      TYPE,
	                                                                        TYPE_FROM,
	                                                                          TYPE_TO,
	                                                                            U_NAME,
		                                                                            U_IP,
		                                                                              U_HOST_NAME,
		                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_MAX_W_IDN_HS.nextval,
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
                                 i.TABLE_NAME,
                                   i.CONDITION,
	                                   i.TYPE,
	                                     i.TYPE_FROM,
	                                       i.TYPE_TO,
		                                       i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                             i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_MAX_WGHT_IDN i
    on i.id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_MAX_WGHT_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_MAX_WGHT_IDN.id and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_MAX_WGHT_USE_H (ID_,
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
		                                                                IDN,
	                                                                    USE_ITEM_ID,
	                                                                      OPER_ID_1,
	                                                                        VALUE_1,
	                                                                          OPER_ID_2,
	                                                                            VALUE_2,
	                                                                              U_NAME,
	                                                                                U_IP,
	                                                                                  U_HOST_NAME,
	                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_MX_WT_USE_H.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         d.id,
                           d.ID_AC,
	                           d.ID_WS,
		                           d.ID_BORT,
		                             d.IDN,
                                   d.USE_ITEM_ID,
	                                   d.OPER_ID_1,
	                                     d.VALUE_1,
	                                       d.OPER_ID_2,
	                                         d.VALUE_2,
                                             d.U_NAME,
		                               	           d.U_IP,
		                                	           d.U_HOST_NAME,
		                                 	             d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_MAX_WGHT_USE d
    on d.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

     delete from WB_REF_WS_AIR_MAX_WGHT_USE
     where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_MAX_WGHT_USE.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_MAX_WGHT_T_H(ID_,
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
                                                                 IDN,
                                                                	 MAX_RMP_WEIGHT,
                                                                 	   MAX_ZF_WEIGHT,
                                                                	     MAX_TO_WEIGHT,
	                                                                       MAX_LND_WEIGHT,
	                                                                         IS_DEFAULT,
	                                                                           REMARK,
	                                                                             U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_MAX_W_T_H.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         d.id,
                           d.ID_AC,
	                           d.ID_WS,
		                           d.ID_BORT,
		                             d.IDN,
                                   d.MAX_RMP_WEIGHT,
                                     d.MAX_ZF_WEIGHT,
                                       d.MAX_TO_WEIGHT,
	                                       d.MAX_LND_WEIGHT,
	                                         d.IS_DEFAULT,
	                                           d.REMARK,
                                               d.U_NAME,
		                               	             d.U_IP,
		                                	             d.U_HOST_NAME,
		                                 	               d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_MAX_WGHT_T d
    on d.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_MAX_WGHT_T
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_MAX_WGHT_T.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_MX_WT_IDN_DEL;
/
