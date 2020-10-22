create or replace procedure SP_WB_REF_WS_AIR_HLD_DECK_INS
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
V_ID number:=-1;

P_DECK_ID number:=-1;

P_MAX_WEIGHT number:=-1;
P_MAX_WEIGHT_INT_PART varchar2(50):='';
P_MAX_WEIGHT_DEC_PART varchar2(50):='';

P_MAX_VOLUME number:=null;
P_MAX_VOLUME_INT_PART varchar2(50):='NULL';
P_MAX_VOLUME_DEC_PART varchar2(50):='NULL';

P_LA_FROM number:=-1;
P_LA_FROM_INT_PART varchar2(50):='';
P_LA_FROM_DEC_PART varchar2(50):='';

P_LA_TO number:=-1;
P_LA_TO_INT_PART varchar2(50):='';
P_LA_TO_DEC_PART varchar2(50):='';

P_BA_FWD number:=-1;
P_BA_FWD_INT_PART varchar2(50):='';
P_BA_FWD_DEC_PART varchar2(50):='';

P_BA_AFT number:=-1;
P_BA_AFT_INT_PART varchar2(50):='';
P_BA_AFT_DEC_PART varchar2(50):='';

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DECK_ID[1]')) into P_DECK_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_INT_PART[1]') into P_MAX_VOLUME_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_DEC_PART[1]') into P_MAX_VOLUME_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_FROM_INT_PART[1]') into P_LA_FROM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_FROM_DEC_PART[1]') into P_LA_FROM_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_TO_INT_PART[1]') into P_LA_TO_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_TO_DEC_PART[1]') into P_LA_TO_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_INT_PART[1]') into P_BA_FWD_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_DEC_PART[1]') into P_BA_FWD_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_INT_PART[1]') into P_BA_AFT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_DEC_PART[1]') into P_BA_AFT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART);
    if P_MAX_VOLUME_INT_PART<>'NULL' then  P_MAX_VOLUME:=to_number(P_MAX_VOLUME_INT_PART||'.'||P_MAX_VOLUME_DEC_PART); end if;
    P_LA_FROM:=to_number(P_LA_FROM_INT_PART||'.'||P_LA_FROM_DEC_PART);
    P_LA_TO:=to_number(P_LA_TO_INT_PART||'.'||P_LA_TO_DEC_PART);
    P_BA_FWD:=to_number(P_BA_FWD_INT_PART||'.'||P_BA_FWD_DEC_PART);
    P_BA_AFT:=to_number(P_BA_AFT_INT_PART||'.'||P_BA_AFT_DEC_PART);
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID_WS;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This aircraft is removed!';
          end;
        else
          begin
            V_STR_MSG:='Этот тип ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) and
         (P_ID_AC<>-1) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRCOMPANY_KEY
        where id=P_ID_AC;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='This airline is removed!';
              end;
            else
              begin
                V_STR_MSG:='Эта авиакомпания удалена!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
   ----------------------------------------------------------------------------
   if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_DECK
        where id=P_DECK_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The selected value of "Deck" is removed from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное значение "Deck" удалено из справочника!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_HLD_DECK
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              deck_id=P_DECK_ID;

        if V_R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='Recording with the value of the field "Deck" already exists!';
               end;
             else
               begin
                 V_STR_MSG:='Запись с таким значением поля "Deck" уже существует!';
               end;
             end if;
           end;
          end if;


      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        V_ID:=SEC_WB_REF_WS_AIR_HLD_DECK.nextval();

        insert into WB_REF_WS_AIR_HLD_DECK(ID,
	                                           ID_AC,
	                                             ID_WS,
		                                             ID_BORT,
                                                   DECK_ID,
	                                                   MAX_WEIGHT,
	                                                     LA_FROM,
	                                                       LA_TO,
	                                                         BA_FWD,
	                                                           BA_AFT,
	                                                             U_NAME,
		                                                             U_IP,
		                                                               U_HOST_NAME,
		                                                                 DATE_WRITE,
                                                                       MAX_VOLUME)
        select V_ID,
                 P_ID_AC,
	                 P_ID_WS,
		                 P_ID_BORT,
                       P_DECK_ID,
	                       P_MAX_WEIGHT,
	                         P_LA_FROM,
	                           P_LA_TO,
	                             P_BA_FWD,
	                               P_BA_AFT,
                                   P_U_NAME,
	                                   P_U_IP,
	                                     P_U_HOST_NAME,
                                         sysdate(),
                                           P_MAX_VOLUME
        from dual;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(V_ID)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_HLD_DECK_INS;
/
