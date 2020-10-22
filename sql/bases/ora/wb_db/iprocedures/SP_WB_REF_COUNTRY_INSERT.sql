create or replace procedure SP_WB_REF_COUNTRY_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
p_CC_R varchar2(50):='';
p_CC_E varchar2(50):='';
p_NAME_RUS_SMALL varchar2(50):='';
p_NAME_RUS_FULL varchar2(100):='';
p_NAME_ENG_SMALL varchar2(50):='';
p_NAME_ENG_FULL varchar2(100):='';
REMARK varchar2(2000):='';
U_NAME varchar2(50):='';
U_IP varchar2(50):='';
U_COMP_NAME varchar2(50):='';
U_HOST_NAME varchar2(50):='';
flag clob:=null;
str_msg varchar2(1000):=null;
r_count int:=0;
rs char(2):=
'
';
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/CC_R[1]') into p_CC_R from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/CC_E[1]') into p_CC_E from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_SMALL[1]') into p_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_RUS_FULL[1]') into p_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_SMALL[1]') into p_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/NAME_ENG_FULL[1]') into p_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/REMARK[1]') into REMARK from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_NAME[1]') into U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_IP[1]') into U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_COMP_NAME[1]') into U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_HOST_NAME[1]') into U_HOST_NAME from dual;
    --select extractValue(xmltype(cXML_in),'/root[1]/list[1]/FLAG[1]') into FLAG from dual;

    select value(t).extract('/FLAG/text()').getclobval() into FLAG
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/FLAG/text()').getclobval() is not null;

    if REMARK='NULL' then
      begin
        REMARK:=null;
      end; end if;

    if flag='NULL' then
      begin
        flag:=null;
      end; end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into r_count
    from WB_REF_COUNTRY
    where cc_r=p_cc_r;

    if r_count>0 then
      begin
        if lang='ENG' then
          begin
            str_msg:='Record with this value field &quot;Country code/RUS&quot; already exists!';
          end;
        else
          begin
            str_msg:='Запись с таким значением поля &quot;Код страны/RUS&quot; уже существует!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(id) into r_count
        from WB_REF_COUNTRY
        where cc_e=p_cc_e;

        if r_count>0 then
          begin
            if lang='ENG' then
              begin
                str_msg:='Record with this value field &quot;Country code/ENG&quot; already exists!';
              end;
            else
              begin
                str_msg:='Запись с таким значением поля &quot;Код страны/ENG&quot; уже существует!';
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
        from WB_REF_COUNTRY
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
        from WB_REF_COUNTRY
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
        from WB_REF_COUNTRY
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
        from WB_REF_COUNTRY
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
        id:=SEC_WB_REF_COUNTRY.nextval();

        insert into WB_REF_COUNTRY (ID,
	                                    CC_R,
	                                      CC_E,
	                                        NAME_RUS_SMALL,
	                                          NAME_RUS_FULL,
	                                            NAME_ENG_SMALL,
	                                              NAME_ENG_FULL,
	                                                REMARK,
	                                                  U_NAME,
	                                                    U_IP,
	                                                      U_HOST_NAME,
	                                                        DATE_WRITE,
	                                                          FLAG)
        select ID,
	               p_CC_R,
	                 p_CC_E,
	                   p_NAME_RUS_SMALL,
	                     p_NAME_RUS_FULL,
	                       p_NAME_ENG_SMALL,
	                         p_NAME_ENG_FULL,
	                           REMARK,
	                             U_NAME,
	                               U_IP,
	                                 U_HOST_NAME,
                                     SYSDATE,
                                       flag
        from dual;

        id_hist:=SEC_WB_REF_COUNTRY_HISTORY.nextval();

        insert into WB_REF_COUNTRY_HISTORY(ID_,
	                                           U_NAME_,
	                                             U_IP_,
	                                               U_HOST_NAME_,
	                                                 DATE_WRITE_,
	                                                   ID,
	                                                     CC_R_OLD,
	                                                       CC_E_OLD,
	                                                         NAME_RUS_SMALL_OLD,
	                                                           NAME_RUS_FULL_OLD,
	                                                             NAME_ENG_SMALL_OLD,
	                                                               NAME_ENG_FULL_OLD,
	                                                                 REMARK_OLD,
	                                                                   U_NAME_OLD,
	                                                                     U_IP_OLD,
	                                                                       U_HOST_NAME_OLD,
	                                                                         DATE_WRITE_OLD,
	                                                                           CC_R_NEW,
	                                                                             CC_E_NEW,
	                                                                               NAME_RUS_SMALL_NEW,
	                                                                                 NAME_RUS_FULL_NEW,
	                                                                                   NAME_ENG_SMALL_NEW,
	                                                                                     NAME_ENG_FULL_NEW,
	                                                                                       REMARK_NEW,	
	                                                                                         U_NAME_NEW,
	                                                                                           U_IP_NEW,
	                                                                                             U_HOST_NAME_NEW,
	                                                                                               DATE_WRITE_NEW,
	                                                                                                 ACTION,
                                                                                                     FLAG_OLD,
                                                                                                       FLAG_NEW)
        select id_hist,
                 U_NAME,
	                 U_IP,
	                   U_HOST_NAME,
                       SYSDATE,
                         id,
                           p_CC_R,
	                           p_CC_E,
	                             p_NAME_RUS_SMALL,
	                               p_NAME_RUS_FULL,
	                                 p_NAME_ENG_SMALL,
	                                   p_NAME_ENG_FULL,
	                                     REMARK,
	                                       U_NAME,
	                                         U_IP,
	                                           U_HOST_NAME,
                                               SYSDATE,
                                                 p_CC_R,
	                                                 p_CC_E,
	                                                   p_NAME_RUS_SMALL,
	                                                     p_NAME_RUS_FULL,
	                                                       p_NAME_ENG_SMALL,
	                                                         p_NAME_ENG_FULL,
	                                                           REMARK,
	                                                             U_NAME,
	                                                               U_IP,
	                                                                 U_HOST_NAME,
                                                                     SYSDATE,
                                                                       'insert',
                                                                         flag,
                                                                           flag
        from dual;
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(id)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_COUNTRY_INSERT;
/
