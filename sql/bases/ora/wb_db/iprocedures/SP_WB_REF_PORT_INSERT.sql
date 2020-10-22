create or replace procedure SP_WB_REF_PORT_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
p_id_city number;
p_AP varchar2(50):='EMPTY_STRING';
p_IATA varchar2(50):='EMPTY_STRING';
p_IKAO varchar2(50):='EMPTY_STRING';
p_NAME_RUS_SMALL varchar2(50):='';
p_NAME_RUS_FULL varchar2(100):='';
p_NAME_ENG_SMALL varchar2(50):='';
p_NAME_ENG_FULL varchar2(100):='';
REMARK clob:='';
U_NAME varchar2(50):='';
U_IP varchar2(50):='';
U_COMP_NAME varchar2(50):='';
U_HOST_NAME varchar2(50):='';
str_msg varchar2(1000):=null;
r_count int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/CITY_ID[1]') into p_id_city from dual;

    begin
      select extractValue(xmltype(cXML_in),'/root[1]/list[1]/AP[1]') into p_AP from dual;
    exception
      when no_data_FOUND then p_AP:='EMPTY_STRING';
    end;

    begin
      select extractValue(xmltype(cXML_in),'/root[1]/list[1]/IATA[1]') into p_IATA from dual;
    exception
      when no_data_FOUND then p_IATA:='EMPTY_STRING';
    end;

    begin
      select extractValue(xmltype(cXML_in),'/root[1]/list[1]/IKAO[1]') into p_IKAO from dual;
    exception
      when no_data_FOUND then p_IKAO:='EMPTY_STRING';
    end;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_SMALL[1]') into p_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_FULL[1]') into p_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_SMALL[1]') into p_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_FULL[1]') into p_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/REMARK[1]') into REMARK from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_NAME[1]') into U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_IP[1]') into U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_COMP_NAME[1]') into U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_HOST_NAME[1]') into U_HOST_NAME from dual;

    if REMARK='NULL' then
      begin
        REMARK:=null;
      end; end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into r_count
    from WB_REF_CITIES
    where id=p_id_city;

    if r_count=0 then
      begin
        if lang='ENG' then
          begin
            str_msg:='Your city is removed!';
          end;
        else
          begin
            str_msg:='Выбранный город удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(id) into r_count
        from WB_REF_AIRPORTS
        where AP=p_AP and
              IATA=p_IATA;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this combination values fields &quot;AP/IATA&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с такой комбинацией значений полей &quot;AP/IATA&quot; уже существует!';
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
        from WB_REF_AIRPORTS
        where IKAO=p_IKAO;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;IKAO&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;IKAO&quot; уже существует!';
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
        from WB_REF_AIRPORTS
        where NAME_RUS_SMALL=p_NAME_RUS_SMALL;

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
        from WB_REF_AIRPORTS
        where NAME_ENG_SMALL=p_NAME_ENG_SMALL;

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
        from WB_REF_AIRPORTS
        where NAME_RUS_FULL=p_NAME_RUS_FULL;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;Title full/RUS&quot; already exists!';
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
        from WB_REF_AIRPORTS
        where NAME_ENG_FULL=p_NAME_ENG_FULL;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;Title full/ENG&quot; already exists!';
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
        id:=SEC_WB_REF_AIRPORTS.nextval();

        insert into WB_REF_AIRPORTS (ID,
	                                     ID_CITY,
                                         AP,
                                           IATA,
                                             IKAO,
	                                             NAME_RUS_SMALL,
	                                               NAME_RUS_FULL,
	                                                 NAME_ENG_SMALL,
	                                                   NAME_ENG_FULL,
	                                                     REMARK,
	                                                       U_NAME,
	                                                         U_IP,
	                                                           U_HOST_NAME,
	                                                             DATE_WRITE)
        select ID,
	               p_id_city,
                   nvl(p_AP, 'EMPTY_STRING'),
                     nvl(p_IATA, 'EMPTY_STRING'),
                       nvl(p_IKAO, 'EMPTY_STRING'),
	                       p_NAME_RUS_SMALL,
	                         p_NAME_RUS_FULL,
	                           p_NAME_ENG_SMALL,
	                             p_NAME_ENG_FULL,
	                               REMARK,
	                                 U_NAME,
	                                   U_IP,
	                                     U_HOST_NAME,
                                         SYSDATE
        from dual;

        id_hist:=SEC_WB_REF_AIRPORTS_HISTORY.nextval();

        insert into WB_REF_AIRPORTS_HISTORY (ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
	                                                DATE_WRITE_,
	                                                  ACTION,
	                                                    ID,
	                                                      ID_CITY_OLD,
                                                          AP_OLD,
                                                            IATA_OLD,
                                                              IKAO_OLD,
	                                                              NAME_RUS_SMALL_OLD,
	                                                                NAME_RUS_FULL_OLD,
	                                                                  NAME_ENG_SMALL_OLD,
	                                                                    NAME_ENG_FULL_OLD,
	                                                                      REMARK_OLD,
	                                                                        U_NAME_OLD,
	                                                                          U_IP_OLD,
	                                                                            U_HOST_NAME_OLD,
	                                                                              DATE_WRITE_OLD,
	                                                                                ID_CITY_NEW,
                                                                                    AP_NEW,
                                                                                      IATA_NEW,
                                                                                        IKAO_NEW,
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
                 U_NAME,
	                 U_IP,
	                   U_HOST_NAME,
                       SYSDATE,
                         'insert',
                            id,
                              p_id_city,
                                nvl(p_AP, 'EMPTY_STRING'),
                                  nvl(p_IATA, 'EMPTY_STRING'),
                                    nvl(p_IKAO, 'EMPTY_STRING'),
	                                    p_NAME_RUS_SMALL,
	                                      p_NAME_RUS_FULL,
	                                        p_NAME_ENG_SMALL,
	                                          p_NAME_ENG_FULL,
	                                            REMARK,
	                                              U_NAME,
	                                                U_IP,
	                                                  U_HOST_NAME,
                                                      SYSDATE,
                                                        p_id_city,
                                                          nvl(p_AP, 'EMPTY_STRING'),
                                                            nvl(p_IATA, 'EMPTY_STRING'),
                                                              nvl(p_IKAO, 'EMPTY_STRING'),
	                                                              p_NAME_RUS_SMALL,
	                                                                p_NAME_RUS_FULL,
	                                                                  p_NAME_ENG_SMALL,
	                                                                    p_NAME_ENG_FULL,
	                                                                      REMARK,
	                                                                        U_NAME,
	                                                                          U_IP,
	                                                                            U_HOST_NAME,
                                                                                SYSDATE
        from dual;
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(id)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_PORT_INSERT;
/
