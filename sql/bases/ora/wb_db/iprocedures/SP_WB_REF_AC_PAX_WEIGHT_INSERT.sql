create or replace procedure SP_WB_REF_AC_PAX_WEIGHT_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
P_CLASS_ID number:=-1;
P_DATE_FROM date:=null;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);
P_ADULT NUMBER:=null;
P_ADULT_INT_PART varchar2(50);
P_ADULT_DEC_PART varchar2(50);
P_MALE NUMBER:=null;
P_MALE_INT_PART varchar2(50);
P_MALE_DEC_PART varchar2(50);
P_FEMALE NUMBER:=null;
P_FEMALE_INT_PART varchar2(50);
P_FEMALE_DEC_PART varchar2(50);
P_CHILD NUMBER:=null;
P_CHILD_INT_PART varchar2(50);
P_CHILD_DEC_PART varchar2(50);
P_INFANT NUMBER:=null;
P_INFANT_INT_PART varchar2(50);
P_INFANT_DEC_PART varchar2(50);
P_HAND_BAG NUMBER:=null;
P_HAND_BAG_INT_PART varchar2(50);
P_HAND_BAG_DEC_PART varchar2(50);
P_HAND_BAG_INCLUDE NUMBER:=null;
P_BY_DEFAULT NUMBER:=1;
P_DESCRIPTION varchar2(1000);
P_IS_DESCRIPTION_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
STR_MSG clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CLASS_ID[1]')) into P_CLASS_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADULT_INT_PART[1]') into P_ADULT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADULT_DEC_PART[1]') into P_ADULT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MALE_INT_PART[1]') into P_MALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MALE_DEC_PART[1]') into P_MALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FEMALE_INT_PART[1]') into P_FEMALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FEMALE_DEC_PART[1]') into P_FEMALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CHILD_INT_PART[1]') into P_CHILD_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CHILD_DEC_PART[1]') into P_CHILD_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INFANT_INT_PART[1]') into P_INFANT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INFANT_DEC_PART[1]') into P_INFANT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HAND_BAG_INT_PART[1]') into P_HAND_BAG_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HAND_BAG_DEC_PART[1]') into P_HAND_BAG_DEC_PART from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HAND_BAG_INCLUDE[1]')) into P_HAND_BAG_INCLUDE from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BY_DEFAULT[1]')) into P_BY_DEFAULT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DESCRIPTION[1]') into P_DESCRIPTION from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_DESCRIPTION_EMPTY[1]')) into P_IS_DESCRIPTION_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_DATE_FROM_D>-1
      then P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    end if;

    if P_ADULT_INT_PART<>'NULL'
      then P_ADULT:=to_number(P_ADULT_INT_PART||
                                      '.'||
                                        P_ADULT_DEC_PART);
    end if;

    if P_MALE_INT_PART<>'NULL'
      then P_MALE:=to_number(P_MALE_INT_PART||
                               '.'||
                                 P_MALE_DEC_PART);
    end if;

    if P_FEMALE_INT_PART<>'NULL'
      then P_FEMALE:=to_number(P_FEMALE_INT_PART||
                                 '.'||
                                   P_FEMALE_DEC_PART);
    end if;

    if P_CHILD_INT_PART<>'NULL'
      then P_CHILD:=to_number(P_CHILD_INT_PART||
                                '.'||
                                  P_CHILD_DEC_PART);
    end if;

    if P_INFANT_INT_PART<>'NULL'
      then P_INFANT:=to_number(P_INFANT_INT_PART||
                                '.'||
                                  P_INFANT_DEC_PART);
    end if;

    if P_HAND_BAG_INT_PART<>'NULL'
      then P_HAND_BAG:=to_number(P_HAND_BAG_INT_PART||
                                  '.'||
                                    P_HAND_BAG_DEC_PART);
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
        if P_CLASS_ID<>-1 then
          begin
            select count(id) into R_COUNT
            from WB_REF_AIRCO_CLASS_CODES
            where id=P_CLASS_ID;

            if R_COUNT=0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='This class code is removed!';
                  end;
                else
                  begin
                    str_msg:='Этот класс удален!';
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
         select count(id) into R_COUNT
         from WB_REF_AIRCO_PAX_WEIGHTS
         where ID_AC=P_AIRCO_ID and
               ID_CLASS=P_CLASS_ID and
               nvl(DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy'))=
                 nvl(P_DATE_FROM, to_date('01.01.1900', 'dd.mm.yyyy'));
         if r_count>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The record for this date already existss!';
               end;
             else
               begin
                 str_msg:='Запись для такой даты уже существует!';
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
        P_ID:=SEC_WB_REF_AC_MISC_DET_HIST.nextval();

        insert into WB_REF_AIRCO_PAX_WEIGHTS (ID,
                                                ID_AC,
                                                  DESCRIPTION,
	                                                  DATE_FROM,
	                                                    ADULT,
	                                                      MALE,
	                                                        FEMALE,
	                                                          CHILD,
	                                                            INFANT,
	                                                              HAND_BAG,
	                                                                HAND_BAG_INCLUDE,
	                                                                  U_NAME,
	                                                                    U_IP,
	                                                                      U_HOST_NAME,
	                                                                        DATE_WRITE,
	                                                                          ID_CLASS,
	                                                                            BY_DEFAULT)
        select P_ID,
                 P_AIRCO_ID,
                   P_DESCRIPTION,
	                   P_DATE_FROM,
	                     P_ADULT,
	                       P_MALE,
	                         P_FEMALE,
                             P_CHILD,
                               P_INFANT,
	                               P_HAND_BAG,
	                                 P_HAND_BAG_INCLUDE,
	                                   P_U_NAME,
	                                     P_U_IP,
	                                       P_U_HOST_NAME,
	                                         sysdate(),
                                             P_CLASS_ID,
                                               P_BY_DEFAULT
        from dual;

        if P_BY_DEFAULT=1 then
          begin
            update WB_REF_AIRCO_PAX_WEIGHTS
            set BY_DEFAULT=0
            where ID_AC=P_AIRCO_ID and
                  BY_DEFAULT=1 and
                  ID_CLASS=P_CLASS_ID and
                  ID<>P_ID;
          end;
        end if;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_PAX_WEIGHT_INSERT;
/
