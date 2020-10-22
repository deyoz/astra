create or replace procedure SP_WB_REF_AC_CLASS_CODE_UPDATE
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

P_DATE_TO date;
P_DATE_TO_D varchar2(50);
P_DATE_TO_M varchar2(50);
P_DATE_TO_Y varchar2(50);

P_CLASS_CODE varchar2(50);
P_PRIORITY_CODE number:=0;
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
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_D[1]') into P_DATE_TO_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_M[1]') into P_DATE_TO_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_Y[1]') into P_DATE_TO_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CLASS_CODE[1]') into P_CLASS_CODE from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PRIORITY_CODE[1]')) into P_PRIORITY_CODE from dual;
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
         from WB_REF_AIRCO_CLASS_CODES
         where ID=P_ID;

         if R_COUNT=0 then
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
         select count(id) into R_COUNT
         from WB_REF_AIRCO_CLASS_CODES
         where ID_AC=P_AIRCO_ID and
               ID<>P_ID and

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

               CLASS_CODE=P_CLASS_CODE;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The record for this class with overlapping date range already existss!';
               end;
             else
               begin
                 str_msg:='Запись для такого класса с пересекающимся диапазоном дат уже существует!';
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
                    str_msg:='Value field "Description" is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля "Описание" является зарезервированной фразой!';
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
        str_msg:='EMPTY_STRING';

        savepoint sp_1;

        update WB_REF_AIRCO_CLASS_CODES
        set CLASS_CODE=P_CLASS_CODE,
	          DATE_FROM=P_DATE_FROM,
	          DATE_TO=P_DATE_TO,
	          PRIORITY_CODE=P_PRIORITY_CODE,
	          DESCRIPTION=P_DESCRIPTION,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where ID=P_ID;

        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        select count(distinct(tt.id)) into R_COUNT
        from WB_REF_AIRCO_CLASS_CODES cc join WB_REF_WS_AIR_SL_CAI_TT tt
        on tt.id_ac=cc.id_ac and
           cc.id=P_ID and
           not exists(select 1
                      from WB_REF_AIRCO_CLASS_CODES ccc
                      where ccc.id_ac=tt.id_ac and
                            ccc.class_code=tt.class_code);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then STR_MSG:='Referenced in blocks "Airline Fleet"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
            if P_LANG='RUS' then STR_MSG:='Имеются ссылки в блоках "Airline Fleet"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
          end;
        end if;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        if str_msg='EMPTY_STRING' then
          begin
            select count(distinct(tt.id)) into R_COUNT
            from WB_REF_WS_AIR_SL_CAI_TT tt
            where tt.id_ac=-1 and
                  not exists(select 1
                             from WB_REF_AIRCO_CLASS_CODES ccc
                             where ccc.class_code=tt.class_code);

            if R_COUNT>0 then
              begin
                if P_LANG='ENG' then STR_MSG:='Referenced in blocks "Types of Aircraft"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
                if P_LANG='RUS' then STR_MSG:='Имеются ссылки в блоках "Types of Aircraft"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
              end;
            end if;
          end;
        end if;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        if str_msg='EMPTY_STRING' then
          begin
            select count(distinct(tt.id)) into R_COUNT
            from WB_REF_AIRCO_CLASS_CODES cc join WB_REF_WS_AIR_SL_CI_T tt
            on tt.id_ac=cc.id_ac and
               cc.id=P_ID and
               not exists(select 1
                          from WB_REF_AIRCO_CLASS_CODES ccc
                          where ccc.id_ac=tt.id_ac and
                                ccc.class_code=tt.class_code);

            if R_COUNT>0 then
              begin
                if P_LANG='ENG' then STR_MSG:='Referenced in blocks "Airline Fleet"->"Seating Layout"->"Configuration"->"Class Area Information"!'; end if;
                if P_LANG='RUS' then STR_MSG:='Имеются ссылки в блоках "Airline Fleet"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
              end;
            end if;
          end;
        end if;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        if str_msg='EMPTY_STRING' then
          begin
            select count(distinct(tt.id)) into R_COUNT
            from WB_REF_WS_AIR_SL_CI_T tt
            where tt.id_ac=-1 and
                  not exists(select 1
                             from WB_REF_AIRCO_CLASS_CODES ccc
                             where ccc.class_code=tt.class_code);

            if R_COUNT>0 then
              begin
                if P_LANG='ENG' then STR_MSG:='Referenced in blocks "Types of Aircraft"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
                if P_LANG='RUS' then STR_MSG:='Имеются ссылки в блоках "Types of Aircraft"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
              end;
            end if;
          end;
        end if;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------

        if str_msg='EMPTY_STRING' then
          begin

             commit;
          end;
        else
          begin

            rollback to sp_1;
          end;
        end if;
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';
  end SP_WB_REF_AC_CLASS_CODE_UPDATE;
/
