create or replace procedure SP_WB_REF_AC_PORT_FLT_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_ID_PORT number:=-1;
P_UTC_DIFF number:=0;
P_IS_CHECK_IN number:=-1;
P_IS_LOAD_CONTROL number:=-1;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
r_count int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]') into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_PORT[1]')) into P_ID_PORT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_UTC_DIFF[1]')) into P_UTC_DIFF from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_CHECK_IN[1]')) into P_IS_CHECK_IN from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_LOAD_CONTROL[1]')) into P_IS_LOAD_CONTROL from dual;

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
         from WB_REF_AIRCO_PORT_FLIGHT
         where ID=P_ID;

         if r_count=0 then
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
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_AIRPORTS
         where ID=P_ID_PORT;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='This airport is removed!';
               end;
             else
               begin
                 str_msg:='Этот аэропорт удален!';
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
         from WB_REF_AIRCO_PORT_FLIGHT
         where ID<>P_ID and
               ID_AC=P_AIRCO_ID and
               DATE_FROM=P_DATE_FROM and
               ID_PORT=P_ID_PORT;

         if r_count>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Record on this date for this airport already exists!';
               end;
             else
               begin
                 str_msg:='Запись с такой датой для этого аэропорта уже существует!';
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
        update WB_REF_AIRCO_PORT_FLIGHT
        set ID_PORT=P_ID_PORT,
            DATE_FROM=P_DATE_FROM,
            UTC_DIFF=P_UTC_DIFF,
            IS_CHECK_IN=P_IS_CHECK_IN,
	          IS_LOAD_CONTROL=P_IS_LOAD_CONTROL,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where ID=P_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_PORT_FLT_UPDATE;
/
