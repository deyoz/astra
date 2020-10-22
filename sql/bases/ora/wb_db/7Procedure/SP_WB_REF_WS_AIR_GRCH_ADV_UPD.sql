create or replace procedure SP_WB_REF_WS_AIR_GRCH_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(1000):='';
P_CH_INDEX number:=-1;
P_CH_PROC_MAC number:=-1;
P_CH_CERTIFIED number:=-1;
P_CH_CURTAILED number:=-1;

P_CONDITION varchar2(1000):='';
P_IS_CONDITION_EMPTY number;
P_TYPE varchar2(1000):='';
P_IS_TYPE_EMPTY number;

P_REMARK_1 clob:='';
P_REMARK_2 clob:='';
P_IS_REMARK_1_EMPTY number:=0;
P_IS_REMARK_2_EMPTY number:=0;

P_TYPE_FROM_INT_PART varchar2(1000):='';
P_TYPE_FROM_DEC_PART varchar2(1000):='';
P_TYPE_FROM number:=null;

P_TYPE_TO_INT_PART varchar2(1000):='';
P_TYPE_TO_DEC_PART varchar2(1000):='';
P_TYPE_TO number:=null;

P_IDN_REL number:=-1;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;

V_ACTION_NAME varchar2(100):='SP_WB_REF_WS_AIR_GRCH_ADV_UPD';
V_ACTION_DATE date:=sysdate();
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_INDEX[1]')) into P_CH_INDEX from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_PROC_MAC[1]')) into P_CH_PROC_MAC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CERTIFIED[1]')) into P_CH_CERTIFIED from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CURTAILED[1]')) into P_CH_CURTAILED from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IDN_REL[1]')) into P_IDN_REL from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CONDITION[1]') into P_CONDITION from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE[1]') into P_TYPE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_REMARK_1[1]') into P_REMARK_1 from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_REMARK_2[1]') into P_REMARK_2 from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_CONDITION_EMPTY[1]')) into P_IS_CONDITION_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_TYPE_EMPTY[1]')) into P_IS_TYPE_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_REMARK_1_EMPTY[1]')) into P_IS_REMARK_1_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_REMARK_2_EMPTY[1]')) into P_IS_REMARK_2_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE_FROM_INT_PART[1]') into P_TYPE_FROM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE_FROM_DEC_PART[1]') into P_TYPE_FROM_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE_TO_INT_PART[1]') into P_TYPE_TO_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE_TO_DEC_PART[1]') into P_TYPE_TO_DEC_PART from dual;

    if P_TYPE_FROM_INT_PART<>'NULL' then
      begin
        P_TYPE_FROM:=to_number(P_TYPE_FROM_INT_PART||'.'||P_TYPE_FROM_DEC_PART);
      end;
    end if;

    if P_TYPE_TO_INT_PART<>'NULL' then
      begin
        P_TYPE_TO:=to_number(P_TYPE_TO_INT_PART||'.'||P_TYPE_TO_DEC_PART);
      end;
    end if;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    ---------------------------------------------------------------------------------------------------------------------
    insert into WB_TEMP_XML_ID_EX (ID,
                                     F_FLT_1,
                                       F_FLT_2,
                                         F_FLT_3,
                                           ACTION_NAME,
                                             ACTION_DATE)
    select distinct P_ID,
                      f.P_USE_ITEM_ID,
                        case when f.P_VALUE_1_INT_PART='NULL'
                             then NULL
                             else to_number(f.P_VALUE_1_INT_PART||'.'||f.P_VALUE_1_DEC_PART)
                        end,
                          case when f.P_VALUE_2_INT_PART='NULL'
                               then NULL
                               else to_number(f.P_VALUE_2_INT_PART||'.'||f.P_VALUE_2_DEC_PART)
                          end,
                            V_ACTION_NAME,
                              V_ACTION_DATE
    from (select extractValue(value(t),'u_c_data/P_USE_ITEM_ID[1]') as P_USE_ITEM_ID,
                   extractValue(value(t),'u_c_data/P_VALUE_1_INT_PART[1]') as P_VALUE_1_INT_PART,
                     extractValue(value(t),'u_c_data/P_VALUE_1_DEC_PART[1]') as P_VALUE_1_DEC_PART,
                       extractValue(value(t),'u_c_data/P_VALUE_2_INT_PART[1]') as P_VALUE_2_INT_PART,
                         extractValue(value(t),'u_c_data/P_VALUE_2_DEC_PART[1]') as P_VALUE_2_DEC_PART
          from table(xmlsequence(xmltype(cXML_in).extract('//u_c_data'))) t) f;


    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_AIR_GR_CH_ADV
    where idn=P_ID;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='This record is removed!';
          end;
        else
          begin
            str_msg:='Эта запись удалена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
              not exists(select 1
                         from WB_REF_WS_AIR_GR_CH_USE_ITEMS i
                         where i.id=t.F_FLT_1);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Some of the parameters for the use of terms deleted from the directory!';
              end;
            else
              begin
                str_msg:='Часть параметров для условий использования удалена из справочника!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        if p_IS_REMARK_2_EMPTY=1 then
          begin

            P_REMARK_2:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REMARK_2_EMPTY=0 then
          begin
            if P_REMARK_2='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field "Remarks" is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Значение поля "Примечание" является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;

          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        if p_IS_CONDITION_EMPTY=1 then
          begin

            P_CONDITION:='EMPTY_STRING';
          end;
        end if;

        if p_IS_CONDITION_EMPTY=0 then
          begin
            if P_CONDITION='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field "Condition" is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Значение поля "Условие" является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;

          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        if p_IS_TYPE_EMPTY=1 then
          begin

            P_TYPE:='EMPTY_STRING';
          end;
        end if;

        if p_IS_TYPE_EMPTY=0 then
          begin
            if P_TYPE='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field "Condition" is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Значение поля "Условие" является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;

          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        if P_IDN_REL>-1 then
          begin
            select count(id) into R_COUNT
            from WB_REF_WS_AIR_GR_CH_IDN
            where id=P_IDN_REL;

            if R_COUNT=0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Selected for relation CG Chart removed by another user!';
                  end;
                else
                  begin
                    str_msg:='Выбранный для связи CG Chart удален другим пользователем!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    ----------------------------------------------------------------------------
    if (str_msg is null) then
      begin
        UPDATE WB_REF_WS_AIR_GR_CH_ADV
        set CH_INDEX=P_CH_INDEX,
            CH_PROC_MAC=P_CH_PROC_MAC,
	          CH_CERTIFIED=P_CH_CERTIFIED,
	          CH_CURTAILED=P_CH_CURTAILED,
	          TABLE_NAME=P_TABLE_NAME,
            CONDITION=P_CONDITION,
	          TYPE=P_TYPE,
	          TYPE_FROM=P_TYPE_FROM,
	          TYPE_TO=P_TYPE_TO,
	          REMARK_1=P_REMARK_1,
	          REMARK_2=P_REMARK_2,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where idn=P_ID;

        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_A_L d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_A_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           i.id=WB_REF_WS_AIR_GR_CH_A_L.id_ch);

       /* where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_A_L d
                     on i.IDN=P_ID and
                        d.id_ch=i.id);*/

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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_F_L d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_F_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           i.id=WB_REF_WS_AIR_GR_CH_F_L.id_ch);
        /*
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_F_L d
                     on i.IDN=P_ID and
                        d.id_ch=i.id);*/
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_ITL_L d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_ITL_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           i.id=WB_REF_WS_AIR_GR_CH_ITL_L.id_ch);
        /*
        delete from WB_REF_WS_AIR_GR_CH_ITL_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_ITL_L d
                     on i.IDN=P_ID and
                        d.id_ch=i.id);*/
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_BORT d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_BORT
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           i.id=WB_REF_WS_AIR_GR_CH_BORT.id_ch);
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_L_L d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_L_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           i.id=WB_REF_WS_AIR_GR_CH_L_L.id_ch);
        /*
        delete from WB_REF_WS_AIR_GR_CH_L_L
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_L_L d
                     on i.IDN=P_ID and
                        d.id_ch=i.id);*/

         insert into WB_REF_WS_AIR_GR_CH_A_L (ID,
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
        select SEC_WB_REF_WS_AIR_GR_CH_A_L.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 i.ID_BORT,
                       i.id,
                         to_number(P_WEIGHT_INT_PART||'.'||P_WEIGHT_DEC_PART),
                           to_number(P_PROC_MAC_INT_PART||'.'||P_PROC_MAC_DEC_PART),
                             to_number(P_INDEX_INT_PART||'.'||P_INDEX_DEC_PART),
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
	                                   sysdate()
        from (select extractValue(value(t),'a_l_data/P_WEIGHT_INT_PART[1]') as P_WEIGHT_INT_PART,
                       extractValue(value(t),'a_l_data/P_WEIGHT_DEC_PART[1]') as P_WEIGHT_DEC_PART,
                         extractValue(value(t),'a_l_data/P_PROC_MAC_INT_PART[1]') as P_PROC_MAC_INT_PART,
                           extractValue(value(t),'a_l_data/P_PROC_MAC_DEC_PART[1]') as P_PROC_MAC_DEC_PART,
                             extractValue(value(t),'a_l_data/P_INDEX_INT_PART[1]') as P_INDEX_INT_PART,
                               extractValue(value(t),'a_l_data/P_INDEX_DEC_PART[1]') as P_INDEX_DEC_PART
              from table(xmlsequence(xmltype(cXML_in).extract('//a_l_data'))) t) f join WB_REF_WS_AIR_GR_CH_ADV i
        on i.IDN=P_ID;

        insert into WB_REF_WS_AIR_GR_CH_F_L (ID,
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
        select SEC_WB_REF_WS_AIR_GR_CH_F_L.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 i.ID_BORT,
                       i.id,
                         to_number(P_WEIGHT_INT_PART||'.'||P_WEIGHT_DEC_PART),
                           to_number(P_PROC_MAC_INT_PART||'.'||P_PROC_MAC_DEC_PART),
                             to_number(P_INDEX_INT_PART||'.'||P_INDEX_DEC_PART),
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
	                                   sysdate()
        from (select extractValue(value(t),'f_l_data/P_WEIGHT_INT_PART[1]') as P_WEIGHT_INT_PART,
                       extractValue(value(t),'f_l_data/P_WEIGHT_DEC_PART[1]') as P_WEIGHT_DEC_PART,
                         extractValue(value(t),'f_l_data/P_PROC_MAC_INT_PART[1]') as P_PROC_MAC_INT_PART,
                           extractValue(value(t),'f_l_data/P_PROC_MAC_DEC_PART[1]') as P_PROC_MAC_DEC_PART,
                             extractValue(value(t),'f_l_data/P_INDEX_INT_PART[1]') as P_INDEX_INT_PART,
                               extractValue(value(t),'f_l_data/P_INDEX_DEC_PART[1]') as P_INDEX_DEC_PART
              from table(xmlsequence(xmltype(cXML_in).extract('//f_l_data'))) t) f join WB_REF_WS_AIR_GR_CH_ADV i
        on i.IDN=P_ID;

        insert into WB_REF_WS_AIR_GR_CH_ITL_L (ID,
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
        select SEC_WB_REF_WS_AIR_GR_CH_ITL_L.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 i.ID_BORT,
                       i.id,
                         to_number(P_WEIGHT_INT_PART||'.'||P_WEIGHT_DEC_PART),
                           to_number(P_PROC_MAC_INT_PART||'.'||P_PROC_MAC_DEC_PART),
                             to_number(P_INDEX_INT_PART||'.'||P_INDEX_DEC_PART),
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
	                                   sysdate()
        from (select extractValue(value(t),'itl_l_data/P_WEIGHT_INT_PART[1]') as P_WEIGHT_INT_PART,
                       extractValue(value(t),'itl_l_data/P_WEIGHT_DEC_PART[1]') as P_WEIGHT_DEC_PART,
                         extractValue(value(t),'itl_l_data/P_PROC_MAC_INT_PART[1]') as P_PROC_MAC_INT_PART,
                           extractValue(value(t),'itl_l_data/P_PROC_MAC_DEC_PART[1]') as P_PROC_MAC_DEC_PART,
                             extractValue(value(t),'itl_l_data/P_INDEX_INT_PART[1]') as P_INDEX_INT_PART,
                               extractValue(value(t),'itl_l_data/P_INDEX_DEC_PART[1]') as P_INDEX_DEC_PART
              from table(xmlsequence(xmltype(cXML_in).extract('//itl_l_data'))) t) f join WB_REF_WS_AIR_GR_CH_ADV i
        on i.IDN=P_ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_GR_CH_BORT (ID,
	                                              ID_AC,
	                                                ID_WS,
		                                                ID_BORT,
		                                                  ID_CH,	
	                                                      U_NAME,
	                                                        U_IP,
	                                                          U_HOST_NAME,
	                                                            DATE_WRITE)
        select SEC_WB_REF_WS_AIR_GR_CH_BORT.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 f.ID_BORT,
                       i.id,
                         P_U_NAME,
	                         P_U_IP,
	                           P_U_HOST_NAME,
	                             sysdate()
        from (select to_number(extractValue(value(t),'bort_data/P_ID_BORT[1]')) as ID_BORT
              from table(xmlsequence(xmltype(cXML_in).extract('//bort_data'))) t) f join WB_REF_WS_AIR_GR_CH_ADV i
        on i.IDN=P_ID join WB_REF_AIRCO_WS_BORTS b
           on b.id=f.ID_BORT and
              b.id_ac=i.ID_AC and
              b.id_ws=i.ID_WS;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_GR_CH_L_L (ID,
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
        select SEC_WB_REF_WS_AIR_GR_CH_L_L.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 i.ID_BORT,
                       i.id,
                         to_number(P_WEIGHT_INT_PART||'.'||P_WEIGHT_DEC_PART),
                           to_number(P_PROC_MAC_INT_PART||'.'||P_PROC_MAC_DEC_PART),
                             to_number(P_INDEX_INT_PART||'.'||P_INDEX_DEC_PART),
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
	                                   sysdate()
        from (select extractValue(value(t),'l_l_data/P_WEIGHT_INT_PART[1]') as P_WEIGHT_INT_PART,
                       extractValue(value(t),'l_l_data/P_WEIGHT_DEC_PART[1]') as P_WEIGHT_DEC_PART,
                         extractValue(value(t),'l_l_data/P_PROC_MAC_INT_PART[1]') as P_PROC_MAC_INT_PART,
                           extractValue(value(t),'l_l_data/P_PROC_MAC_DEC_PART[1]') as P_PROC_MAC_DEC_PART,
                             extractValue(value(t),'l_l_data/P_INDEX_INT_PART[1]') as P_INDEX_INT_PART,
                               extractValue(value(t),'l_l_data/P_INDEX_DEC_PART[1]') as P_INDEX_DEC_PART
              from table(xmlsequence(xmltype(cXML_in).extract('//l_l_data'))) t) f join WB_REF_WS_AIR_GR_CH_ADV i
        on i.IDN=P_ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_GR_CH_ADV i join WB_REF_WS_AIR_GR_CH_USE d
        on i.IDN=P_ID and
           d.id_ch=i.id;

        delete from WB_REF_WS_AIR_GR_CH_USE
        where exists(select 1
                     from WB_REF_WS_AIR_GR_CH_ADV i
                     where i.IDN=P_ID and
                           WB_REF_WS_AIR_GR_CH_USE.id_ch=i.id);

        insert into WB_REF_WS_AIR_GR_CH_USE (ID,
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
       select SEC_WB_REF_WS_AIR_GR_CH_USE.nextval,
                 i.ID_AC,
	                 i.ID_WS,
		                 i.ID_BORT,
                       i.id,
                         t.f_flt_1,
                           -1,
                             t.f_flt_2,
                               -1,
                                 t.f_flt_3,
                                   P_U_NAME,
	                                   P_U_IP,
	                                     P_U_HOST_NAME,
	                                       sysdate()
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_GR_CH_ADV i
        on t.ACTION_NAME=V_ACTION_NAME and
           t.ACTION_DATE=V_ACTION_DATE and
           i.IDN=t.ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
         from WB_REF_WS_AIR_GR_CH_REL i
         where i.IDN=P_ID;

         delete WB_REF_WS_AIR_GR_CH_REL
         where IDN=P_ID;

         if P_IDN_REL>-1 then
           begin
             insert into WB_REF_WS_AIR_GR_CH_REL (ID,
	                                                  ID_AC,
	                                                    ID_WS,
		                                                    ID_BORT,
		                                                      IDN,
	                                                          IDN_REL,
		                                                          U_NAME,
		                                                            U_IP,
		                                                              U_HOST_NAME,
		                                                                DATE_WRITE)
             select SEC_WB_REF_WS_AIR_GR_CH_REL.nextval,
                      i.ID_AC,
	                      i.ID_WS,
		                      i.ID_BORT,
                            i.IDN,
                              P_IDN_REL,
                                P_U_NAME,
	                                P_U_IP,
	                                  P_U_HOST_NAME,
                                      SYSDATE()
             from WB_REF_WS_AIR_GR_CH_ADV i
             where i.IDN=P_ID;
           end;
         end if;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    commit;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_GRCH_ADV_UPD;
/
