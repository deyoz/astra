create or replace procedure SP_WB_REF_WS_AIR_GRCHIDN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_ACTION_NAME varchar2(200):='WB_REF_WS_AIR_GR_CH_IDN_DELETE';
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


    insert into WB_REF_WS_AIR_GR_CH_IDN_HST(ID_,
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
		                                                              CH_TYPE,
		                                                                U_NAME,
		                                                                  U_IP,
		                                                                    U_HOST_NAME,
		                                                                      DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_IDN_HS.nextval,
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
                                 i.CH_TYPE,
		                               i.U_NAME,
		                                 i.U_IP,
		                                   i.U_HOST_NAME,
		                                     i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_IDN i
    on i.id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_GR_CH_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_GR_CH_IDN.id and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_GR_CH_A_L_H (ID_,
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
		                                                             ID_CH,
	                                                                 WEIGHT,
	                                                                   PROC_MAC,
	                                                                     INDX,
	                                                                       U_NAME,
	                                                                         U_IP,
	                                                                           U_HOST_NAME,
	                                                                             DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_A_L_H.nextval,
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
		                             d.ID_CH,
	                                 d.WEIGHT,
	                                   d.PROC_MAC,
	                                     d.INDX,
	                                       d.U_NAME,
		                               	       d.U_IP,
		                                	       d.U_HOST_NAME,
		                                 	         d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_A_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_A_L
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_A_L.id_ch);

    insert into WB_REF_WS_AIR_GR_CH_F_L_H (ID_,
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
		                                                             ID_CH,
	                                                                 WEIGHT,
	                                                                   PROC_MAC,
	                                                                     INDX,
	                                                                       U_NAME,
	                                                                         U_IP,
	                                                                           U_HOST_NAME,
	                                                                             DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_F_L_H.nextval,
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
		                             d.ID_CH,
	                                 d.WEIGHT,
	                                   d.PROC_MAC,
	                                     d.INDX,
	                                       d.U_NAME,
		                               	       d.U_IP,
		                                	       d.U_HOST_NAME,
		                                 	         d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_F_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_F_L
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_F_L.id_ch);
     ---------------------------------------------------------------------------
     ---------------------------------------------------------------------------
     insert into WB_REF_WS_AIR_GR_CH_ITL_L_H (ID_,
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
		                                                               ID_CH,
	                                                                   WEIGHT,
	                                                                     PROC_MAC,
	                                                                       INDX,
	                                                                         U_NAME,
	                                                                           U_IP,
	                                                                             U_HOST_NAME,
	                                                                               DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_ITL_LH.nextval,
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
		                             d.ID_CH,
	                                 d.WEIGHT,
	                                   d.PROC_MAC,
	                                     d.INDX,
	                                       d.U_NAME,
		                               	       d.U_IP,
		                                	       d.U_HOST_NAME,
		                                 	         d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_ITL_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_ITL_L
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_ITL_L.id_ch);
     ---------------------------------------------------------------------------
     ---------------------------------------------------------------------------
     insert into WB_REF_WS_AIR_GR_CH_BORT_H (ID_,
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
		                                                               ID_CH,
	                                                                   U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_BORTH.nextval,
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
		                             d.ID_CH,	
	                                 d.U_NAME,
		                               	 d.U_IP,
		                                	 d.U_HOST_NAME,
		                                 	   d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_BORT d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_BORT
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_BORT.id_ch);
     ---------------------------------------------------------------------------
     ---------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_GR_CH_L_L_H (ID_,
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
		                                                             ID_CH,
	                                                                 WEIGHT,
	                                                                   PROC_MAC,
	                                                                     INDX,
	                                                                       U_NAME,
	                                                                         U_IP,
	                                                                           U_HOST_NAME,
	                                                                             DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_L_L_H.nextval,
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
		                             d.ID_CH,
	                                 d.WEIGHT,
	                                   d.PROC_MAC,
	                                     d.INDX,
	                                       d.U_NAME,
		                               	       d.U_IP,
		                                	       d.U_HOST_NAME,
		                                 	         d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_L_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_L_L
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_L_L.id_ch);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    insert into WB_REF_WS_AIR_GR_CH_USE_H (ID_,
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
		                                                             ID_CH,
	                                                                 USE_ITEM_ID,
	                                                                   OPER_ID_1,
	                                                                     VALUE_1,
	                                                                       OPER_ID_2,
	                                                                         VALUE_2,
	                                                                           U_NAME,
	                                                                             U_IP,
	                                                                               U_HOST_NAME,
	                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_USE_H.nextval,
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
		                             d.ID_CH,
                                   d.USE_ITEM_ID,
	                                   d.OPER_ID_1,
	                                     d.VALUE_1,
	                                       d.OPER_ID_2,
	                                         d.VALUE_2,
                                             d.U_NAME,
		                               	           d.U_IP,
		                                	           d.U_HOST_NAME,
		                                 	             d.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_GR_CH_USE d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_USE
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.IDN=t.id and
                    t.ACTION_NAME=P_ACTION_NAME and
                    t.ACTION_DATE=P_ACTION_DATE and
                    i.id=WB_REF_WS_AIR_GR_CH_USE.id_ch);
   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
   insert into WB_REF_WS_AIR_GR_CH_ADV_HST(ID_,
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
	                                                                  CH_INDEX,
	                                                                    CH_PROC_MAC,
	                                                                      CH_CERTIFIED,
	                                                                        CH_CURTAILED,
	                                                                          TABLE_NAME,
	                                                                            CONDITION,
	                                                                              TYPE,
	                                                                                TYPE_FROM,
	                                                                                  TYPE_TO,
	                                                                                    REMARK_1,
	                                                                                      REMARK_2,
		                                                                                      U_NAME,
		                                                                                        U_IP,
		                                                                                          U_HOST_NAME,
		                                                                                            DATE_WRITE)
     select SEC_WB_REF_WS_AIR_GR_CH_ADV_HS.nextval,
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
                                 i.IDN,
	                                 i.CH_INDEX,
	                                   i.CH_PROC_MAC,
	                                     i.CH_CERTIFIED,
	                                       i.CH_CURTAILED,
	                                         i.TABLE_NAME,
	                                           i.CONDITION,
	                                             i.TYPE,
	                                               i.TYPE_FROM,
	                                                 i.TYPE_TO,
	                                                   i.REMARK_1,
	                                                     i.REMARK_2,
		                                                     i.U_NAME,
		                                                       i.U_IP,
		                                                         i.U_HOST_NAME,
		                                                           i.DATE_WRITE
     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE;

     delete from WB_REF_WS_AIR_GR_CH_ADV
     where exists(select 1
                  from WB_TEMP_XML_ID_EX t
                  where t.id=WB_REF_WS_AIR_GR_CH_ADV.IDN and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    update WB_REF_WS_AIR_GR_CH_REL
    set IDN_REL=-1,
        U_NAME=P_U_NAME,
        U_IP=P_U_IP,
        U_HOST_NAME=P_U_HOST_NAME,
        DATE_WRITE=SYSDATE()
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_GR_CH_REL.IDN_REL and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);

    insert into WB_REF_WS_AIR_GR_CH_REL_H(ID_,
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
	                                                                IDN_REL,
		                                                                U_NAME,
		                                                                  U_IP,
		                                                                    U_HOST_NAME,
		                                                                      DATE_WRITE)
     select SEC_WB_REF_WS_AIR_GR_CH_REL_H.nextval,
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
                                 i.IDN,
	                                 i.IDN_REL,
		                                 i.U_NAME,
		                                   i.U_IP,
		                                     i.U_HOST_NAME,
		                                       i.DATE_WRITE
     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_REL i
     on i.IDN=t.id and
        t.ACTION_NAME=P_ACTION_NAME and
        t.ACTION_DATE=P_ACTION_DATE;

     delete from WB_REF_WS_AIR_GR_CH_REL
     where exists(select 1
                  from WB_TEMP_XML_ID_EX t
                  where t.id=WB_REF_WS_AIR_GR_CH_REL.IDN and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_GRCHIDN_DEL;
/
