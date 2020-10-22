create or replace procedure SP_WB_REF_AC_CR_WEIGHTS_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
P_DATE_FROM date:=null;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);
P_FC_STANDART NUMBER:=null;
P_FC_STANDART_INT_PART varchar2(50);
P_FC_STANDART_DEC_PART varchar2(50);
P_FC_MALE NUMBER:=null;
P_FC_MALE_INT_PART varchar2(50);
P_FC_MALE_DEC_PART varchar2(50);
P_FC_FEMALE NUMBER:=null;
P_FC_FEMALE_INT_PART varchar2(50);
P_FC_FEMALE_DEC_PART varchar2(50);
P_FC_HAND_BAG NUMBER:=null;
P_FC_HAND_BAG_INT_PART varchar2(50);
P_FC_HAND_BAG_DEC_PART varchar2(50);
P_FC_HAND_BAG_INCLUDE NUMBER:=null;
P_CC_STANDART NUMBER:=null;
P_CC_STANDART_INT_PART varchar2(50);
P_CC_STANDART_DEC_PART varchar2(50);
P_CC_MALE NUMBER:=null;
P_CC_MALE_INT_PART varchar2(50);
P_CC_MALE_DEC_PART varchar2(50);
P_CC_FEMALE NUMBER:=null;
P_CC_FEMALE_INT_PART varchar2(50);
P_CC_FEMALE_DEC_PART varchar2(50);
P_CC_HAND_BAG NUMBER:=null;
P_CC_HAND_BAG_INT_PART varchar2(50);
P_CC_HAND_BAG_DEC_PART varchar2(50);
P_CC_HAND_BAG_INCLUDE NUMBER:=0;
P_FC_BAGGAGE_WEIGHT NUMBER:=null;
P_FC_BAGGAGE_WEIGHT_INT_PART varchar2(50);
P_FC_BAGGAGE_WEIGHT_DEC_PART varchar2(50);
P_CC_BAGGAGE_WEIGHT NUMBER:=null;
P_CC_BAGGAGE_WEIGHT_INT_PART varchar2(50);
P_CC_BAGGAGE_WEIGHT_DEC_PART varchar2(50);
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
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_STANDART_INT_PART[1]') into P_FC_STANDART_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_STANDART_DEC_PART[1]') into P_FC_STANDART_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_MALE_INT_PART[1]') into P_FC_MALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_MALE_DEC_PART[1]') into P_FC_MALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_FEMALE_INT_PART[1]') into P_FC_FEMALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_FEMALE_DEC_PART[1]') into P_FC_FEMALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_HAND_BAG_INT_PART[1]') into P_FC_HAND_BAG_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_HAND_BAG_DEC_PART[1]') into P_FC_HAND_BAG_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_STANDART_INT_PART[1]') into P_CC_STANDART_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_STANDART_DEC_PART[1]') into P_CC_STANDART_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_MALE_INT_PART[1]') into P_CC_MALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_MALE_DEC_PART[1]') into P_CC_MALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_FEMALE_INT_PART[1]') into P_CC_FEMALE_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_FEMALE_DEC_PART[1]') into P_CC_FEMALE_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_HAND_BAG_INT_PART[1]') into P_CC_HAND_BAG_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_HAND_BAG_DEC_PART[1]') into P_CC_HAND_BAG_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_BAGGAGE_WEIGHT_INT_PART[1]') into P_FC_BAGGAGE_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_BAGGAGE_WEIGHT_DEC_PART[1]') into P_FC_BAGGAGE_WEIGHT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_BAGGAGE_WEIGHT_INT_PART[1]') into P_CC_BAGGAGE_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_BAGGAGE_WEIGHT_DEC_PART[1]') into P_CC_BAGGAGE_WEIGHT_DEC_PART from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_FC_HAND_BAG_INCLUDE[1]')) into P_FC_HAND_BAG_INCLUDE from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CC_HAND_BAG_INCLUDE[1]')) into P_CC_HAND_BAG_INCLUDE from dual;
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

    if P_FC_STANDART_INT_PART<>'NULL'
      then P_FC_STANDART:=to_number(P_FC_STANDART_INT_PART||
                                      '.'||
                                        P_FC_STANDART_DEC_PART);
    end if;

    if P_FC_MALE_INT_PART<>'NULL'
      then P_FC_MALE:=to_number(P_FC_MALE_INT_PART||
                                  '.'||
                                    P_FC_MALE_DEC_PART);
    end if;

    if P_FC_FEMALE_INT_PART<>'NULL'
      then P_FC_FEMALE:=to_number(P_FC_FEMALE_INT_PART||
                                    '.'||
                                      P_FC_FEMALE_DEC_PART);
    end if;

    if P_FC_HAND_BAG_INT_PART<>'NULL'
      then P_FC_HAND_BAG:=to_number(P_FC_HAND_BAG_INT_PART||
                                      '.'||
                                        P_FC_HAND_BAG_DEC_PART);
    end if;


    if P_CC_STANDART_INT_PART<>'NULL'
      then P_CC_STANDART:=to_number(P_CC_STANDART_INT_PART||
                                      '.'||
                                        P_CC_STANDART_DEC_PART);
    end if;

    if P_CC_MALE_INT_PART<>'NULL'
      then P_CC_MALE:=to_number(P_CC_MALE_INT_PART||
                                  '.'||
                                    P_CC_MALE_DEC_PART);
    end if;

    if P_CC_FEMALE_INT_PART<>'NULL'
      then P_CC_FEMALE:=to_number(P_CC_FEMALE_INT_PART||
                                    '.'||
                                      P_CC_FEMALE_DEC_PART);
    end if;

    if P_CC_HAND_BAG_INT_PART<>'NULL'
      then P_CC_HAND_BAG:=to_number(P_CC_HAND_BAG_INT_PART||
                                      '.'||
                                        P_CC_HAND_BAG_DEC_PART);
    end if;

    if P_FC_BAGGAGE_WEIGHT_INT_PART<>'NULL'
      then P_FC_BAGGAGE_WEIGHT:=to_number(P_FC_BAGGAGE_WEIGHT_INT_PART||
                                           '.'||
                                             P_FC_BAGGAGE_WEIGHT_DEC_PART);
    end if;

   if P_CC_BAGGAGE_WEIGHT_INT_PART<>'NULL'
      then P_CC_BAGGAGE_WEIGHT:=to_number(P_CC_BAGGAGE_WEIGHT_INT_PART||
                                           '.'||
                                             P_CC_BAGGAGE_WEIGHT_DEC_PART);
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
         from WB_REF_AIRCO_CREW_WEIGHTS
         where ID_AC=P_AIRCO_ID and
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
        P_ID:=SEC_WB_REF_AIRCO_CREW_WEIGHTS.nextval();

        insert into WB_REF_AIRCO_CREW_WEIGHTS (ID,
                                                ID_AC,
                                                  DESCRIPTION,
	                                                  DATE_FROM,
	                                                    FC_STANDART,
	                                                      FC_MALE,
	                                                        FC_FEMALE,
	                                                          FC_HAND_BAG,
	                                                            FC_HAND_BAG_INCLUDE,
	                                                              CC_STANDART,
	                                                                CC_MALE,
	                                                                  CC_FEMALE,
	                                                                    CC_HAND_BAG,
	                                                                      CC_HAND_BAG_INCLUDE,
	                                                                        FC_BAGGAGE_WEIGHT,
	                                                                          CC_BAGGAGE_WEIGHT,
	                                                                            BY_DEFAULT,
	                                                                              U_NAME,
	                                                                                U_IP,
	                                                                                  U_HOST_NAME,
	                                                                                    DATE_WRITE)
        select P_ID,
                 P_AIRCO_ID,
                   P_DESCRIPTION,
	                   P_DATE_FROM,
	                     P_FC_STANDART,
	                       P_FC_MALE,
	                         P_FC_FEMALE,
	                           P_FC_HAND_BAG,
	                             P_FC_HAND_BAG_INCLUDE,
	                               P_CC_STANDART,
	                                 P_CC_MALE,
	                                   P_CC_FEMALE,
	                                     P_CC_HAND_BAG,
	                                       P_CC_HAND_BAG_INCLUDE,
	                                         P_FC_BAGGAGE_WEIGHT,
	                                           P_CC_BAGGAGE_WEIGHT,
	                                             P_BY_DEFAULT,
	                                               P_U_NAME,
	                                                 P_U_IP,
	                                                   P_U_HOST_NAME,
	                                                     sysdate()
        from dual;

        if P_BY_DEFAULT=1 then
          begin
            update WB_REF_AIRCO_CREW_WEIGHTS
            set BY_DEFAULT=0
            where ID_AC=P_AIRCO_ID and
                  BY_DEFAULT=1 and
                  ID<>P_ID;
          end;
        end if;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_CR_WEIGHTS_INSERT;
/
