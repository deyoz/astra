create or replace procedure SP_WB_REF_WS_AIR_DOORS_T_INS
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_HOLD_ID number:=-1;
P_D_POS_ID number:=-1;
P_DOOR_NAME varchar2(200):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_HEIGHT number:=null;
P_WIDTH number:=null;
P_BA_FWD number:=null;
P_BA_AFT number:=null;

P_HEIGHT_INT_PART varchar2(50):='NULL';
P_HEIGHT_DEC_PART varchar2(50):='NULL';
P_WIDTH_INT_PART varchar2(50):='NULL';
P_WIDTH_DEC_PART varchar2(50):='NULL';
P_BA_FWD_INT_PART varchar2(50):='NULL';
P_BA_FWD_DEC_PART varchar2(50):='NULL';
P_BA_AFT_INT_PART varchar2(50):='NULL';
P_BA_AFT_DEC_PART varchar2(50):='NULL';

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

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HOLD_ID[1]')) into P_HOLD_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_D_POS_ID[1]')) into P_D_POS_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOOR_NAME[1]') into P_DOOR_NAME from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HEIGHT_INT_PART[1]') into P_HEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HEIGHT_DEC_PART[1]') into P_HEIGHT_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WIDTH_INT_PART[1]') into P_WIDTH_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WIDTH_DEC_PART[1]') into P_WIDTH_DEC_PART from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_INT_PART[1]') into P_BA_FWD_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_FWD_DEC_PART[1]') into P_BA_FWD_DEC_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_INT_PART[1]') into P_BA_AFT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BA_AFT_DEC_PART[1]') into P_BA_AFT_DEC_PART from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    if P_HEIGHT_INT_PART<>'NULL' then P_HEIGHT:=to_number(P_HEIGHT_INT_PART||'.'||P_HEIGHT_DEC_PART); end if;
    if P_WIDTH_INT_PART<>'NULL' then P_WIDTH:=to_number(P_WIDTH_INT_PART||'.'||P_WIDTH_DEC_PART); end if;


    if P_BA_FWD_INT_PART<>'NULL' then P_BA_FWD:=to_number(P_BA_FWD_INT_PART||'.'||P_BA_FWD_DEC_PART); end if;
    if P_BA_AFT_INT_PART<>'NULL' then P_BA_AFT:=to_number(P_BA_AFT_INT_PART||'.'||P_BA_AFT_DEC_PART); end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_HLD_T
    where id=P_HOLD_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='The selected value of "Hold" is removed from the block "Holds"!';
          end;
        else
          begin
            V_STR_MSG:='Выбранное значение "Hold" удалено из блока "Holds"!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_DOORS_POSITIONS
        where ID=P_D_POS_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='The selected value of "Door Position" is removed from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное значение "Door Position" удалено из справочника!';
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
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_DOORS_T
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              DOOR_NAME=P_DOOR_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='The value entered "Door ID/Description" is already in use!';
              end;
            else
              begin
                V_STR_MSG:='Введенное значение "Door ID/Description" уже используется!';
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
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_DOOR_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_DOOR_NAME||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_DOOR_NAME||' является зарезервированной фразой!';
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
        P_ID:=SEC_WB_REF_WS_AIR_DOORS_T.nextval();

        insert into WB_REF_WS_AIR_DOORS_T (ID,
	                                           ID_AC,
	                                             ID_WS,
	                                               ID_BORT,
	                                                 HOLD_ID,
	                                                   D_POS_ID,
	                                                     DOOR_NAME,
	                                                       BA_FWD,
	                                                         BA_AFT,
	                                                           HEIGHT,
	                                                             WIDTH,
                                                        	       U_NAME,
	                                                                 U_IP,
                                                        	           U_HOST_NAME,
	                                                                     DATE_WRITE)
        select P_ID,
	               P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_HOLD_ID,
	                       P_D_POS_ID,
	                         P_DOOR_NAME,
	                           P_BA_FWD,
	                             P_BA_AFT,
	                               P_HEIGHT,
	                                 P_WIDTH,
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
  end SP_WB_REF_WS_AIR_DOORS_T_INS;
/
