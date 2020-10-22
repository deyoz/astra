create or replace procedure SP_WB_REF_AC_WS_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_INT_TRAN number:=0;
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/optons[1]/P_INT_TRAN[1]')) into P_INT_TRAN from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id_ac,
                      f.id_ws
    from (select to_number(extractValue(value(t),'list/ID_AC[1]')) as id_ac,
                 to_number(extractValue(value(t),'list/ID_WS[1]')) as id_ws
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ---------------------------ìÑÄãüÖå óíé çÄÑé Ç ÑÄççõï Ää---------------------
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_TYPE
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_TYPE.id_ac and
                       t.num=WB_REF_WS_AIR_TYPE.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_MEASUREMENT_HIST (ID_,
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
		                                                                  WEIGHT_ID_OLD,
		                                                                    VOLUME_ID_LIQUID_OLD,
		                                                                      VOLUME_ID_OLD,
		                                                                        LENGTH_ID_OLD,
		                                                                          DENSITY_ID_FUEL_OLD,
		                                                                            MOMENTS_ID_OLD,
		                                                                              REMARK_OLD,
		                                                                                U_NAME_OLD,
		                                                                                  U_IP_OLD,
		                                                                                    U_HOST_NAME_OLD,
		                                                                                      DATE_WRITE_OLD,
		                                                                                        ID_AC_NEW,
		                                                                                          ID_WS_NEW,
		                                                                                            ID_BORT_NEW,
		                                                                                              DATE_FROM_NEW,
		                                                                                                WEIGHT_ID_NEW,
		                                                                                                  VOLUME_ID_LIQUID_NEW,
		                                                                                                    VOLUME_ID_NEW,
		                                                                                                      LENGTH_ID_NEW,
		                                                                                                        DENSITY_ID_FUEL_NEW,
		                                                                                                          MOMENTS_ID_NEW,
		                                                                                                            REMARK_NEW,
		                                                                                                              U_NAME_NEW,
		                                                                                                                U_IP_NEW,
		                                                                                                                  U_HOST_NAME_NEW,
		                                                                                                                    DATE_WRITE_NEW)
    select SEC_WB_REF_WS_AIR_MEAS_HIST.nextval,
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
		                             i.WEIGHT_ID,
		                               i.VOLUME_ID_LIQUID,
		                                 i.VOLUME_ID,
		                                   i.LENGTH_ID,
		                                     i.DENSITY_ID_FUEL,
		                                       i.MOMENTS_ID,
		                                         i.REMARK,
		                                           i.U_NAME,
		                                             i.U_IP,
		                                               i.U_HOST_NAME,
		                                                 i.DATE_WRITE,
		                                                   i.ID_AC,
		                                                     i.ID_WS,
		                                                       i.ID_BORT,
		                                                         i.DATE_FROM,
		                                                           i.WEIGHT_ID,
		                                                             i.VOLUME_ID_LIQUID,
		                                                               i.VOLUME_ID,
		                                                                 i.LENGTH_ID,
		                                                                   i.DENSITY_ID_FUEL,
		                                                                     i.MOMENTS_ID,
		                                                                       i.REMARK,
		                                                                         i.U_NAME,
		                                                                           i.U_IP,
		                                                                             i.U_HOST_NAME,
		                                                                               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MEASUREMENT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_MEASUREMENT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_MEASUREMENT.id_ac and
                       t.num=WB_REF_WS_AIR_MEASUREMENT.id_ws);
   -----------------------------------------------------------------------------
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_BAS_IND_FORM
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_BAS_IND_FORM.id_ac and
                       t.num=WB_REF_WS_AIR_BAS_IND_FORM.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_ATO_HIST (ID_,
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
		                                                          SORT_CONTAINER_LOADING_OLD,
	                                                              EZFW_THRESHOLD_LIMIT_OLD,
	                                                                EZFW_THRESHOLD_LIMIT_VAL_OLD,
	                                                                  CHECK_LATERAL_I_LIMITS_OLD,
	                                                                    PTO_CLASS_TRIM_OLD,
	                                                                      PTO_CABIN_AREA_TRIM_OLD,
	                                                                        PTO_SEAT_ROW_TRIM_OLD,
	                                                                          REMARK_OLD,
		                                                                          U_NAME_OLD,
		                                                                            U_IP_OLD,
		                                                                              U_HOST_NAME_OLD,
		                                                                                DATE_WRITE_OLD,
		                                                                                  ID_AC_NEW,
		                                                                                    ID_WS_NEW,
		                                                                                      ID_BORT_NEW,
		                                                                                        DATE_FROM_NEW,
		                                                                                          SORT_CONTAINER_LOADING_NEW,
	                                                                                              EZFW_THRESHOLD_LIMIT_NEW,
	                                                                                                EZFW_THRESHOLD_LIMIT_VAL_NEW,
	                                                                                                  CHECK_LATERAL_I_LIMITS_NEW,
	                                                                                                    PTO_CLASS_TRIM_NEW,
	                                                                                                      PTO_CABIN_AREA_TRIM_NEW,
	                                                                                                        PTO_SEAT_ROW_TRIM_NEW,
	                                                                                                          REMARK_NEW,
		                                                                                                          U_NAME_NEW,
		                                                                                                            U_IP_NEW,
		                                                                                                              U_HOST_NAME_NEW,
		                                                                                                                DATE_WRITE_NEW)
    select SEC_WB_REF_WS_AIR_ATO_HIST.nextval,
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
		                             i.SORT_CONTAINER_LOADING,
	                                 i.EZFW_THRESHOLD_LIMIT,
	                                   i.EZFW_THRESHOLD_LIMIT_VAL,
	                                     i.CHECK_LATERAL_IMBALANCE_LIMITS,
	                                       i.PTO_CLASS_TRIM,
	                                         i.PTO_CABIN_AREA_TRIM,
	                                           i.PTO_SEAT_ROW_TRIM,
	                                             i.REMARK,
		                                             i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                                   i.DATE_WRITE,
		                                                     i.ID_AC,
		                                                       i.ID_WS,
		                                                         i.ID_BORT,
		                                                           i.DATE_FROM,
		                                                             i.SORT_CONTAINER_LOADING,
	                                                                 i.EZFW_THRESHOLD_LIMIT,
	                                                                   i.EZFW_THRESHOLD_LIMIT_VAL,
	                                                                     i.CHECK_LATERAL_IMBALANCE_LIMITS,
	                                                                       i.PTO_CLASS_TRIM,
	                                                                         i.PTO_CABIN_AREA_TRIM,
	                                                                           i.PTO_SEAT_ROW_TRIM,
	                                                                             i.REMARK,
		                                                                             i.U_NAME,
		                                                                               i.U_IP,
		                                                                                 i.U_HOST_NAME,
		                                                                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_ATO i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_ATO
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_ATO.id_ac and
                       t.num=WB_REF_WS_AIR_ATO.id_ws);
    ---------------------------ìÑÄãüÖå óíé çÄÑé Ç ÑÄççõï Ää---------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_WS_TYPES_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
                                                          ID_AC_OLD,
	                                                          ID_WS_OLD,
	                                                            U_NAME_OLD,
	                                                              U_IP_OLD,
                                                      	          U_HOST_NAME_OLD,
	                                                                  DATE_WRITE_OLD,
                                                                      ID_AC_NEW,
	                                                                      ID_WS_NEW,
	                                                                        U_NAME_NEW,
	                                                                          U_IP_NEW,
	                                                                            U_HOST_NAME_NEW,
	                                                                              DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_WS_TYPES_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ID_WS,
	                           i.U_NAME,
	                             i.U_IP,
	                               i.U_HOST_NAME,
	                                 i.DATE_WRITE,
                                     i.ID_AC,
	                                     i.ID_WS,
	                                       i.U_NAME,
	                                         i.U_IP,
	                                           i.U_HOST_NAME,
	                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_WS_TYPES i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_AIRCO_WS_TYPES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_WS_TYPES.id_ac and
                       t.num=WB_REF_AIRCO_WS_TYPES.id_ws);


    insert into WB_REF_AIRCO_WS_BORTS_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
                                                          ID_AC_OLD,
	                                                          ID_WS_OLD,
                                                              BORT_NUM_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
                                                      	            U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,
                                                                        ID_AC_NEW,
	                                                                        ID_WS_NEW,
                                                                            BORT_NUM_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_WS_BORTS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ID_WS,
                             i.BORT_NUM,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,
                                       i.ID_AC,
	                                       i.ID_WS,
                                           i.BORT_NUM,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_WS_BORTS i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_AIRCO_WS_BORTS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_WS_BORTS.id_ac and
                       t.num=WB_REF_AIRCO_WS_BORTS.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_BAL_OUT_ADV_HST (ID_,
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
    select SEC_WB_REF_WS_AIR_BO_ADV_HST.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_BAL_OUTPUT_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_BAL_OUTPUT_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_BAL_OUTPUT_ADV.id_ac and
                       t.num=WB_REF_WS_AIR_BAL_OUTPUT_ADV.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_BAL_OUT_AD_I_HST(ID_,
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
    select SEC_WB_REF_WS_AIR_BO_ADV_I_HST.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_BAL_OUTPUT_ADV_I i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_BAL_OUTPUT_ADV_I
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_BAL_OUTPUT_ADV_I.id_ac and
                       t.num=WB_REF_WS_AIR_BAL_OUTPUT_ADV_I.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_BAL_OUT_DATA_HST (ID_,
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
    select SEC_WB_REF_WS_AIR_BL_OT_DT_HST.nextval,
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
                                                                         i.ADV_ID,
                                                                           i.ADV_ID
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_BAL_OUTPUT_DATA i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_BAL_OUTPUT_DATA
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_BAL_OUTPUT_DATA.id_ac and
                       t.num=WB_REF_WS_AIR_BAL_OUTPUT_DATA.id_ws);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SUP_INFO_ADV_HST (ID_,
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SUPPL_INFO_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_ADV.id_ac and
                       t.num=WB_REF_WS_AIR_SUPPL_INFO_ADV.id_ws);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SUPPL_INFO_ADV_I
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_ADV_I.id_ac and
                       t.num=WB_REF_WS_AIR_SUPPL_INFO_ADV_I.id_ws);
    ----------------------------------------------------------------------------
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
                                                                         i.ADV_ID,
                                                                           i.ADV_ID
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SUPPL_INFO_DATA i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SUPPL_INFO_DATA
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_SUPPL_INFO_DATA.id_ac and
                       t.num=WB_REF_WS_AIR_SUPPL_INFO_DATA.id_ws);
    ----------------------------------------------------------------------------
    ------------------------------------------------------------------------------------------------------------------------
    ----------------------------------------------------------------------------------------------------GRAVITY CHARTS-START
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_GR_CH_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_GR_CH_IDN.id_ac and
                       t.num=WB_REF_WS_AIR_GR_CH_IDN.id_ws);

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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num join WB_REF_WS_AIR_GR_CH_A_L d
       on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_A_L
    where exists(select 1
                 from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.id_ac=t.id and
                    i.id_ws=t.num and
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num join WB_REF_WS_AIR_GR_CH_F_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_F_L
    where exists(select 1
                 from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.id_ac=t.id and
                    i.id_ws=t.num and
                    i.id=WB_REF_WS_AIR_GR_CH_F_L.id_ch);

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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num join WB_REF_WS_AIR_GR_CH_ITL_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_ITL_L
    where exists(select 1
                 from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.id_ac=t.id and
                    i.id_ws=t.num and
                    i.id=WB_REF_WS_AIR_GR_CH_ITL_L.id_ch);

    ----
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num join WB_REF_WS_AIR_GR_CH_L_L d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_L_L
    where exists(select 1
                 from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.id_ac=t.id and
                    i.id_ws=t.num and
                    i.id=WB_REF_WS_AIR_GR_CH_L_L.id_ch);

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num join WB_REF_WS_AIR_GR_CH_BORT d
        on d.id_ch=i.id;

    delete from WB_REF_WS_AIR_GR_CH_BORT
    where exists(select 1
                 from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
                 on i.id_ac=t.id and
                    i.id_ws=t.num and
                    i.id=WB_REF_WS_AIR_GR_CH_BORT.id_ch);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_GR_CH_CND_H (ID_,
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
	                                                                 CND_NAME_ID,
	                                                                   OPER_ID_1,
	                                                                     VALUE_1,
	                                                                       OPER_ID_2,
	                                                                         VALUE_2,
	                                                                           U_NAME,
	                                                                             U_IP,
	                                                                               U_HOST_NAME,
	                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_CND_H.nextval,
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
	                                 i.CND_NAME_ID,
	                                   i.OPER_ID_1,
	                                     i.VALUE_1,
	                                       i.OPER_ID_2,
	                                         i.VALUE_2,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_CND i
     on i.id_ac=t.id and
        i.id_ws=t.num;

     delete from WB_REF_WS_AIR_GR_CH_CND
     where exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id=WB_REF_WS_AIR_GR_CH_CND.id_ac and
                        t.num=WB_REF_WS_AIR_GR_CH_CND.id_ws);
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_USE d
     on d.id_ac=t.id and
        d.id_ws=t.num;

     delete from WB_REF_WS_AIR_GR_CH_USE
     where exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id=WB_REF_WS_AIR_GR_CH_USE.id_ac and
                        t.num=WB_REF_WS_AIR_GR_CH_USE.id_ws);
    ----------------------------------------------------------------------------
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
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num;

     delete from  WB_REF_WS_AIR_GR_CH_ADV
     where exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id= WB_REF_WS_AIR_GR_CH_ADV.id_ac and
                        t.num= WB_REF_WS_AIR_GR_CH_ADV.id_ws);
     ---------------------------------------------------------------------------

    update WB_REF_WS_AIR_GR_CH_REL
    set IDN_REL=-1,
        U_NAME=P_U_NAME,
        U_IP=P_U_IP,
        U_HOST_NAME=P_U_HOST_NAME,
        DATE_WRITE=SYSDATE()
    where IDN_REL in (select d.id
                      from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_ADV d
                      on d.id_ac=t.id and
                         d.id_ws=t.num);

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
                         d.id,
                           d.ID_AC,
	                           d.ID_WS,
		                           d.ID_BORT,		
	                               d.IDN,
	                                 d.IDN_REL,	
	                                   d.U_NAME,
		                             	     d.U_IP,
		                               	     d.U_HOST_NAME,
		                               	       d.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_REL d
     on d.id_ac=t.id and
        d.id_ws=t.num;

    delete from WB_REF_WS_AIR_GR_CH_REL
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_GR_CH_REL.id_ac=t.id and
                       WB_REF_WS_AIR_GR_CH_REL.id_ws=t.num);


    ---------------------------------------------------------------------------
    ----------------------------------------------------------------------------------------------------GRAVITY CHARTS-END
    -----------------------------------------------------------------------------------------------------------------------
    ------------------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------CABIN-START
    insert into WB_REF_WS_AIR_CABIN_IDN_HST(ID_,
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
    select SEC_WB_REF_WS_AIR_CABIN_IDN_HS.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CABIN_IDN i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CABIN_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CABIN_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_CABIN_IDN.id_ws=t.num);

    insert into WB_REF_WS_AIR_CABIN_ADV_HST(ID_,
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
	                                                                  CD_BALANCE_ARM,
	                                                                    CD_INDEX_UNIT,
	                                                                      FDL_BALANCE_ARM,
	                                                                        FDL_INDEX_UNIT,
	                                                                          CCL_BALANCE_ARM,
	                                                                            CCL_INDEX_UNIT,
		                                                                            U_NAME,
		                                                                              U_IP,
		                                                                                U_HOST_NAME,
		                                                                                  DATE_WRITE,
                                                                                        DATE_FROM)
    select SEC_WB_REF_WS_AIR_CABIN_ADV_HS.nextval,
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
	                                 i.CD_BALANCE_ARM,
	                                   i.CD_INDEX_UNIT,
	                                     i.FDL_BALANCE_ARM,
	                                       i.FDL_INDEX_UNIT,
	                                         i.CCL_BALANCE_ARM,
	                                           i.CCL_INDEX_UNIT,
		                         	                 i.U_NAME,
		                               	             i.U_IP,
		                                	             i.U_HOST_NAME,
		                                 	               i.DATE_WRITE,
                                                       i.DATE_FROM
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CABIN_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CABIN_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CABIN_ADV.id_ac=t.id and
                       WB_REF_WS_AIR_CABIN_ADV.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_CD_HST(ID_,
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
                                                                     DECK_ID,
	                                                                     SECTION,
	                                                                       ROWS_FROM,
	                                                                         ROWS_TO,
	                                                                           LA_FROM,
	                                                                             LA_TO,
	                                                                               BA_CENTROID,
	                                                                                 BA_FWD,
	                                                                                   BA_AFT,
	                                                                                     INDEX_PER_WT_UNIT,
		                                                                                     U_NAME,
		                                                                                       U_IP,
		                                                                                         U_HOST_NAME,
		                                                                                           DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_CD_HST.nextval,
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
                                     i.DECK_ID,
	                                     i.SECTION,
	                                       i.ROWS_FROM,
	                                         i.ROWS_TO,
	                                           i.LA_FROM,
	                                             i.LA_TO,
	                                               i.BA_CENTROID,
	                                                 i.BA_FWD,
	                                                   i.BA_AFT,
	                                                     i.INDEX_PER_WT_UNIT,
		                         	                           i.U_NAME,
		                               	                       i.U_IP,
		                                	                       i.U_HOST_NAME,
		                                 	                         i.DATE_WRITE
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CABIN_CD i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CABIN_CD
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CABIN_CD.id_ac=t.id and
                       WB_REF_WS_AIR_CABIN_CD.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
                                                                     FCL_ID,
	                                                                     MAX_NUM_SEATS,
	                                                                       LA_CENTROID,
	                                                                         BA_CENTROID,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
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
                                     i.FCL_ID,
	                                     i.MAX_NUM_SEATS,
	                                       i.LA_CENTROID,
	                                         i.BA_CENTROID,
	                                           i.INDEX_PER_WT_UNIT,
		                         	                 i.U_NAME,
		                               	             i.U_IP,
		                                	             i.U_HOST_NAME,
		                                 	               i.DATE_WRITE
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CABIN_FD i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CABIN_FD
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CABIN_FD.id_ac=t.id and
                       WB_REF_WS_AIR_CABIN_FD.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_CCL_HST(ID_,
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
                                                                     DECK_ID,
                                                                       LOCATION,
	                                                                       MAX_NUM_SEATS,
	                                                                         LA_CENTROID,
	                                                                           BA_CENTROID,
	                                                                             INDEX_PER_WT_UNIT,
		                                                                             U_NAME,
		                                                                               U_IP,
		                                                                                 U_HOST_NAME,
		                                                                                   DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CBN_CCL_HST.nextval,
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
                                     i.DECK_ID,
                                       i.LOCATION,
	                                       i.MAX_NUM_SEATS,
	                                         i.LA_CENTROID,
	                                           i.BA_CENTROID,
	                                             i.INDEX_PER_WT_UNIT,
		                         	                   i.U_NAME,
		                               	               i.U_IP,
		                                	               i.U_HOST_NAME,
		                                 	                 i.DATE_WRITE
     from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CABIN_CCL i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CABIN_CCL
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CABIN_CCL.id_ac=t.id and
                       WB_REF_WS_AIR_CABIN_CCL.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    ---------------------------------------------------------------------------------------------------------------CABIN-END
    ------------------------------------------------------------------------------------------------------------------------



    ------------------------------------------------------------------------------------------------------------------------
    ---------------------------------------------------------------------------------------------------------EQUIPMRNT-BEGIN
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_IDN i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_EQUIP_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_EQUIP_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_EQUIP_IDN.id_ws=t.num);

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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_EQUIP_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_EQUIP_ADV.id_ac=t.id and
                       WB_REF_WS_AIR_EQUIP_ADV.id_ws=t.num);
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_PWL i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_EQUIP_PWL
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_EQUIP_PWL.id_ac=t.id and
                       WB_REF_WS_AIR_EQUIP_PWL.id_ws=t.num);
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_BORT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_EQUIP_BORT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_EQUIP_BORT.id_ac=t.id and
                       WB_REF_WS_AIR_EQUIP_BORT.id_ws=t.num);
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_GOL i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_EQUIP_GOL
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_EQUIP_GOL.id_ac=t.id and
                       WB_REF_WS_AIR_EQUIP_GOL.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------EQUIPMRNT-END
    ------------------------------------------------------------------------------------------------------------------------
    ------------------------------------------------------------------------------------------------------------------------
    ----------------------------------------------------------------------------------------------Combined Fuel Vector-BEGIN
    insert into WB_REF_WS_AIR_CFV_IDN_HST(ID_,
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
    select SEC_WB_REF_WS_AIR_CFV_IDN_HIST.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CFV_IDN i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CFV_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CFV_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_CFV_IDN.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CFV_ADV_HST(ID_,
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
	                                                                 PROC_NAME,
	                                                                   REMARKS,
	                                                                     DENSITY,
	                                                                       MAX_VOLUME,
	                                                                         MAX_WEIGHT,
	                                                                           CH_WEIGHT,
	                                                                             CH_VOLUME,
	                                                                               CH_BALANCE_ARM,
	                                                                                 CH_INDEX_UNIT,
	                                                                                   CH_STANDART,
	                                                                                     CH_NON_STANDART,
	                                                                                       CH_USE_BY_DEFAULT,
		                                                                                       U_NAME,
		                                                                                         U_IP,
		                                                                                           U_HOST_NAME,
		                                                                                             DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_ADV_HIST.nextval,
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
	                                 i.PROC_NAME,
	                                   i.REMARKS,
	                                     i.DENSITY,
	                                       i.MAX_VOLUME,
	                                         i.MAX_WEIGHT,
	                                           i.CH_WEIGHT,
	                                             i.CH_VOLUME,
	                                               i.CH_BALANCE_ARM,
	                                                 i.CH_INDEX_UNIT,
	                                                   i.CH_STANDART,
	                                                     i.CH_NON_STANDART,
	                                                       i.CH_USE_BY_DEFAULT,
		                                                       i.U_NAME,
		                                                         i.U_IP,
		                                                           i.U_HOST_NAME,
		                                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CFV_ADV i
     on i.id_ac=t.id and
        i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CFV_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CFV_ADV.id_ac=t.id and
                       WB_REF_WS_AIR_CFV_ADV.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CFV_TBL_HST(ID_,
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
                                                                    WEIGHT,
	                                                                    VOLUME,
	                                                                      ARM,
	                                                                        INDEX_UNIT,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_TBL_HST.nextval,
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
	                                   i.WEIGHT,
	                                     i.VOLUME,
	                                       i.ARM,
	                                         i.INDEX_UNIT,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
                                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CFV_TBL i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CFV_TBL
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CFV_TBL.id_ac=t.id and
                       WB_REF_WS_AIR_CFV_TBL.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CFV_BORT_HST(ID_,
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
    select SEC_WB_REF_WS_AIR_CFV_BORT_HST.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CFV_BORT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_CFV_BORT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_CFV_BORT.id_ac=t.id and
                       WB_REF_WS_AIR_CFV_BORT.id_ws=t.num);
    ------------------------------------------------------------------------------------------------Combined Fuel Vector-END
    ------------------------------------------------------------------------------------------------------------------------
    ------------------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------Holds-BEGIN
    insert into WB_REF_WS_AIR_HLD_DECK_HST(ID_,
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
                                                                 DECK_ID,
	                                                                 MAX_WEIGHT,
	                                                                   LA_FROM,
	                                                                     LA_TO,
	                                                                       BA_FWD,
	                                                                         BA_AFT,
	                                                                           U_NAME,
		                                                                           U_IP,
		                                                                             U_HOST_NAME,
		                                                                               DATE_WRITE,
                                                                                     MAX_VOLUME)
    select SEC_WB_REF_WS_AIR_HLD_DECK_HST.nextval,
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
                                 i.DECK_ID,	
	                                 i.MAX_WEIGHT,
	                                   i.LA_FROM,
	                                     i.LA_TO,
	                                       i.BA_FWD,
	                                         i.BA_AFT,
	                                           i.U_NAME,
		                               	           i.U_IP,
		                                	           i.U_HOST_NAME,
		                                 	             i.DATE_WRITE,
                                                     i.MAX_VOLUME
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_DECK i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_DECK
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_DECK.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_DECK.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_HLD_DCK_REM_HST(ID_,
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
                                                                    REMARKS,
	                                                                    U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_DCKRMHST.nextval,
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
                                 i.REMARKS,
                                   i.U_NAME,
		                               	 i.U_IP,
		                                	 i.U_HOST_NAME,
		                                 	   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_DECK_REM i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_DECK_REM
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_DECK_REM.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_DECK_REM.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_HLD_CMP_A_HST(ID_,
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
                                                                  CH_BALANCE_ARM,
	                                                                  CH_INDEX_UNIT,
	                                                                    REMARKS,
	                                                                      U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_CMP_A_HS.nextval,
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
                                 i.CH_BALANCE_ARM,
	                                 i.CH_INDEX_UNIT,
                                     i.REMARKS,
                                       i.U_NAME,
		                                  	 i.U_IP,
		                                	     i.U_HOST_NAME,
		                                 	       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_CMP_A i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_CMP_A
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_CMP_A.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_CMP_A.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_HLD_HLD_A_HST(ID_,
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
                                                                  CH_BALANCE_ARM,
	                                                                  CH_INDEX_UNIT,
	                                                                    REMARKS,
	                                                                      U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_HLD_A_HS.nextval,
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
                                 i.CH_BALANCE_ARM,
	                                 i.CH_INDEX_UNIT,
                                     i.REMARKS,
                                       i.U_NAME,
		                                  	 i.U_IP,
		                                	     i.U_HOST_NAME,
		                                 	       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_HLD_A i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_HLD_A
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_HLD_A.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_HLD_A.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
   insert into WB_REF_WS_AIR_HLD_CMP_T_HST(ID_,
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
                                                                 HOLD_ID,
	                                                                 CMP_NAME,
	                                                                   MAX_WEIGHT,
	                                                                     MAX_VOLUME,
	                                                                       LA_CENTROID,
	                                                                         LA_FROM,
	                                                                           LA_TO,
	                                                                             BA_CENTROID,
	                                                                               BA_FWD,
	                                                                                 BA_AFT,
	                                                                                   INDEX_PER_WT_UNIT,
	                                                                                     U_NAME,
		                                                                                     U_IP,
		                                                                                       U_HOST_NAME,
		                                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_CMP_T_HS.nextval,
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
                                 i.HOLD_ID,
	                                 i.CMP_NAME,
	                                   i.MAX_WEIGHT,
	                                     i.MAX_VOLUME,
	                                       i.LA_CENTROID,
	                                         i.LA_FROM,
	                                           i.LA_TO,
	                                             i.BA_CENTROID,
	                                               i.BA_FWD,
	                                                 i.BA_AFT,
	                                                   i.INDEX_PER_WT_UNIT,
                                                       i.U_NAME,
		                                  	                 i.U_IP,
		                                	                     i.U_HOST_NAME,
		                                 	                       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_CMP_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_CMP_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_CMP_T.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_CMP_T.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_HLD_HLD_T_HST(ID_,
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
                                                                 DECK_ID,
	                                                                 HOLD_NAME,
	                                                                   MAX_WEIGHT,
	                                                                     MAX_VOLUME,
	                                                                       LA_CENTROID,
	                                                                         LA_FROM,
	                                                                           LA_TO,
	                                                                             BA_CENTROID,
	                                                                               BA_FWD,
	                                                                                 BA_AFT,
	                                                                                   INDEX_PER_WT_UNIT,
	                                                                                     U_NAME,
		                                                                                     U_IP,
		                                                                                       U_HOST_NAME,
		                                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_HLD_T_HS.nextval,
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
                                 i.DECK_ID,
	                                 i.HOLD_NAME,
	                                   i.MAX_WEIGHT,
	                                     i.MAX_VOLUME,
	                                       i.LA_CENTROID,
	                                         i.LA_FROM,
	                                           i.LA_TO,
	                                             i.BA_CENTROID,
	                                               i.BA_FWD,
	                                                 i.BA_AFT,
	                                                   i.INDEX_PER_WT_UNIT,
                                                       i.U_NAME,
		                                  	                 i.U_IP,
		                                	                     i.U_HOST_NAME,
		                                 	                       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_HLD_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_HLD_HLD_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_HLD_HLD_T.id_ac=t.id and
                       WB_REF_WS_AIR_HLD_HLD_T.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SEC_BAY_A_HST(ID_,
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
                                                                  CH_BALANCE_ARM,
	                                                                  CH_INDEX_UNIT,
	                                                                    REMARKS,
	                                                                      U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SEC_BAY_A_HS.nextval,
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
                                 i.CH_BALANCE_ARM,
	                                 i.CH_INDEX_UNIT,
                                     i.REMARKS,
                                       i.U_NAME,
		                                  	 i.U_IP,
		                                	     i.U_HOST_NAME,
		                                 	       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_A i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SEC_BAY_A
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SEC_BAY_A.id_ac=t.id and
                       WB_REF_WS_AIR_SEC_BAY_A.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SEC_BAY_T_HST(ID_,
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
                                                                  CMP_NAME,
	                                                                  SEC_BAY_NAME,
	                                                                    SEC_BAY_TYPE_ID,
	                                                                      MAX_WEIGHT,
	                                                                        MAX_VOLUME,
	                                                                          LA_CENTROID,
	                                                                            LA_FROM,
	                                                                              LA_TO,
	                                                                                BA_CENTROID,
	                                                                                  BA_FWD,
	                                                                                    BA_AFT,
	                                                                                      INDEX_PER_WT_UNIT,
	                                                                                        DOOR_POSITION,
	                                                                                          U_NAME,
		                                                                                          U_IP,
		                                                                                            U_HOST_NAME,
		                                                                                              DATE_WRITE,
                                                                                                    COLOR)
    select SEC_WB_REF_WS_AIR_SEC_BAY_T_HS.nextval,
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
                                 i.CMP_NAME,
	                                 i.SEC_BAY_NAME,
	                                   i.SEC_BAY_TYPE_ID,
	                                     i.MAX_WEIGHT,
	                                       i.MAX_VOLUME,
	                                         i.LA_CENTROID,
	                                           i.LA_FROM,
	                                             i.LA_TO,
	                                               i.BA_CENTROID,
	                                                 i.BA_FWD,
	                                                   i.BA_AFT,
	                                                     i.INDEX_PER_WT_UNIT,
	                                                       i.DOOR_POSITION,
	                                                         i.U_NAME,
		                               	                         i.U_IP,
		                                	                         i.U_HOST_NAME,
		                                 	                           i.DATE_WRITE,
                                                                   i.color
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SEC_BAY_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SEC_BAY_T.id_ac=t.id and
                       WB_REF_WS_AIR_SEC_BAY_T.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
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
                                                                  T_ID,
	                                                                  ULD_IATA_ID,
	                                                                    U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
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
                                 i.T_ID,
	                                 i.ULD_IATA_ID,
                                     i.U_NAME,
		                                 	 i.U_IP,
		                                  	 i.U_HOST_NAME,
		                                 	     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_TT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SEC_BAY_TT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SEC_BAY_TT.id_ac=t.id and
                       WB_REF_WS_AIR_SEC_BAY_TT.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_ULD_OVER_HST(ID_,
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
                                                                 POSITION,
	                                                                 OVERLAY,
	                                                                   U_NAME,
		                                                                   U_IP,
		                                                                     U_HOST_NAME,
		                                                                       DATE_WRITE)
    select SEC_WB_REF_WS_AIR_ULD_OVER_HST.nextval,
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
                                 i.POSITION,
	                                 i.OVERLAY,
                                     i.U_NAME,
		                                 	 i.U_IP,
		                                  	 i.U_HOST_NAME,
		                                 	     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_ULD_OVER i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_ULD_OVER
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_ULD_OVER.id_ac=t.id and
                       WB_REF_WS_AIR_ULD_OVER.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------DOORS/NETS - DOORS
    insert into WB_REF_WS_AIR_DOORS_A_HST(ID_,
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
	                                                                REMARKS,
	                                                                  U_NAME,
		                                                                  U_IP,
		                                                                    U_HOST_NAME,
		                                                                      DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOORS_A_HS.nextval,
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
                                 i.REMARKS,
                                   i.U_NAME,
		                                 i.U_IP,
		                                	 i.U_HOST_NAME,
		                                 	   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOORS_A i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOORS_A
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOORS_A.id_ac=t.id and
                       WB_REF_WS_AIR_DOORS_A.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOORS_T_HST(ID_,
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
                                                                 HOLD_ID,
	                                                                 D_POS_ID,
	                                                                   DOOR_NAME,
	                                                                     BA_FWD,
	                                                                       BA_AFT,
	                                                                         HEIGHT,
	                                                                           WIDTH,
	                                                                             U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOORS_T_HS.nextval,
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
                                 i.HOLD_ID,
	                                 i.D_POS_ID,
	                                   i.DOOR_NAME,
	                                     i.BA_FWD,
	                                       i.BA_AFT,
	                                         i.HEIGHT,
	                                           i.WIDTH,
                                               i.U_NAME,
		                                  	         i.U_IP,
		                                	             i.U_HOST_NAME,
		                                 	               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOORS_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOORS_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOORS_T.id_ac=t.id and
                       WB_REF_WS_AIR_DOORS_T.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------



    ---------------------------------------------------------------------------------------------------------------Holds-END
    ------------------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------SEATING LAYOUT
    insert into WB_REF_WS_AIR_SEAT_LAY_IDN_HST(ID_,
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
		                                                                 U_NAME,
		                                                                   U_IP,
		                                                                     U_HOST_NAME,
		                                                                       DATE_WRITE)
    select SEC_WB_REF_WS_AIR_ST_LY_IDN_HS.nextval,
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
		                             i.U_NAME,
		                             	 i.U_IP,
		                                 i.U_HOST_NAME,
		                                 	 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEAT_LAY_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SEAT_LAY_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SEAT_LAY_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_SEAT_LAY_IDN.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SEAT_LAY_ADV_HST(ID_,
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
	                                                                     CH_BALANCE_ARM,
	                                                                       CH_INDEX_UNIT,
	                                                                         NUM_OF_SEATS,
	                                                                           CABIN_ID,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE,
                                                                                       IDN,
                                                                                         TABLE_NAME)
    select SEC_WB_REF_WS_AIR_ST_LY_ADV_HS.nextval,
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
	                                 i.CH_BALANCE_ARM,
	                                   i.CH_INDEX_UNIT,
	                                     i.NUM_OF_SEATS,
	                                       i.CABIN_ID,
		                                       i.U_NAME,
		                             	           i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE,
                                                   i.idn,
                                                     i.TABLE_NAME
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEAT_LAY_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SEAT_LAY_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SEAT_LAY_ADV.id_ac=t.id and
                       WB_REF_WS_AIR_SEAT_LAY_ADV.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_C_IDN_HST(ID_,
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
                                                                  ADV_ID,
                                                                    table_name,
		                                                                  U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_C_IDN_HS.nextval,
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
                                 i.ADV_ID,
		                               i.U_NAME,
                                     i.table_name,
		                                 	 i.U_IP,
		                                     i.U_HOST_NAME,
		                                 	     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_C_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_C_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_C_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_S_L_C_IDN.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_A_U_S_HST(ID_,
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
	                                                                    SEAT_ID,
		                                                                    U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SLAUS_HST.nextval,
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
	                                   i.SEAT_ID,
                                       i.U_NAME,
		                                   	 i.U_IP,
		                                       i.U_HOST_NAME,
		                                 	       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_A_U_S i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_A_U_S
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_A_U_S.id_ac=t.id and
                       WB_REF_WS_AIR_S_L_A_U_S.id_ws=t.num);
		----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_PLAN_HST(ID_,
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
	                                                                   CABIN_SECTION,
	                                                                     ROW_NUMBER,
	                                                                       MAX_WEIGHT,
	                                                                         MAX_SEATS,
	                                                                           BALANCE_ARM,
	                                                                             INDEX_PER_WT_UNIT,
		                                                                             U_NAME,
		                                                                               U_IP,
		                                                                                 U_HOST_NAME,
		                                                                                   DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_PLAN_HST.nextval,
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
	                                   i.CABIN_SECTION,
	                                     i.ROW_NUMBER,
	                                       i.MAX_WEIGHT,
	                                         i.MAX_SEATS,
	                                           i.BALANCE_ARM,
	                                             i.INDEX_PER_WT_UNIT,
                                                 i.U_NAME,
		                                        	     i.U_IP,
		                                                 i.U_HOST_NAME,
		                                 	                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_PLAN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_PLAN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_PLAN.id_ac=t.id and
                       WB_REF_WS_AIR_S_L_PLAN.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_P_S_HST(ID_,
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
	                                                                  PLAN_ID,
	                                                                    SEAT_ID,
	                                                                      IS_SEAT,
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE,
                                                                                  MAX_WEIGHT)
    select SEC_WB_REF_WS_AIR_S_L_P_S_HST.nextval,
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
	                                   i.PLAN_ID,
	                                     i.SEAT_ID,
	                                       i.IS_SEAT,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE,
                                                   i.MAX_WEIGHT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_P_S i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_P_S
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_P_S.id_ac=t.id and
                       WB_REF_WS_AIR_S_L_P_S.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_P_S_P_HST(ID_,
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
	                                                                    PLAN_ID,
	                                                                      S_SEAT_ID,
	                                                                        PARAM_ID,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_P_S_P_HS.nextval,
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
	                                   i.PLAN_ID,
	                                     i.S_SEAT_ID,
	                                       i.PARAM_ID,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_P_S_P i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_P_S_P
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_P_S_P.id_ac=t.id and
                       WB_REF_WS_AIR_S_L_P_S_P.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_S_L_C_ADV_HST(ID_,
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
                                                                  ADV_ID,
                                                                    IDN,
                                                                      DATE_FROM,
   	                                                                    CH_CAI_BALANCE_ARM,
   	                                                                      CH_CAI_INDEX_UNIT,
   	                                                                        CH_CI_BALANCE_ARM,
   	                                                                          CH_CI_INDEX_UNIT,
   	                                                                            CH_USE_AS_DEFAULT,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_C_ADV_HS.nextval,
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
                                 i.ADV_ID,
                                   i.IDN,
                                     i.DATE_FROM,
   	                                   i.CH_CAI_BALANCE_ARM,
   	                                     i.CH_CAI_INDEX_UNIT,
   	                                       i.CH_CI_BALANCE_ARM,
   	                                         i.CH_CI_INDEX_UNIT,
   	                                           i.CH_USE_AS_DEFAULT,
		                                             i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_S_L_C_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_S_L_C_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_S_L_C_ADV.id_ac=t.id and
                      WB_REF_WS_AIR_S_L_C_ADV.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SL_CAI_T_HST(ID_,
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
	                                                                   CABIN_SECTION,
	                                                                     BA_CENTROID,
	                                                                       BA_FWD,
	                                                                         BA_AFT,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SL_CAI_T_HST.nextval,
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
	                                   i.CABIN_SECTION,
	                                     i.BA_CENTROID,
	                                       i.BA_FWD,
	                                         i.BA_AFT,
	                                           i.INDEX_PER_WT_UNIT,
                                               i.U_NAME,
		                                        	   i.U_IP,
		                                               i.U_HOST_NAME,
		                                 	               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CAI_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SL_CAI_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SL_CAI_T.id_ac=t.id and
                       WB_REF_WS_AIR_SL_CAI_T.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SL_CAI_TT_HST(ID_,
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
	                                                                    T_ID,
	                                                                      CLASS_CODE,
	                                                                        NUM_OF_SEATS,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SL_CAI_TT_HS.nextval,
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
	                                   i.T_ID,
	                                     i.CLASS_CODE,
	                                       i.NUM_OF_SEATS,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                 	           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CAI_TT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SL_CAI_TT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SL_CAI_TT.id_ac=t.id and
                       WB_REF_WS_AIR_SL_CAI_TT.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_SL_CI_T_HST(ID_,
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
                                                                    CLASS_CODE,
	                                                                    FIRST_ROW,
	                                                                      LAST_ROW,
	                                                                        NUM_OF_SEATS,
	                                                                          LA_FROM,
	                                                                            LA_TO,	
	                                                                              BA_CENTROID,
	                                                                                BA_FWD,
	                                                                                  BA_AFT,
	                                                                                    INDEX_PER_WT_UNIT,
		                                                                                    U_NAME,
		                                                                                      U_IP,
		                                                                                        U_HOST_NAME,
		                                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SL_CI_T_HST.nextval,
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
                                     i.CLASS_CODE,
                                       i.FIRST_ROW,
	                                       i.LAST_ROW,
	                                         i.NUM_OF_SEATS,
	                                           i.LA_FROM,
	                                             i.LA_TO,
                                                 i.BA_CENTROID,
	                                                 i.BA_FWD,
	                                                   i.BA_AFT,	
	                                                     i.INDEX_PER_WT_UNIT,
		                                                     i.U_NAME,
		                                                       i.U_IP,
		                                                         i.U_HOST_NAME,
		                                                           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CI_T i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_SL_CI_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_SL_CI_T.id_ac=t.id and
                       WB_REF_WS_AIR_SL_CI_T.id_ws=t.num);
    -----------------------------------------------------------------------------------------------------------SEATING LAYOUT
    -------------------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------DOW_BUILD_UP
    insert into WB_REF_WS_AIR_DOW_ITM_HST(ID_,
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
	                                                                  ITEM_ID,
	                                                                    IS_INCLUDED,
	                                                                      IS_EXCLUDED,
	                                                                        REMARK,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_ITM_HST.nextval,
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
                                     i.ITEM_ID,
	                                     i.IS_INCLUDED,
	                                       i.IS_EXCLUDED,
	                                         i.REMARK,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
		                                               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_ITM i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_ITM
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_ITM.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_ITM.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_ADV_HST(ID_,
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
	                                                                DATE_FROM,
	                                                                  CH_BASIC_WEIGHT,
	                                                                    CH_DOW,
	                                                                      REMARK,
		                                                                      U_NAME,
		                                                                        U_IP,
		                                                                          U_HOST_NAME,
		                                                                            DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_ADV_HST.nextval,
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
                                   i.DATE_FROM,
	                                   i.CH_BASIC_WEIGHT,
	                                     i.CH_DOW,
	                                       i.REMARK,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_ADV i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_ADV.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_ADV.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_IDN_HST(ID_,
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
		                                                            U_NAME,
		                                                              U_IP,
		                                                                U_HOST_NAME,
		                                                                  DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_IDN_HST.nextval,
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
                                 i.U_NAME,
		                               i.U_IP,
		                                 i.U_HOST_NAME,
		                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_IDN.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_CR_CODES_HST(ID_,
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
		                                                                 CR_CODE_NAME,
	                                                                     FC_NUMBER,
	                                                                       CC_NUMBER,
	                                                                         WEIHGT_DIFF,
	                                                                           ARM_DIFF,
	                                                                             INDEX_DIFF,
	                                                                               CODE_TYPE,
	                                                                                 REMARKS,
		                                                                                 U_NAME,
		                                                                                   U_IP,
		                                                                                     U_HOST_NAME,
		                                                                                       DATE_WRITE,
                                                                                             BY_DEFAULT)
    select SEC_WB_REF_WS_AIR_DOW_CR_C_HST.nextval,
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
                                 i.CR_CODE_NAME,
	                                 i.FC_NUMBER,
	                                   i.CC_NUMBER,
	                                     i.WEIHGT_DIFF,
	                                       i.ARM_DIFF,
	                                         i.INDEX_DIFF,
	                                           i.CODE_TYPE,
	                                             i.REMARKS,
		                                             i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                                   i.DATE_WRITE,
                                                         i.BY_DEFAULT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_CR_CODES i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_CR_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_CR_CODES.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_CR_CODES.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_PT_CODES_HST(ID_,
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
		                                                                 PT_CODE_NAME,
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
    select SEC_WB_REF_WS_AIR_DOW_PT_C_HST.nextval,
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
                                 i.PT_CODE_NAME,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_PT_CODES i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_PT_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_PT_CODES.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_PT_CODES.id_ws=t.num);
    ----------------------------------------------------------------------------
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_PW_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_PW_CODES.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_PW_CODES.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_SWA_CDER_HST(ID_,
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
		                                                                 IS_BALANCE_ARM,
	                                                                     IS_INDEX_UNIT,
                                                                         U_NAME,
		                                                                       U_IP,
		                                                                         U_HOST_NAME,
		                                                                           DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_SWA_CD_H.nextval,
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
                                 i.IS_BALANCE_ARM,
	                                 i.IS_INDEX_UNIT,	
		                                 i.U_NAME,
		                                   i.U_IP,
		                                     i.U_HOST_NAME,
		                                       i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_SWA_CDER i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_SWA_CDER
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_SWA_CDER.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_SWA_CDER.id_ws=t.num);
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_DOW_SWA_CODE_HST(ID_,
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
		                                                                 CODE_NAME_1,
	                                                                     CODE_NAME_2,
		                                                                     WEIGHT,
		                                                                       BALANCE_ARM,
		                                                                         INDEX_UNIT,
	                                                                             REMARKS,
		                                                                             U_NAME,
		                                                                               U_IP,
		                                                                                 U_HOST_NAME,
		                                                                                   DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_SWA_C_HS.nextval,
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
                                 i.CODE_NAME_1,
	                                 i.CODE_NAME_2,
		                                 i.WEIGHT,
		                                   i.BALANCE_ARM,
		                                     i.INDEX_UNIT,
	                                         i.REMARKS,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
		                                               i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOW_SWA_CODES i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_DOW_SWA_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_DOW_SWA_CODES.id_ac=t.id and
                       WB_REF_WS_AIR_DOW_SWA_CODES.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_WCC_AC
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_WCC_AC.id_ac=t.id and
                       WB_REF_WS_AIR_WCC_AC.id_ws=t.num);

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
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_WCC
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_WCC.id_ac=t.id and
                       WB_REF_WS_AIR_WCC.id_ws=t.num);
    -------------------------------------------------------------------------------------------------------------DOW_BUILD_UP
    -------------------------------------------------------------------------------------------------------------------------

    -------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------REGISTRATION WEIGHTS
    insert into WB_REF_WS_AIR_REG_WGT_A_HST(ID_,
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
                                                                  CH_BALANCE_ARM,
	                                                                  CH_INDEX_UNIT,
	                                                                    CH_PROC_MAC,
	                                                                      U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
    select SEC_WB_REF_WS_AIR_REG_WGT_A_HS.nextval,
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
                                 i.CH_BALANCE_ARM,
	                                 i.CH_INDEX_UNIT,
	                                   i.CH_PROC_MAC,
                                       i.U_NAME,
		                                     i.U_IP,
		                                       i.U_HOST_NAME,
		                                         i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_REG_WGT_A i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_REG_WGT_A
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_REG_WGT_A.id_ac=t.id and
                       WB_REF_WS_AIR_REG_WGT_A.id_ws=t.num);
    ----------------------------------------------------------------------------

    insert into WB_REF_WS_AIR_REG_WGT_HST(ID_,
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
                                                                  ADV_ID,
	                                                                  DATE_FROM,
	                                                                    S_L_ADV_ID,
	                                                                      WCC_ID,
	                                                                        IS_ACARS,
	                                                                          IS_DEFAULT,
	                                                                            IS_APPROVED,
	                                                                              DOW,
	                                                                                DOI,
	                                                                                  ARM,
	                                                                                    PROC_MAC,
	                                                                                      REMARK,
	                                                                                        U_NAME,
		                                                                                        U_IP,
		                                                                                          U_HOST_NAME,
		                                                                                            DATE_WRITE)
    select SEC_WB_REF_WS_AIR_REG_WGT_HS.nextval,
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
                                 i.ADV_ID,
	                                 i.DATE_FROM,
	                                   i.S_L_ADV_ID,
	                                     i.WCC_ID,
	                                       i.IS_ACARS,
	                                         i.IS_DEFAULT,
	                                           i.IS_APPROVED,
	                                             i.DOW,
	                                               i.DOI,
	                                                 i.ARM,
	                                                   i.PROC_MAC,
	                                                     i.REMARK,
	                                                       i.U_NAME,
		                               	                       i.U_IP,
		                                	                       i.U_HOST_NAME,
		                                 	                         i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_REG_WGT i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_REG_WGT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_REG_WGT.id_ac=t.id and
                       WB_REF_WS_AIR_REG_WGT.id_ws=t.num);
    -----------------------------------------------------------------------------------------------------REGISTRATION WEIGHTS
    -------------------------------------------------------------------------------------------------------------------------

    -------------------------------------------------------------------------------------------------------------------------
    ----------------------------------------------------------------------------------------------------------MAXIMUM_WEIGHTS
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
                         i.id,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MAX_WGHT_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_MAX_WGHT_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_MAX_WGHT_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_MAX_WGHT_IDN.id_ws=t.num);
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MAX_WGHT_USE d
     on d.id_ac=t.id and
        d.id_ws=t.num;

     delete from WB_REF_WS_AIR_MAX_WGHT_USE
     where exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id=WB_REF_WS_AIR_MAX_WGHT_USE.id_ac and
                        t.num=WB_REF_WS_AIR_MAX_WGHT_USE.id_ws);
     ---------------------------------------------------------------------------
     ---------------------------------------------------------------------------
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MAX_WGHT_T d
     on d.id_ac=t.id and
        d.id_ws=t.num;

    delete from WB_REF_WS_AIR_MAX_WGHT_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_MAX_WGHT_T.id_ac and
                       t.num=WB_REF_WS_AIR_MAX_WGHT_T.id_ws);
    ----------------------------------------------------------------------------------------------------------MAXIMUM_WEIGHTS
    -------------------------------------------------------------------------------------------------------------------------

    -------------------------------------------------------------------------------------------------------------------------
    ----------------------------------------------------------------------------------------------------------MINIMUM_WEIGHTS
    insert into WB_REF_WS_AIR_MIN_WGHT_IDN_HS(ID_,
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
    select SEC_WB_REF_WS_AIR_MIN_W_IDN_HS.nextval,
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
                                 i.TABLE_NAME,
	                                 i.CONDITION,
	                                   i.TYPE,
	                                     i.TYPE_FROM,
	                                       i.TYPE_TO,
                                           i.U_NAME,
		                                         i.U_IP,
		                                           i.U_HOST_NAME,
		                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MIN_WGHT_IDN i
    on i.id_ac=t.id and
       i.id_ws=t.num;

    delete from WB_REF_WS_AIR_MIN_WGHT_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where WB_REF_WS_AIR_MIN_WGHT_IDN.id_ac=t.id and
                       WB_REF_WS_AIR_MIN_WGHT_IDN.id_ws=t.num);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_MIN_WGHT_USE_H (ID_,
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
    select SEC_WB_REF_WS_AIR_MN_WT_USE_H.nextval,
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
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MIN_WGHT_USE d
     on d.id_ac=t.id and
        d.id_ws=t.num;

     delete from WB_REF_WS_AIR_MIN_WGHT_USE
     where exists(select 1
                  from WB_TEMP_XML_ID t
                  where t.id=WB_REF_WS_AIR_MIN_WGHT_USE.id_ac and
                        t.num=WB_REF_WS_AIR_MIN_WGHT_USE.id_ws);
    ---------------------------------------------------------------------------
     ---------------------------------------------------------------------------
     insert into WB_REF_WS_AIR_MIN_WGHT_T_H(ID_,
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
                                                                	 MIN_RMP_WEIGHT,
                                                                 	   MIN_ZF_WEIGHT,
                                                                	     MIN_TO_WEIGHT,
	                                                                       MIN_LND_WEIGHT,
	                                                                         IS_DEFAULT,
	                                                                           REMARK,
	                                                                             U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_MIN_W_T_H.nextval,
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
                                   d.MIN_RMP_WEIGHT,
                                     d.MIN_ZF_WEIGHT,
                                       d.MIN_TO_WEIGHT,
	                                       d.MIN_LND_WEIGHT,
	                                         d.IS_DEFAULT,
	                                           d.REMARK,
                                               d.U_NAME,
		                               	             d.U_IP,
		                                	             d.U_HOST_NAME,
		                                 	               d.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MIN_WGHT_T d
     on d.id_ac=t.id and
        d.id_ws=t.num;

    delete from WB_REF_WS_AIR_MIN_WGHT_T
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_WS_AIR_MIN_WGHT_T.id_ac and
                       t.num=WB_REF_WS_AIR_MIN_WGHT_T.id_ws);
    ----------------------------------------------------------------------------------------------------------MINIMUM_WEIGHTS
    -------------------------------------------------------------------------------------------------------------------------

    -------------------------------------------------------------------------------------------------------------------------
    ---------------------------------------------------------------------------------------------------------------------SHED
    insert into WB_SHED_HIST (ID_,
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
	                                                  BORT_NUM,
	                                                    NR,
	                                                      ID_AP_1,
	                                                        ID_AP_2,
	                                                          TERM_1,
	                                                            TERM_2,
	                                                              MVL_TYPE,
	                                                                S_DTU_1,
	                                                                  S_DTU_2,
	                                                                    S_DTL_1,
	                                                                      S_DTL_2,
	                                                                        E_DTU_1,
	                                                                          E_DTU_2,
	                                                                            E_DTL_1,
	                                                                              E_DTL_2,
	                                                                                F_DTU_1,
	                                                                                  F_DTU_2,
	                                                                                    F_DTL_1,
	                                                                                      F_DTL_2,
	                                                                                        U_NAME,
	                                                                                          U_IP,
	                                                                                            U_HOST_NAME,
	                                                                                              DATE_WRITE)
    select SEC_WB_SHED_HIST.nextval,
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
                                 d.BORT_NUM,
	                                 d.NR,
	                                   d.ID_AP_1,
	                                     d.ID_AP_2,
	                                       d.TERM_1,
	                                         d.TERM_2,
	                                           d.MVL_TYPE,
	                                             d.S_DTU_1,
	                                               d.S_DTU_2,
	                                                 d.S_DTL_1,
	                                                   d.S_DTL_2,
	                                                     d.E_DTU_1,
	                                                       d.E_DTU_2,
	                                                         d.E_DTL_1,
	                                                           d.E_DTL_2,
	                                                             d.F_DTU_1,
	                                                               d.F_DTU_2,
	                                                                 d.F_DTL_1,
	                                                                   d.F_DTL_2,
	                                                                     d.U_NAME,
	                                                                       d.U_IP,
	                                                                         d.U_HOST_NAME,
	                                                                           d.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_SHED d
     on d.id_ac=t.id and
        d.id_ws=t.num;

    delete from WB_SHED
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_SHED.id_ac and
                       t.num=WB_SHED.id_ws);
    ---------------------------------------------------------------------------------------------------------------------SHED
    -------------------------------------------------------------------------------------------------------------------------

      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    if P_INT_TRAN=0 then
      begin


        commit;
      end;
    end if;
  end SP_WB_REF_AC_WS_DELETE;
/
