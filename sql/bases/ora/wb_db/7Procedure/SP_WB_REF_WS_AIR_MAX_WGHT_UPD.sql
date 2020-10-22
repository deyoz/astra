create or replace procedure SP_WB_REF_WS_AIR_MAX_WGHT_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(1000):='';

P_CONDITION varchar2(1000):='';
P_IS_CONDITION_EMPTY number;
P_TYPE varchar2(1000):='';
P_IS_TYPE_EMPTY number;

P_TYPE_FROM_INT_PART varchar2(1000):='';
P_TYPE_FROM_DEC_PART varchar2(1000):='';
P_TYPE_FROM number:=null;

P_TYPE_TO_INT_PART varchar2(1000):='';
P_TYPE_TO_DEC_PART varchar2(1000):='';
P_TYPE_TO number:=null;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;

V_ACTION_NAME varchar2(100):='SP_WB_REF_WS_AIR_MAX_WGHT_UPD';
V_ACTION_DATE date:=sysdate();
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CONDITION[1]') into P_CONDITION from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TYPE[1]') into P_TYPE from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_CONDITION_EMPTY[1]')) into P_IS_CONDITION_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_TYPE_EMPTY[1]')) into P_IS_TYPE_EMPTY from dual;

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
    ---------------------------------------------------------------------------------------------------------------------

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_MAX_WGHT_IDN
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This record is removed!';
          end;
        else
          begin
            V_STR_MSG:='Эта запись удалена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
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
                    V_STR_MSG:='Value field "Condition" is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение поля "Condition" является зарезервированной фразой!';
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
    if V_STR_MSG is null then
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
                    V_STR_MSG:='Value field "Type" is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение поля "Type" является зарезервированной фразой!';
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
    if V_STR_MSG is null then
      begin
        select count(t.id) into V_R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
              not exists(select 1
                         from WB_REF_WS_AIR_GR_CH_USE_ITEMS i
                         where i.id=t.F_FLT_1);

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='Some of the parameters for the use of terms deleted from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Часть параметров для условий использования удалена из справочника!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        UPDATE WB_REF_WS_AIR_MAX_WGHT_IDN
        set TABLE_NAME=P_TABLE_NAME,
            CONDITION=P_CONDITION,
	          TYPE=P_TYPE,
	          TYPE_FROM=P_TYPE_FROM,
	          TYPE_TO=P_TYPE_TO,	
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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
        from WB_REF_WS_AIR_MAX_WGHT_USE d
        where d.idn=P_ID;

        delete from WB_REF_WS_AIR_MAX_WGHT_USE where idn=P_ID;

        insert into WB_REF_WS_AIR_MAX_WGHT_USE (ID,
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
       select SEC_WB_REF_WS_AIR_MX_WT_USE.nextval,
                a.ID_AC,
	                a.ID_WS,
		                a.ID_BORT,
		                  a.ID,
                        t.f_flt_1,
                          -1,
                            t.f_flt_2,
                              -1,
                                t.f_flt_3,
                                  P_U_NAME,
	                                  P_U_IP,
	                                    P_U_HOST_NAME,
	                                      sysdate()
       from WB_REF_WS_AIR_MAX_WGHT_IDN a join WB_TEMP_XML_ID_EX t
       on a.id=P_ID and
          t.ACTION_NAME=V_ACTION_NAME and
          t.ACTION_DATE=V_ACTION_DATE;


        V_STR_MSG:='EMPTY_STRING';
      end;
      end if;

    commit;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_MAX_WGHT_UPD;
/
