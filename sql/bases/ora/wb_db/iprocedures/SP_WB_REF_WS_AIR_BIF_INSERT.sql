create or replace procedure SP_WB_REF_WS_AIR_BIF_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_REF_ARM number:=0;
P_REF_ARM_INT_PART varchar2(50);
P_REF_ARM_DEC_PART varchar2(50);

P_K_CONST number:=0;
P_K_CONST_INT_PART varchar2(50);
P_K_CONST_DEC_PART varchar2(50);

P_C_CONST number:=0;
P_C_CONST_INT_PART varchar2(50);
P_C_CONST_DEC_PART varchar2(50);

P_LEN_MAC_RC number:=-1;
P_LEN_MAC_RC_INT_PART varchar2(50);
P_LEN_MAC_RC_DEC_PART varchar2(50);

P_LEMAC_LERC number:=-1;
P_LEMAC_LERC_INT_PART varchar2(50);
P_LEMAC_LERC_DEC_PART varchar2(50);

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/REF_ARM_INT_PART[1]') into P_REF_ARM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/REF_ARM_DEC_PART[1]') into P_REF_ARM_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/K_CONST_INT_PART[1]') into P_K_CONST_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/K_CONST_DEC_PART[1]') into P_K_CONST_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/C_CONST_INT_PART[1]') into P_C_CONST_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/C_CONST_DEC_PART[1]') into P_C_CONST_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/LEN_MAC_RC_INT_PART[1]') into P_LEN_MAC_RC_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/LEN_MAC_RC_DEC_PART[1]') into P_LEN_MAC_RC_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/LEMAC_LERC_INT_PART[1]') into P_LEMAC_LERC_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/LEMAC_LERC_DEC_PART[1]') into P_LEMAC_LERC_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    if P_REF_ARM_INT_PART<>'NULL'
      then P_REF_ARM:=to_number(P_REF_ARM_INT_PART||
                                  '.'||
                                    P_REF_ARM_DEC_PART);
    end if;

    if P_K_CONST_INT_PART<>'NULL'
      then P_K_CONST:=to_number(P_K_CONST_INT_PART||
                                  '.'||
                                    P_K_CONST_DEC_PART);
    end if;

    if P_C_CONST_INT_PART<>'NULL'
      then P_C_CONST:=to_number(P_C_CONST_INT_PART||
                                  '.'||
                                    P_C_CONST_DEC_PART);
    end if;

    if P_LEN_MAC_RC_INT_PART<>'NULL'
      then P_LEN_MAC_RC:=to_number(P_LEN_MAC_RC_INT_PART||
                                     '.'||
                                       P_LEN_MAC_RC_DEC_PART);
    end if;

    if P_LEMAC_LERC_INT_PART<>'NULL'
      then P_LEMAC_LERC:=to_number(P_LEMAC_LERC_INT_PART||
                                     '.'||
                                       P_LEMAC_LERC_DEC_PART);
    end if;

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID_WS;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='This aircraft is removed!';
          end;
        else
          begin
            str_msg:='Этт тип ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (str_msg is null) and
         (P_ID_AC<>-1) then
      begin
        select count(id) into R_COUNT
        from WB_REF_AIRCOMPANY_KEY
        where id=P_ID_AC;

        if R_COUNT=0 then
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
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into R_COUNT
         from WB_REF_WS_AIR_BAS_IND_FORM
         where ID_AC=P_ID_AC and
               ID_WS=P_ID_WS and
               ID_BORT=P_ID_BORT and
               DATE_FROM=P_DATE_FROM;

         if R_COUNT>0 then
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
    if (str_msg is null) then
      begin
        P_ID:=SEC_WB_REF_WS_AIR_BAS_IND_FORM.nextval();

        insert into WB_REF_WS_AIR_BAS_IND_FORM (ID,
                                                 ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,
	                                                     DATE_FROM,
	                                                       REF_ARM,
	                                                         K_CONST,
	                                                           C_CONST,
	                                                             LEN_MAC_RC,
	                                                               LEMAC_LERC,
	                                                                 U_NAME,
	                                                                   U_IP,
	                                                                     U_HOST_NAME,
	                                                                       DATE_WRITE)
        select P_ID,
                 P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_DATE_FROM,
	                       P_REF_ARM,
                           P_K_CONST,
                             P_C_CONST,
                               P_LEN_MAC_RC,
                                 P_LEMAC_LERC,
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
  end SP_WB_REF_WS_AIR_BIF_INSERT;
/
