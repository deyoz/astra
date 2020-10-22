create or replace procedure SP_WB_REF_AC_DOC_H_F_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
P_DOC_ID number:=-1;
P_HEADER clob:='';
P_FOOTER clob:='';
P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);
P_IS_HEADER_EMPTY number:=0;
P_IS_FOOTER_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
r_count number:=-1;
P_ID number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]') into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOC_ID[1]') into P_DOC_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_HEADER_EMPTY[1]')) into P_IS_HEADER_EMPTY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_FOOTER_EMPTY[1]')) into P_IS_FOOTER_EMPTY from dual;

    select value(t).extract('/P_HEADER/text()').getclobval() into P_HEADER
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_HEADER/text()').getclobval() is not null;

    select value(t).extract('/P_FOOTER/text()').getclobval() into P_FOOTER
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_FOOTER/text()').getclobval() is not null;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
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
         from WB_REF_DOC_TYPE_LIST
         where ID=P_DOC_ID;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected document is deleted from the directory!';
               end;
             else
               begin
                 str_msg:='Выбранный документ удален из справочника!';
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
         from WB_REF_AIRCO_DOCS_HEAD_FOOT
         where ID_AC=P_AIRCO_ID and
               DOC_ID=P_DOC_ID and
               DATE_FROM=P_DATE_FROM;

         if r_count>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Record with the date for the selected document already exists!';
               end;
             else
               begin
                 str_msg:='Запись с такой датой для выбранного документа уже существует!';
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
        if P_IS_HEADER_EMPTY=1 then
          begin

            P_HEADER:='EMPTY_STRING';
          end;
        end if;

        if P_IS_HEADER_EMPTY=0 then
          begin
            if P_HEADER='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Header&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Заголовок&quot; является зарезервированной фразой!';
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
        if P_IS_FOOTER_EMPTY=1 then
          begin

            P_FOOTER:='EMPTY_STRING';
          end;
        end if;

        if P_IS_FOOTER_EMPTY=0 then
          begin
            if P_FOOTER='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Footer&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Подвал&quot; является зарезервированной фразой!';
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
        P_ID:=SEC_WB_REF_AC_DOCS_HEAD_FOOT.nextval();

        insert into WB_REF_AIRCO_DOCS_HEAD_FOOT (ID,
	                                                 ID_AC,
	                                                   DOC_ID,
	                                                     DATE_FROM,
	                                                       HEADER,
	                                                         FOOTER,
	                                                           U_NAME,
	                                                             U_IP,
	                                                               U_HOST_NAME,
	                                                                 DATE_WRITE)
        select P_ID,
                 P_AIRCO_ID,
                   P_DOC_ID,
	                   P_DATE_FROM,
	                     P_HEADER,
                         P_FOOTER,
	                         P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 sysdate()
        from dual;

        str_msg:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_DOC_H_F_INSERT;
/
