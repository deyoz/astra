create or replace procedure SP_WB_REF_WS_AIR_TYPE_UPDATE
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

P_UPPER_DECK number:=0;
P_MAIN_DECK number:=0;
P_LOWER_DECK number:=0;

P_ID_WS_TRANSP_KATEG number:=-1;
P_ID_WS_TYPE_OF_LOADING number:=-1;

P_REVISION varchar2(500):='';
P_IS_REVISION_EMPTY number:=0;
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
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_UPPER_DECK[1]')) into P_UPPER_DECK from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAIN_DECK[1]')) into P_MAIN_DECK from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LOWER_DECK[1]')) into P_LOWER_DECK from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS_TRANSP_KATEG[1]')) into P_ID_WS_TRANSP_KATEG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS_TYPE_OF_LOADING[1]')) into P_ID_WS_TYPE_OF_LOADING from dual;


    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REVISION_EMPTY[1]')) into P_IS_REVISION_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REVISION[1]') into P_REVISION from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
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
    if str_msg is null then
      begin
        select count(id) into R_COUNT
        from WB_REF_WS_AIR_TYPE
        where ID=P_ID;

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
    if str_msg is null then
      begin
        select count(id) into R_COUNT
        from WB_REF_WS_TRANSP_KATEG
        where id=P_ID_WS_TRANSP_KATEG;

        if R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='This Transport Category is removed!';
              end;
            else
              begin
                str_msg:='Эта транспортная категория удалена!';
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
        from WB_REF_WS_TYPE_OF_LOADING
        where id=P_ID_WS_TYPE_OF_LOADING;

        if R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='This Type Of Loading is removed!';
              end;
            else
              begin
                str_msg:='Этот Тип загрузки удален!';
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
         from WB_REF_WS_AIR_TYPE
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
        if p_IS_REVISION_EMPTY=1 then
          begin

            P_REVISION:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REVISION_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_REVISION;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Revision&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Ревизия; является зарезервированной фразой!';
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
        update WB_REF_WS_AIR_TYPE
        set DATE_FROM=P_DATE_FROM,
            REVIZION=P_REVISION,
            ID_WS_TRANSP_KATEG=P_ID_WS_TRANSP_KATEG,
	          ID_WS_TYPE_OF_LOADING=P_ID_WS_TYPE_OF_LOADING,
	          IS_UPPER_DECK=P_UPPER_DECK,
	          IS_MAIN_DECK=P_MAIN_DECK,
	          IS_LOWER_DECK=P_LOWER_DECK,
            REMARK=P_REMARK,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_TYPE_UPDATE;
/
