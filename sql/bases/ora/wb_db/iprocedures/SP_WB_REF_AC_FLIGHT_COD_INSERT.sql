create or replace procedure SP_WB_REF_AC_FLIGHT_COD_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';

P_DATE_FROM date:=null;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_DATE_TO date:=null;
P_DATE_TO_D varchar2(50);
P_DATE_TO_M varchar2(50);
P_DATE_TO_Y varchar2(50);

P_CODE_NAME varchar2(50);
P_DESCRIPTION varchar2(1000);
P_IS_DESCRIPTION_EMPTY number:=0;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_D[1]') into P_DATE_TO_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_M[1]') into P_DATE_TO_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_Y[1]') into P_DATE_TO_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CODE_NAME[1]') into P_CODE_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DESCRIPTION[1]') into P_DESCRIPTION from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_DESCRIPTION_EMPTY[1]')) into P_IS_DESCRIPTION_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_DATE_FROM_D>-1
      then P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    end if;

    if P_DATE_TO_D>-1
      then P_DATE_TO:=to_date(P_DATE_TO_D||'.'||P_DATE_TO_M||'.'||P_DATE_TO_Y, 'dd.mm.yyyy');
    end if;

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
         select count(id) into R_COUNT
         from WB_REF_AIRCO_FLIGHT_CODES
         where ID_AC=P_AIRCO_ID and

               (nvl(DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy'))
                  between nvl(P_DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy')) and
                            nvl(P_DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy')) or

                nvl(DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy'))
                  between nvl(P_DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy')) and
                            nvl(P_DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy')) or

                nvl(P_DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy'))
                  between nvl(DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy')) and
                            nvl(DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy')) or

                nvl(P_DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy'))
                  between nvl(DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy')) and
                            nvl(DATE_TO, to_date('01.01.9999', 'dd.mm.yyyy'))) and

               CODE_NAME=P_CODE_NAME;

         if r_count>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The record for this Flight Type with overlapping date range already existss!';
               end;
             else
               begin
                 str_msg:='Запись для такого типа с пересекающимся диапазоном дат уже существует!';
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
        if P_IS_DESCRIPTION_EMPTY=1 then
          begin

            P_DESCRIPTION:='EMPTY_STRING';
          end;
        end if;

        if P_IS_DESCRIPTION_EMPTY=0 then
          begin
            select count(id) into r_count
            from WB_REF_RESERVED_PHRASE
            where phrase=P_DESCRIPTION;

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Description&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Описание&quot; является зарезервированной фразой!';
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
        id:=SEC_WB_REF_AIRCO_FLIGHT_CODES.nextval();

        insert into WB_REF_AIRCO_FLIGHT_CODES (ID,
                                                 ID_AC,
	                                                 CODE_NAME,	
	                                                   DATE_FROM,
	                                                     DATE_TO,
	                                                       DESCRIPTION,
	                                                         U_NAME,
	                                                           U_IP,
	                                                             U_HOST_NAME,
	                                                               DATE_WRITE)
        select id,
                 P_AIRCO_ID,
                   P_CODE_NAME,	
	                   P_DATE_FROM,
	                     P_DATE_TO,
	                       P_DESCRIPTION,
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
  end SP_WB_REF_AC_FLIGHT_COD_INSERT;
/
