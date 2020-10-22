create or replace procedure SP_WB_REF_AC_MISC_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
P_REMARK clob:='';
P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);
P_IS_REMARK_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
r_count number:=-1;
p_id number:=-1;

TYPE DEL_REF_REC IS REF CURSOR;
CUR_DEL_REF_REC DEL_REF_REC;
DEL_REF_NAME varchar2(200);
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_AIRCO_ID[1]') into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//user/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num,
                                    STRING_VAL)
    select distinct f.id,
                      f.id,
                        f.misc_name
    from (select to_number(extractValue(value(t),'list/P_MISC_ID[1]')) as id,
                 extractValue(value(t),'list/P_MISC_NAME[1]') as misc_name
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;
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
         from WB_REF_AIRCO_MISC_ADV
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
    if str_msg is null then
      begin
        select count(num) into r_count
        from WB_TEMP_XML_ID
        where not exists(select 1
                         from WB_REF_AIRCO_MISC_ITEMS d
                         where d.id=WB_TEMP_XML_ID.id);

        if r_count>0 then
          begin
            if P_LANG='RUS' then
              begin
                str_msg:='Следущие наименования удалены из справочника:'||chr(10)||chr(10);
              end;
            else
              begin
                str_msg:='Next name removed from the directory:'||chr(10)||chr(10);
              end;
          end if;

            open CUR_DEL_REF_REC
            for 'select distinct string_val
                 from WB_TEMP_XML_ID
                 where not exists(select 1
                                  from WB_REF_AIRCO_MISC_ITEMS d
                                  where d.id=WB_TEMP_XML_ID.id)
                 order by string_val';

            LOOP
              FETCH CUR_DEL_REF_REC INTO DEL_REF_NAME;
              EXIT WHEN CUR_DEL_REF_REC%NOTFOUND;

              str_msg:=str_msg||' - '||
                         DEL_REF_NAME||
                           chr(10);

            END LOOP;
            CLOSE CUR_DEL_REF_REC;

            str_msg:=str_msg||chr(10);

            if P_LANG='RUS' then
              begin
                str_msg:=str_msg||'Операция запрещена!';
              end;
            else
              begin
                str_msg:=str_msg||'Operation is prohibited!';
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
        P_ID:=SEC_WB_REF_AIRC_MISC_ADV.nextval();

        insert into WB_REF_AIRCO_MISC_ADV (ID,
	                                           ID_AC,
	                                             DATE_FROM,
	                                               REMARK,
	                                                 U_NAME,
	                                                   U_IP,
	                                                     U_HOST_NAME,
	                                                       DATE_WRITE)
        select p_id,
                 P_AIRCO_ID,
	                 P_DATE_FROM,
	                   P_REMARK,
	                     P_U_NAME,
	                       P_U_IP,
	                         P_U_HOST_NAME,
                             sysdate()
        from dual;

        insert into WB_REF_AIRCO_MISC_DET (ID,
	                                           ID_AC,
	                                             ADV_ID,
	                                               MISC_ID,
	                                                 U_NAME,
	                                                   U_IP,
	                                                     U_HOST_NAME,
	                                                       DATE_WRITE)
        select SEC_WB_REF_AIRC_MISC_DET.nextval,
                 P_AIRCO_ID,
                   P_ID,
                     ID,
                       P_U_NAME,
	                       P_U_IP,
	                         P_U_HOST_NAME,
                             sysdate()
        from WB_TEMP_XML_ID;

        str_msg:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list id="'||to_char(p_id)||'" str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_MISC_INSERT;
/
