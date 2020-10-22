create or replace procedure SP_WB_REF_AIRCO_ADV_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_AIRCO_ID number:=-1;
lang varchar2(50):='';
r_count int:=0;
P_DATE_FROM date;
P_DATE_TO date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);
p_NAME_RUS_SMALL varchar2(50):='';
p_NAME_RUS_FULL varchar2(100):='';
p_NAME_ENG_SMALL varchar2(50):='';
p_NAME_ENG_FULL varchar2(100):='';
P_IATA_CODE varchar2(100):='';
P_ICAO_CODE varchar2(50):='';
P_OTHER_CODE varchar2(100):='';
P_ID_CITY number:=-1;
P_REMARK varchar2(2000):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]') into P_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]') into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_SMALL[1]') into P_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_FULL[1]') into P_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_SMALL[1]') into P_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_FULL[1]') into P_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK[1]') into P_REMARK from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IATA_CODE[1]') into P_IATA_CODE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ICAO_CODE[1]') into P_ICAO_CODE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_OTHER_CODE[1]') into P_OTHER_CODE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_CITY[1]') into P_ID_CITY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_REMARK='NULL' then
      begin
        P_REMARK:=null;
      end; end if;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into r_count
    from WB_REF_AIRCOMPANY_KEY
    where id=P_AIRCO_ID;

    if r_count=0 then
      begin
        if lang='ENG' then
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
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(id) into r_count
        from WB_REF_AIRCOMPANY_ADV_INFO
        where id=P_ID;

        if r_count=0 then
          begin
            if lang='ENG' then
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
    end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_AIRCOMPANY_ADV_INFO
         where ID_AC=P_AIRCO_ID and
               ID<>P_ID and
               DATE_FROM=P_DATE_FROM;

         if r_count>0 then
           begin
             if lang='ENG' then
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
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=p_NAME_RUS_SMALL;

         if r_count>0 then
           begin
             if lang='ENG' then
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
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=p_NAME_ENG_SMALL;

         if r_count>0 then
           begin
             if lang='ENG' then
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
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=p_NAME_RUS_FULL;

         if r_count>0 then
           begin
             if lang='ENG' then
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
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=p_NAME_ENG_FULL;

         if r_count>0 then
           begin
             if lang='ENG' then
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
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=P_IATA_CODE;

         if r_count>0 then
           begin
             if lang='ENG' then
               begin
                 str_msg:='Value field &quot;IATA Code&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Код IATA&quot; является зарезервированной фразой!';
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
         where phrase=P_ICAO_CODE;

         if r_count>0 then
           begin
             if lang='ENG' then
               begin
                 str_msg:='Value field &quot;ICAO Code&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Код ICAO&quot; является зарезервированной фразой!';
               end;
             end if;
           end;
          end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    /*
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=P_OTHER_CODE;

         if r_count>0 then
           begin
             if lang='ENG' then
               begin
                 str_msg:='Value field &quot;Other Code&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Другой код&quot; является зарезервированной фразой!';
               end;
             end if;
           end;
          end if;
      end;
    end if;
    */
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if (str_msg is null) and (P_REMARK is not null) then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=P_REMARK;

         if r_count>0 then
           begin
             if lang='ENG' then
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
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if (str_msg is null) and (P_ID_CITY>-1) then
      begin
         select count(id) into r_count
         from WB_REF_CITIES
         where ID=P_ID_CITY;

         if r_count=0 then
           begin
             if lang='ENG' then
               begin
                 str_msg:='This city is removed!';
               end;
             else
               begin
                 str_msg:='Выбранный город удален!';
               end;
             end if;
           end;
          end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select nvl(q.date_to, to_date('31.12.9999', 'dd.mm.yyyy')) into P_DATE_TO
    from (select min(date_from)-1 date_to
          from WB_REF_AIRCOMPANY_ADV_INFO
          where id_ac=P_AIRCO_ID and
                date_from>P_DATE_FROM) q;

    if (str_msg is null) then
      begin
        select count(q.id_ac) into r_count
        from (select i.id_ac,
                     i.date_from,
                     nvl((select min(ii.date_from)-1
                          from WB_REF_AIRCOMPANY_ADV_INFO ii
                          where ii.id_ac=i.id_ac and
                                ii.date_from>i.date_from), to_date('01.01.9999', 'dd.mm.yyyy')) date_to,
                     i.iata_code
            from WB_REF_AIRCOMPANY_ADV_INFO i
            where i.id_ac<>P_AIRCO_ID
            order by i.id_ac,
                     i.date_from) q
        where q.iata_code=P_IATA_CODE and
              (q.date_from between P_DATE_FROM and P_DATE_TO or
               q.date_to  between P_DATE_FROM and P_DATE_TO or
               P_DATE_FROM between q.date_from and q.date_to or
               P_DATE_TO between q.date_from and q.date_to);


        if r_count>0 then
          begin
            if lang='ENG' then
               begin
                 str_msg:='There are airlines with IATA code in the specified period of!';
               end;
             else
               begin
                 str_msg:='Имеются авиакомпании с таким кодом IATA в указанный период действия!';
               end;
            end if;
          end;
        end if;
      end;
    end if;

    if (str_msg is null) then
      begin
        select count(q.id_ac) into r_count
        from (select i.id_ac,
                     i.date_from,
                     nvl((select min(ii.date_from)-1
                          from WB_REF_AIRCOMPANY_ADV_INFO ii
                          where ii.id_ac=i.id_ac and
                                ii.date_from>i.date_from), to_date('01.01.9999', 'dd.mm.yyyy')) date_to,
                     i.icao_code
            from WB_REF_AIRCOMPANY_ADV_INFO i
            where i.id_ac<>P_AIRCO_ID
            order by i.id_ac,
                     i.date_from) q
        where q.icao_code=P_ICAO_CODE and
              (q.date_from between P_DATE_FROM and P_DATE_TO or
               q.date_to  between P_DATE_FROM and P_DATE_TO or
               P_DATE_FROM between q.date_from and q.date_to or
               P_DATE_TO between q.date_from and q.date_to);


        if r_count>0 then
          begin
            if lang='ENG' then
               begin
                 str_msg:='There are airlines with ICAO code in the specified period of!';
               end;
             else
               begin
                 str_msg:='Имеются авиакомпании с таким кодом ICAO в указанный период действия!';
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
        update WB_REF_AIRCOMPANY_ADV_INFO
        set date_from=P_DATE_FROM,
            id_city=P_ID_CITY,
            IATA_CODE=P_IATA_CODE,
	          ICAO_CODE=P_ICAO_CODE,
	          OTHER_CODE=P_OTHER_CODE,
	          NAME_RUS_SMALL=P_NAME_RUS_SMALL,
	          NAME_RUS_FULL=P_NAME_RUS_FULL,
	          NAME_ENG_SMALL=P_NAME_ENG_SMALL,
	          NAME_ENG_FULL=P_NAME_ENG_FULL,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate(),
            remark=P_REMARK
        where id=P_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_ADV_UPDATE;
/
