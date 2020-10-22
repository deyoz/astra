create or replace procedure SP_WB_REF_WS_AIR_HLD_T_INS
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_DECK_ID number:=-1;
P_HOLD_NAME varchar2(200):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_MAX_WEIGHT number:=null;
P_MAX_VOLUME number:=null;
P_LA_CENTROID number:=null;
P_LA_FROM number:=null;
P_LA_TO number:=null;
P_BA_CENTROID number:=null;
P_BA_FWD number:=null;
P_BA_AFT number:=null;
P_INDEX_PER_WT_UNIT number:=null;

P_MAX_WEIGHT_INT_PART varchar2(50):='NULL';
P_MAX_WEIGHT_DEC_PART varchar2(50):='NULL';
P_MAX_VOLUME_INT_PART varchar2(50):='NULL';
P_MAX_VOLUME_DEC_PART varchar2(50):='NULL';
P_LA_CENTROID_INT_PART varchar2(50):='NULL';
P_LA_CENTROID_DEC_PART varchar2(50):='NULL';
P_LA_FROM_INT_PART varchar2(50):='NULL';
P_LA_FROM_DEC_PART varchar2(50):='NULL';
P_LA_TO_INT_PART varchar2(50):='NULL';
P_LA_TO_DEC_PART varchar2(50):='NULL';
P_BA_CENTROID_INT_PART varchar2(50):='NULL';
P_BA_CENTROID_DEC_PART varchar2(50):='NULL';
P_BA_FWD_INT_PART varchar2(50):='NULL';
P_BA_FWD_DEC_PART varchar2(50):='NULL';
P_BA_AFT_INT_PART varchar2(50):='NULL';
P_BA_AFT_DEC_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_INT_PART varchar2(50):='NULL';
P_INDEX_PER_WT_UNIT_DEC_PART varchar2(50):='NULL';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DECK_ID[1]')) into P_DECK_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HOLD_NAME[1]') into P_HOLD_NAME from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_INT_PART[1]') into P_MAX_VOLUME_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_DEC_PART[1]') into P_MAX_VOLUME_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_CENTROID_INT_PART[1]') into P_LA_CENTROID_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_CENTROID_DEC_PART[1]') into P_LA_CENTROID_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_FROM_INT_PART[1]') into P_LA_FROM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_FROM_DEC_PART[1]') into P_LA_FROM_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_TO_INT_PART[1]') into P_LA_TO_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LA_TO_DEC_PART[1]') into P_LA_TO_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_CENTROID_INT_PART[1]') into P_BA_CENTROID_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_CENTROID_DEC_PART[1]') into P_BA_CENTROID_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_INT_PART[1]') into P_BA_FWD_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_DEC_PART[1]') into P_BA_FWD_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_INT_PART[1]') into P_BA_AFT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_DEC_PART[1]') into P_BA_AFT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_PER_WT_UNIT_INT_PART[1]') into P_INDEX_PER_WT_UNIT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_INDEX_PER_WT_UNIT_DEC_PART[1]') into P_INDEX_PER_WT_UNIT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_MAX_WEIGHT_INT_PART<>'NULL' then P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART); end if;
    if P_MAX_VOLUME_INT_PART<>'NULL' then P_MAX_VOLUME:=to_number(P_MAX_VOLUME_INT_PART||'.'||P_MAX_VOLUME_DEC_PART); end if;

    if P_LA_CENTROID_INT_PART<>'NULL' then P_LA_CENTROID:=to_number(P_LA_CENTROID_INT_PART||'.'||P_LA_CENTROID_DEC_PART); end if;
    if P_LA_FROM_INT_PART<>'NULL' then P_LA_FROM:=to_number(P_LA_FROM_INT_PART||'.'||P_LA_FROM_DEC_PART); end if;
    if P_LA_TO_INT_PART<>'NULL' then P_LA_TO:=to_number(P_LA_TO_INT_PART||'.'||P_LA_TO_DEC_PART); end if;

    if P_BA_CENTROID_INT_PART<>'NULL' then P_BA_CENTROID:=to_number(P_BA_CENTROID_INT_PART||'.'||P_BA_CENTROID_DEC_PART); end if;
    if P_BA_FWD_INT_PART<>'NULL' then P_BA_FWD:=to_number(P_BA_FWD_INT_PART||'.'||P_BA_FWD_DEC_PART); end if;
    if P_BA_AFT_INT_PART<>'NULL' then P_BA_AFT:=to_number(P_BA_AFT_INT_PART||'.'||P_BA_AFT_DEC_PART); end if;

    if P_INDEX_PER_WT_UNIT_INT_PART<>'NULL' then
      begin
        P_INDEX_PER_WT_UNIT:=to_number(P_INDEX_PER_WT_UNIT_INT_PART||'.'||P_INDEX_PER_WT_UNIT_DEC_PART);
      end;
    end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_DECK
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT and
          deck_id=P_DECK_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='The selected value of "Deck" is removed from the block "Decks"!';
          end;
        else
          begin
            V_STR_MSG:='Выбранное значение "Deck" удалено из блока "Decks"!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_HOLD_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_HOLD_NAME||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_HOLD_NAME||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_HLD_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT and
          HOLD_NAME=P_HOLD_NAME;

    if V_R_COUNT>0 then
      begin
        if V_STR_MSG is null then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The value entered "Hold Name" is already in use!';
              end;
            else
              begin
                V_STR_MSG:='Введенное значение "Hold Name" уже используется!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        P_ID:=SEC_WB_REF_WS_AIR_HLD_HLD_T.nextval();

        insert into WB_REF_WS_AIR_HLD_HLD_T (ID,
	                                             ID_AC,
	                                               ID_WS,
	                                                 ID_BORT,
	                                                   DECK_ID,
	                                                     HOLD_NAME,
	                                                       MAX_WEIGHT,
	                                                         MAX_VOLUME,
                                                         	   LA_CENTROID,
                                                          	   LA_FROM,
                                                         	       LA_TO,
                                                        	         BA_CENTROID,
                                                        	           BA_FWD,
                                                        	             BA_AFT,
                                                        	               INDEX_PER_WT_UNIT,
                                                        	                 U_NAME,
	                                                                           U_IP,
                                                        	                     U_HOST_NAME,
	                                                                               DATE_WRITE)
        select P_ID,
	               P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_DECK_ID,
	                       P_HOLD_NAME,
	                         P_MAX_WEIGHT,
	                           P_MAX_VOLUME,
                               P_LA_CENTROID,
                                 P_LA_FROM,
                                   P_LA_TO,
                                     P_BA_CENTROID,
                                       P_BA_FWD,
                                         P_BA_AFT,
                                           P_INDEX_PER_WT_UNIT,
                                             P_U_NAME,
	                                             P_U_IP,
                                                 P_U_HOST_NAME,
	                                                 sysdate()
        from dual;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;


    cXML_out:=cXML_out||'<list id="'||to_number(P_ID)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_HLD_T_INS;
/
