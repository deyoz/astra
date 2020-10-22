create or replace procedure SP_WB_REF_WS_AIR_DOW_PW_C_INS
(cXML_in in clob,
   cXML_out out clob)
as

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_PW_CODE_NAME varchar2(200):='';
P_BY_DEFAULT NUMBER:=0;

P_TOTAL_WEIGHT NUMBER:=null;
P_WEIHGT_DIFF NUMBER:=null;
P_ARM_DIFF NUMBER:=null;
P_INDEX_DIFF NUMBER:=null;

P_TOTAL_WEIGHT_INT_PART varchar2(50):='NULL';
P_TOTAL_WEIGHT_DEC_PART varchar2(50):='NULL';
P_WEIHGT_DIFF_INT_PART varchar2(50):='NULL';
P_WEIHGT_DIFF_DEC_PART varchar2(50):='NULL';
P_ARM_DIFF_INT_PART varchar2(50):='NULL';
P_ARM_DIFF_DEC_PART varchar2(50):='NULL';
P_INDEX_DIFF_INT_PART varchar2(50):='NULL';
P_INDEX_DIFF_DEC_PART varchar2(50):='NULL';

P_CODE_TYPE varchar2(200):='';
P_REMARKS varchar2(1000):='';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

V_ID number:=-1;
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PW_CODE_NAME[1]') into P_PW_CODE_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CODE_TYPE[1]') into P_CODE_TYPE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARKS[1]') into P_REMARKS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BY_DEFAULT[1]')) into P_BY_DEFAULT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TOTAL_WEIGHT_INT_PART[1]') into P_TOTAL_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TOTAL_WEIGHT_DEC_PART[1]') into P_TOTAL_WEIGHT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WEIHGT_DIFF_INT_PART[1]') into P_WEIHGT_DIFF_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WEIHGT_DIFF_DEC_PART[1]') into P_WEIHGT_DIFF_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ARM_DIFF_INT_PART[1]') into P_ARM_DIFF_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ARM_DIFF_DEC_PART[1]') into P_ARM_DIFF_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_DIFF_INT_PART[1]') into P_INDEX_DIFF_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_DIFF_DEC_PART[1]') into P_INDEX_DIFF_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_TOTAL_WEIGHT_INT_PART<>'NULL' then P_TOTAL_WEIGHT:=to_number(P_TOTAL_WEIGHT_INT_PART||'.'||P_TOTAL_WEIGHT_DEC_PART); end if;
    if P_WEIHGT_DIFF_INT_PART<>'NULL' then P_WEIHGT_DIFF:=to_number(P_WEIHGT_DIFF_INT_PART||'.'||P_WEIHGT_DIFF_DEC_PART); end if;
    if P_ARM_DIFF_INT_PART<>'NULL' then P_ARM_DIFF:=to_number(P_ARM_DIFF_INT_PART||'.'||P_ARM_DIFF_DEC_PART); end if;
    if P_INDEX_DIFF_INT_PART<>'NULL' then P_INDEX_DIFF:=to_number(P_INDEX_DIFF_INT_PART||'.'||P_INDEX_DIFF_DEC_PART); end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_WS_TYPES
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This type of aircraft is removed from the selected airline!';
          end;
        else
          begin
            V_STR_MSG:='Этот тип ВС удален из выбранной авиакомпании!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_PW_CODE_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_PW_CODE_NAME||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_PW_CODE_NAME||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOW_PW_CODES
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              PW_CODE_NAME=P_PW_CODE_NAME;

      if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Recording with the value of the field "Portable Water Code Name" already exists!';
              end;
            else
              begin
                V_STR_MSG:='Запись с таким значением поля "Portable Water Code Name" уже существует!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_CODE_TYPE<>'EMPTY_STRING' then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_CODE_TYPE;

            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_CODE_TYPE||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение '||P_CODE_TYPE||' является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_REMARKS<>'EMPTY_STRING' then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_REMARKS;

            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_REMARKS||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение '||P_REMARKS||' является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        V_ID:=SEC_WB_REF_WS_AIR_DOW_PW_CODES.nextval();

        insert into WB_REF_WS_AIR_DOW_PW_CODES (ID,
	                                               ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,
	                                                     PW_CODE_NAME,
	                                                       TOTAL_WEIGHT,	
	                                                         WEIHGT_DIFF,
	                                                           ARM_DIFF,
	                                                             INDEX_DIFF,
	                                                               CODE_TYPE,
                                                                   BY_DEFAULT,
	                                                                   REMARKS,
	                                                                     U_NAME,
	                                                                       U_IP,
	                                                                         U_HOST_NAME,
	                                                                           DATE_WRITE)
        select V_ID,
	               P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_PW_CODE_NAME,
	                       P_TOTAL_WEIGHT,
	                         P_WEIHGT_DIFF,
	                           P_ARM_DIFF,
	                             P_INDEX_DIFF,
	                               P_CODE_TYPE,
                                   P_BY_DEFAULT,
	                                   P_REMARKS,
                                       P_U_NAME,
	                                       P_U_IP,
                                           P_U_HOST_NAME,
	                                           sysdate()
	
        from dual;

        if P_BY_DEFAULT=1 then
          begin
            update WB_REF_WS_AIR_DOW_PW_CODES
            set BY_DEFAULT=0
            where id_ac=P_ID_AC and
                  id_ws=P_ID_WS and
                  id_bort=P_ID_BORT and
                  BY_DEFAULT=1 and
                  id<>V_ID;
          end;
        end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_number(V_ID)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_DOW_PW_C_INS;
/
