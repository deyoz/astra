create or replace procedure SP_WB_REF_WS_TYPES_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';
P_NAME_RUS_SMALL varchar2(100):='';
P_NAME_RUS_FULL varchar2(200):='';
P_NAME_ENG_SMALL varchar2(100):='';
P_NAME_ENG_FULL varchar2(200):='';
P_IATA varchar2(50):='';
P_ICAO varchar2(50):='';
P_DOP_IDENT varchar2(50):='';
P_DOP_IDENT_IS_EMPTY number:=0;
P_REMARK clob:='';
P_REMARK_IS_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
STR_MSG varchar2(1000):=null;
R_COUNT int:=0;
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_RUS_SMALL[1]') into p_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_RUS_FULL[1]') into p_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_ENG_SMALL[1]') into p_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_ENG_FULL[1]') into p_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IATA[1]') into P_IATA from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ICAO[1]') into P_ICAO from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOP_IDENT[1]') into P_DOP_IDENT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOP_IDENT_IS_EMPTY[1]')) into P_DOP_IDENT_IS_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK_IS_EMPTY[1]')) into P_REMARK_IS_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    -----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='The record is deleted by another user!';
          end;
        else
          begin
            str_msg:='Запись удалена другим пользователем!';
          end;
        end if;
      end;
     end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        if P_DOP_IDENT_IS_EMPTY=1 then
          begin

            P_DOP_IDENT:='EMPTY_STRING';
          end;
        end if;

        if P_DOP_IDENT_IS_EMPTY=0 then
          begin
            select count(id) into R_COUNT
            from WB_REF_RESERVED_PHRASE
            where phrase=P_DOP_IDENT;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Suffix&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Суффикс&quot; является зарезервированной фразой!';
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
        select count(id) into R_COUNT
        from WB_REF_WS_TYPES
        where ID<>P_ID and
              P_IATA=IATA and
              P_ICAO=ICAO and
              P_DOP_IDENT=DOP_IDENT;

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Record with this value fields &quot;ICAO code/IATA Code/Add Code&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением полей &quot;Код ИКАО/Код ИАТА/Доп.код&quot; уже существует!';
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
        if P_REMARK_IS_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if P_REMARK_IS_EMPTY=0 then
          begin
            select count(id) into R_COUNT
            from WB_REF_RESERVED_PHRASE
            where phrase=to_char(P_REMARK);

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Remark&quot; is a phrase reserved!';
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
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_RUS_SMALL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title short/RUS&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Назв.краткое/RUS&quot; является зарезервированной фразой!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_ENG_SMALL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title short/ENG&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Назв.краткое/ENG&quot; является зарезервированной фразой!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_RUS_FULL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title full/RUS&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Назв.полное/RUS&quot; является зарезервированной фразой!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_ENG_FULL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title full/ENG&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Назв.полное/ENG&quot; является зарезервированной фразой!';
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
        update WB_REF_WS_TYPES
        set U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate(),
	          IATA=P_IATA,
	          ICAO=P_ICAO,
	          DOP_IDENT=P_DOP_IDENT,
	          NAME_RUS_SMALL=P_NAME_RUS_SMALL,
	          NAME_RUS_FULL=P_NAME_RUS_FULL,
	          NAME_ENG_SMALL=P_NAME_ENG_SMALL,
	          NAME_ENG_FULL=P_NAME_ENG_FULL,
	          REMARK=P_REMARK
        where ID=P_ID;

        STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_TYPES_UPDATE;
/
