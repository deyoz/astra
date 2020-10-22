create or replace procedure SP_WB_REF_WS_AIR_EQUIP_IDN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_EQUIP_IDN_DEL';
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


    insert into WB_REF_WS_AIR_EQUIP_IDN_HST(ID_,
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
		                                                                U_NAME,
		                                                                  U_IP,
		                                                                    U_HOST_NAME,
		                                                                      DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQUIP_IDN_HS.nextval,
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
		                               i.U_NAME,
		                                 i.U_IP,
		                                   i.U_HOST_NAME,
		                                     i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_EQUIP_IDN i
    on i.id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_EQUIP_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_EQUIP_IDN.id and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_ADV_HST(ID_,
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
	                                                                  PWL_BALANCE_ARM,
	                                                                    PWL_INDEX_UNIT,
	                                                                      GOL_BALANCE_ARM,
	                                                                        GOL_INDEX_UNIT,	
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE,
                                                                                      DATE_FROM)
    select SEC_WB_REF_WS_AIR_EQUIP_ADV_HS.nextval,
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
		                             i.IDN,
	                                 i.PWL_BALANCE_ARM,
	                                   i.PWL_INDEX_UNIT,
	                                     i.GOL_BALANCE_ARM,
	                                       i.GOL_INDEX_UNIT,	
		                         	           i.U_NAME,
		                               	         i.U_IP,
		                                	         i.U_HOST_NAME,
		                                 	           i.DATE_WRITE,
                                                       i.DATE_FROM
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_EQUIP_ADV i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_EQUIP_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_EQUIP_ADV.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_PWL_HST(ID_,
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
                                                                    ADV_ID,
                                                                      TANK_NAME,
	                                                                      MAX_WEIGHT,
	                                                                        L_CENTROID,
	                                                                          BA_CENTROID,
	                                                                            BA_FWD,
	                                                                              BA_AFT,
	                                                                                INDEX_PER_WT_UNIT,
	                                                                                  SHOW_ON_PLAN,
		                                                                                  U_NAME,
		                                                                                    U_IP,
		                                                                                      U_HOST_NAME,
		                                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQUIP_ADV_HS.nextval,
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
		                             i.IDN,
	                                 i.ADV_ID,
	                                   i.TANK_NAME,
	                                     i.MAX_WEIGHT,
	                                       i.L_CENTROID,
	                                         i.BA_CENTROID,
	                                           i.BA_FWD,
	                                             i.BA_AFT,
	                                               i.INDEX_PER_WT_UNIT,
	                                                 i.SHOW_ON_PLAN,	
		                         	                       i.U_NAME,
		                               	                   i.U_IP,
		                                	                   i.U_HOST_NAME,
		                                 	                     i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_EQUIP_PWL i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_EQUIP_PWL
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_EQUIP_PWL.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_BORT_HS(ID_,
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
                                                                    ADV_ID,
                                                                      U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_BORT_HST.nextval,
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
		                             i.IDN,
	                                 i.ADV_ID,
		                         	       i.U_NAME,
		                               	   i.U_IP,
		                                	   i.U_HOST_NAME,
		                                 	     i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_EQUIP_BORT i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_EQUIP_BORT
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_EQUIP_BORT.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_GOL_HST(ID_,
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
                                                                    ADV_ID,
                                                                      TYPE_ID,
	                                                                      DECK_ID,
	                                                                        DESCRIPTION,
	                                                                          MAX_WEIGHT,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          SHOW_ON_PLAN,
		                                                                                          U_NAME,
		                                                                                            U_IP,
		                                                                                              U_HOST_NAME,
		                                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_GOL_HST.nextval,
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
		                             i.IDN,
	                                 i.ADV_ID,
	                                   i.TYPE_ID,
	                                     i.DECK_ID,
	                                       i.DESCRIPTION,
	                                         i.MAX_WEIGHT,
	                                           i.LA_CENTROID,
	                                             i.LA_FROM,
	                                               i.LA_TO,
	                                                 i.BA_CENTROID,
	                                                   i.BA_FWD,
	                                                     i.BA_AFT,
	                                                       i.INDEX_PER_WT_UNIT,
	                                                         i.SHOW_ON_PLAN,	
		                         	                               i.U_NAME,
		                               	                           i.U_IP,
		                                	                           i.U_HOST_NAME,
		                                 	                             i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_EQUIP_GOL i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_EQUIP_GOL
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_EQUIP_GOL.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_EQUIP_IDN_DEL;
/
