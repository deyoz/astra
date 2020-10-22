create or replace procedure SP_WB_REF_AC_C_DATA_2_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_AR varchar2(200):='';
P_E_MAIL_ADRESS varchar2(200):='';
p_TELETYPE_ADRESS varchar2(200):='';
P_PHONE_NUMBER varchar2(100):='';
P_FAX_NUMBER varchar2(50):='';
P_REMARK clob:='';

P_IS_AR_EMPTY number:=0;
P_IS_E_MAIL_ADRESS_EMPTY number:=0;
P_IS_TELETYPE_ADRESS_EMPTY number:=0;
P_IS_PHONE_NUMBER_EMPTY number:=0;
P_IS_FAX_NUMBER_EMPTY number:=0;
P_IS_REMARK_EMPTY number:=0;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
r_count int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_AR_EMPTY[1]')) into p_IS_AR_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_E_MAIL_ADRESS_EMPTY[1]')) into p_IS_E_MAIL_ADRESS_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_TELETYPE_ADRESS_EMPTY[1]')) into p_IS_TELETYPE_ADRESS_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_PHONE_NUMBER_EMPTY[1]')) into P_IS_PHONE_NUMBER_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_FAX_NUMBER_EMPTY[1]')) into P_IS_FAX_NUMBER_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AR[1]') into p_AR from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_E_MAIL_ADRESS[1]') into P_E_MAIL_ADRESS from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TELETYPE_ADRESS[1]') into p_TELETYPE_ADRESS from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PHONE_NUMBER[1]') into P_PHONE_NUMBER from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FAX_NUMBER[1]') into P_FAX_NUMBER from dual;


   -- select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK[1]') into P_REMARK from dual;

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
    select count(id) into r_count
    from WB_REF_AIRCOMPANY_KEY
    where id=P_AIRCO_ID;

    if r_count=0 then
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
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_AIRCO_C_DATA_2
         where ID_AC=P_AIRCO_ID and
               DATE_FROM=P_DATE_FROM;

         if r_count>0 then
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
        if p_IS_AR_EMPTY=1 then
          begin

            P_AR:='EMPTY_STRING';
          end;
        end if;

        if p_IS_AR_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_AR;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Airport Representative&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Представитель в аэропорте&quot; является зарезервированной фразой!';
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
        if p_IS_E_MAIL_ADRESS_EMPTY=1 then
          begin

            P_E_MAIL_ADRESS:='EMPTY_STRING';
          end;
        end if;

        if p_IS_E_MAIL_ADRESS_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_E_MAIL_ADRESS;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;E-mail Address&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;E-mail&quot; является зарезервированной фразой!';
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
        if p_IS_TELETYPE_ADRESS_EMPTY=1 then
          begin

            P_TELETYPE_ADRESS:='EMPTY_STRING';
          end;
        end if;

        if p_IS_TELETYPE_ADRESS_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_TELETYPE_ADRESS;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Teletype Address&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Телетайп&quot; является зарезервированной фразой!';
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
        if p_IS_PHONE_NUMBER_EMPTY=1 then
          begin

            P_PHONE_NUMBER:='EMPTY_STRING';
          end;
        end if;

        if p_IS_PHONE_NUMBER_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_PHONE_NUMBER;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Phone Number&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Телефон&quot; является зарезервированной фразой!';
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
        if p_IS_FAX_NUMBER_EMPTY=1 then
          begin

            P_FAX_NUMBER:='EMPTY_STRING';
          end;
        end if;

        if p_IS_PHONE_NUMBER_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_FAX_NUMBER;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Fax Number&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Факс&quot; является зарезервированной фразой!';
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

            P_REMARK:=null;
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
        id:=SEC_WB_REF_AIRCO_C_DATA_2.nextval();

        insert into WB_REF_AIRCO_C_DATA_2 (ID,
                                             ID_AC,
	                                             DATE_FROM,
	                                               AR,	 	
		                                               E_MAIL_ADRESS,
	 	                                                 TELETYPE_ADRESS,
	 	                                                   PHONE_NUMBER,
	                                                 	     FAX_NUMBER,
	                                               	         REMARK,
	                                               	           U_NAME,
	                                               	             U_IP,
	                                               	               U_HOST_NAME,
	                                               	                 DATE_WRITE)
        select id,
                 P_AIRCO_ID,
                   P_DATE_FROM,
                     P_AR,	 	
		                   P_E_MAIL_ADRESS,
	 	                     P_TELETYPE_ADRESS,
	 	                       P_PHONE_NUMBER,
	                           P_FAX_NUMBER,
	                             P_REMARK,
                                 P_U_NAME,
	                                 P_U_IP,
	                                   P_U_HOST_NAME,
                                       sysdate()

        from dual;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(id)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_C_DATA_2_INSERT;
/
