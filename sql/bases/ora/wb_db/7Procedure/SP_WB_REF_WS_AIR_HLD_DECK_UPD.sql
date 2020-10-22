create or replace procedure SP_WB_REF_WS_AIR_HLD_DECK_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

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
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
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
    from WB_REF_WS_AIR_HLD_DECK
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='The selected record is removed!';
          end;
        else
          begin
            V_STR_MSG:='Выбранная запись удалена!';
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
        where id<>P_ID and
              id_ac=P_ID_AC and
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
        update WB_REF_WS_AIR_HLD_DECK
        set DECK_ID=P_DECK_ID,
	          MAX_WEIGHT=P_MAX_WEIGHT,
	          LA_FROM=P_LA_FROM,
	          LA_TO=P_LA_TO,
	          BA_FWD=P_BA_FWD,
	          BA_AFT=P_BA_AFT,
	          U_NAME=P_U_NAME,
		        U_IP=P_U_IP,
		        U_HOST_NAME=P_U_HOST_NAME,
		        DATE_WRITE=sysdate(),
            MAX_VOLUME=P_MAX_VOLUME
        where id=P_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_HLD_DECK_UPD;
/
