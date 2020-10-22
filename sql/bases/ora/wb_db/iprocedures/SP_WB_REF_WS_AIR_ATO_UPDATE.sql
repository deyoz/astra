create or replace procedure SP_WB_REF_WS_AIR_ATO_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_SORT_CONTAINER_LOADING number:=0;
P_EZFW_THRESHOLD_LIMIT number:=0;
P_CHECK_LATERAL_I_LIMITS number:=0;

P_PTO_CLASS_TRIM number:=0;
P_PTO_CABIN_AREA_TRIM number:=0;
P_PTO_SEAT_ROW_TRIM number:=0;

P_EZFW_T_LIMIT_VAL number:=null;
P_EZFW_T_LIMIT_VAL_INT_PART varchar2(50);
P_EZFW_T_LIMIT_VAL_DEC_PART varchar2(50);

P_REMARK clob:='';
P_IS_REMARK_EMPTY number:=0;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SORT_CONTAINER_LOADING[1]')) into P_SORT_CONTAINER_LOADING from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_EZFW_THRESHOLD_LIMIT[1]')) into P_EZFW_THRESHOLD_LIMIT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CHECK_LATERAL_IMBALANCE_LIMITS[1]')) into P_CHECK_LATERAL_I_LIMITS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PTO_CLASS_TRIM[1]')) into P_PTO_CLASS_TRIM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PTO_CABIN_AREA_TRIM[1]')) into P_PTO_CABIN_AREA_TRIM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PTO_SEAT_ROW_TRIM[1]')) into P_PTO_SEAT_ROW_TRIM from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_EZFW_THRESHOLD_LIMIT_VAL_INT_PART[1]') into P_EZFW_T_LIMIT_VAL_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_EZFW_THRESHOLD_LIMIT_VAL_DEC_PART[1]') into P_EZFW_T_LIMIT_VAL_DEC_PART from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    if P_EZFW_T_LIMIT_VAL_INT_PART<>'NULL'
      then P_EZFW_T_LIMIT_VAL:=to_number(P_EZFW_T_LIMIT_VAL_INT_PART||
                                           '.'||
                                             P_EZFW_T_LIMIT_VAL_DEC_PART);
    end if;

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID_WS;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='This aircraft is removed!';
          end;
        else
          begin
            str_msg:='Этт тип ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (str_msg is null) and
         (P_ID_AC<>-1) then
      begin
        select count(id) into R_COUNT
        from WB_REF_AIRCOMPANY_KEY
        where id=P_ID_AC;

        if R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='This airline is removed!';
              end;
            else
              begin
                str_msg:='Эта авиакомпания удалена!';
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
        select count(id) into R_COUNT
        from WB_REF_WS_AIR_ATO
        where id=P_ID;

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
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into R_COUNT
         from WB_REF_WS_AIR_ATO
         where ID<>P_ID and
               ID_AC=P_ID_AC and
               ID_WS=P_ID_WS and
               ID_BORT=P_ID_BORT and
               DATE_FROM=P_DATE_FROM;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Record on this date already exists!';
               end;
             else
               begin
                 str_msg:='Запись с такой датой уже существует!';
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
        if p_IS_REMARK_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REMARK_EMPTY=0 then
          begin
            if P_REMARK='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Remarks&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Примечание&quot; является зарезервированной фразой!';
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

    if (str_msg is null) then
      begin
        update WB_REF_WS_AIR_ATO
        set DATE_FROM=P_DATE_FROM,
            SORT_CONTAINER_LOADING=P_SORT_CONTAINER_LOADING,
	          EZFW_THRESHOLD_LIMIT=P_EZFW_THRESHOLD_LIMIT,
	          EZFW_THRESHOLD_LIMIT_VAL=P_EZFW_T_LIMIT_VAL,
	          CHECK_LATERAL_IMBALANCE_LIMITS=P_CHECK_LATERAL_I_LIMITS,
	          PTO_CLASS_TRIM=P_PTO_CLASS_TRIM,
	          PTO_CABIN_AREA_TRIM=P_PTO_CABIN_AREA_TRIM,
	          PTO_SEAT_ROW_TRIM=P_PTO_SEAT_ROW_TRIM,
	          REMARK=P_REMARK,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where ID=P_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_ATO_UPDATE;
/
