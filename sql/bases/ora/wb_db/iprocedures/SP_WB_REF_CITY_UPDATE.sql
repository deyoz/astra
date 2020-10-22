create or replace procedure SP_WB_REF_CITY_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
rec_id number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
p_id_country number;
p_NAME_RUS_SMALL varchar2(50):='';
p_NAME_RUS_FULL varchar2(100):='';
p_NAME_ENG_SMALL varchar2(50):='';
p_NAME_ENG_FULL varchar2(100):='';
p_REMARK clob:='';
p_U_NAME varchar2(50):='';
p_U_IP varchar2(50):='';
p_U_COMP_NAME varchar2(50):='';
p_U_HOST_NAME varchar2(50):='';
str_msg varchar2(1000):=null;
r_count int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/id[1]') into rec_id from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/COUNTRY_ID[1]') into p_id_country from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_SMALL[1]') into p_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_FULL[1]') into p_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_SMALL[1]') into p_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_FULL[1]') into p_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/REMARK[1]') into p_REMARK from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_NAME[1]') into p_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_IP[1]') into p_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_COMP_NAME[1]') into p_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_HOST_NAME[1]') into p_U_HOST_NAME from dual;

    if p_REMARK='NULL' then
      begin
        p_REMARK:=null;
      end; end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into r_count
    from WB_REF_CITIES
    where id=rec_id;

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
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(id) into r_count
        from WB_REF_COUNTRY
        where id=p_id_country;

        if r_count=0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Your country is removed!';
              end;
            else
              begin
                str_msg:='Выбранная страна удалена!';
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
        from WB_REF_CITIES
        where NAME_RUS_SMALL=p_NAME_RUS_SMALL and
              id<>rec_id;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;Title short/RUS&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;Назв.краткое/RUS&quot; уже существует!';
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
        from WB_REF_CITIES
        where NAME_ENG_SMALL=p_NAME_ENG_SMALL and
              id<>rec_id;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;Title short/ENG&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;Назв.краткое/ENG&quot; уже существует!';
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
        from WB_REF_CITIES
        where NAME_RUS_FULL=p_NAME_RUS_FULL and
              id<>rec_id;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Recording with this value field &quot;Title full/RUS&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;Назв.полное/RUS&quot; уже существует!';
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
        from WB_REF_CITIES
        where NAME_ENG_FULL=p_NAME_ENG_FULL and
              id<>rec_id;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Recording with this value field &quot;Title full/ENG&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;Назв.полное/ENG&quot; уже существует!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
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

    if (str_msg is null) then
      begin
        id_hist:=SEC_WB_REF_CITIES_HISTORY.nextval();

        insert into WB_REF_CITIES_HISTORY (ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
	                                                DATE_WRITE_,
	                                                  ACTION,
	                                                    ID,
	                                                      ID_COUNTRY_OLD,
	                                                        NAME_RUS_SMALL_OLD,
	                                                          NAME_RUS_FULL_OLD,
	                                                            NAME_ENG_SMALL_OLD,
	                                                              NAME_ENG_FULL_OLD,
	                                                                REMARK_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
	                                                                      U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,
	                                                                          ID_COUNTRY_NEW,
	                                                                            NAME_RUS_SMALL_NEW,
	                                                                              NAME_RUS_FULL_NEW,
	                                                                                NAME_ENG_SMALL_NEW,
	                                                                                  NAME_ENG_FULL_NEW,
	                                                                                    REMARK_NEW,
	                                                                                      U_NAME_NEW,
	                                                                                        U_IP_NEW,
	                                                                                          U_HOST_NAME_NEW,
	                                                                                            DATE_WRITE_NEW)
        select id_hist,
                 p_U_NAME,
	                 p_U_IP,
	                   p_U_HOST_NAME,
                       SYSDATE,
                         'update',
                            rec_id,
                              id_country,
	                              NAME_RUS_SMALL,
	                                NAME_RUS_FULL,
	                                  NAME_ENG_SMALL,
	                                    NAME_ENG_FULL,
	                                      REMARK,
	                                        U_NAME,
	                                          U_IP,
	                                            U_HOST_NAME,
                                                SYSDATE,
                                                  p_id_country,
	                                                  p_NAME_RUS_SMALL,
	                                                    p_NAME_RUS_FULL,
	                                                      p_NAME_ENG_SMALL,
	                                                        p_NAME_ENG_FULL,
	                                                          p_REMARK,
	                                                            p_U_NAME,
	                                                              p_U_IP,
	                                                                p_U_HOST_NAME,
                                                                    SYSDATE
        from WB_REF_CITIES
        where id=rec_id;


        update WB_REF_CITIES
	      set ID_COUNTRY=p_ID_COUNTRY,	
	          NAME_RUS_SMALL=p_NAME_RUS_SMALL,
	          NAME_RUS_FULL=p_NAME_RUS_FULL,
	          NAME_ENG_SMALL=p_NAME_ENG_SMALL,
	          NAME_ENG_FULL=p_NAME_ENG_FULL,
	          REMARK=p_REMARK,
	          U_NAME=p_U_NAME,
	          U_IP=p_U_IP,
	          U_HOST_NAME=p_U_HOST_NAME,
	          DATE_WRITE=sysdate
        where id=rec_id;

      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_CITY_UPDATE;
/
