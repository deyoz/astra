create or replace procedure SP_WB_REF_WS_AIR_STLY_IDN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(200):='ENG';
V_RCOUNT number:=0;
V_STR_MSG clob:=null;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_STLY_IDN_DEL';
P_ACTION_DATE date:=sysdate();
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
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
    select count(i.id) into V_RCOUNT
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
    on a.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_REG_WGT i
       on i.S_L_ADV_ID=a.id;

    if V_RCOUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='There are links in the "Registration Weights" block. The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='Имеются ссылки в блоке "Registration Weights". Операция запрещена!';
          end;
        end if;
      end;
    end if;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if V_STR_MSG is null then
      begin
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on a.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
           on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CAI_T t
              on t.adv_id=aa.id join WB_REF_WS_AIR_SL_CAI_TT i
                 on i.t_id=t.id;

        delete from WB_REF_WS_AIR_SL_CAI_TT
        where id in (select distinct i.id
                     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
                     on a.idn=t.id and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
                        on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CAI_T t
                           on t.adv_id=aa.id join WB_REF_WS_AIR_SL_CAI_TT i
                              on i.t_id=t.id);
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on a.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
           on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CAI_T i
              on i.adv_id=aa.id;

        delete from WB_REF_WS_AIR_SL_CAI_T
        where id in (select distinct i.id
                     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
                     on a.idn=t.id and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
                        on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CAI_T i
                           on i.adv_id=aa.id);
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on a.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
           on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CI_T i
              on i.adv_id=aa.id;

        delete from WB_REF_WS_AIR_SL_CI_T
        where id in (select distinct i.id
                     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
                     on a.idn=t.id and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV aa
                        on aa.adv_id=a.id join WB_REF_WS_AIR_SL_CI_T i
                           on i.adv_id=aa.id);
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on a.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV i
           on i.adv_id=a.id;

        delete from WB_REF_WS_AIR_S_L_C_ADV
        where id in (select distinct i.id
                     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
                     on a.idn=t.id and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_ADV i
                        on i.adv_id=a.id);

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
                                       i.table_name,
		                                     i.U_NAME,
		                                    	 i.U_IP,
		                                         i.U_HOST_NAME,
		                                 	         i.DATE_WRITE
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on a.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_IDN i
           on i.adv_id=a.id;

        delete from WB_REF_WS_AIR_S_L_C_IDN
        where id in (select distinct i.id
                     from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV a
                     on a.idn=t.id and
                        t.ACTION_NAME=P_ACTION_NAME and
                        t.ACTION_DATE=P_ACTION_DATE join WB_REF_WS_AIR_S_L_C_IDN i
                        on i.adv_id=a.id);
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_IDN i
        on i.id=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_SEAT_LAY_IDN
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_SEAT_LAY_IDN.id and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV i
        on i.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_SEAT_LAY_ADV
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_SEAT_LAY_ADV.idn and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
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
                                   i.table_name,
		                                 i.U_NAME,
		                                 	 i.U_IP,
		                                     i.U_HOST_NAME,
		                                 	     i.DATE_WRITE
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_C_IDN i
    on i.adv_id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_S_L_C_IDN
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where WB_REF_WS_AIR_S_L_C_IDN.adv_id=t.id and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
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
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_C_ADV i
    on i.adv_id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;


    delete from WB_REF_WS_AIR_S_L_C_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where WB_REF_WS_AIR_S_L_C_ADV.adv_id=t.id and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
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
    from  WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_A_U_S i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_S_L_A_U_S
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_S_L_A_U_S.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
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
    from  WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_PLAN i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_S_L_PLAN
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_S_L_PLAN.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
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
    from  WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_P_S i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_S_L_P_S
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_S_L_P_S.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
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
    from  WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_S_L_P_S_P i
    on i.idn=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    delete from WB_REF_WS_AIR_S_L_P_S_P
    where exists(select 1
                 from WB_TEMP_XML_ID_EX t
                 where t.id=WB_REF_WS_AIR_S_L_P_S_P.idn and
                       t.ACTION_NAME=P_ACTION_NAME and
                       t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------








        str_msg:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_STLY_IDN_DEL;
/
