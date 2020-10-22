create or replace procedure SP_WB_REF_WS_AIR_CMP_T_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_HOLD_ID number:=-1;
P_CMP_NAME varchar2(200):='';
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
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HOLD_ID[1]')) into P_HOLD_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CMP_NAME[1]') into P_CMP_NAME from dual;

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
    from WB_REF_WS_AIR_HLD_CMP_T
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
    if V_STR_MSG is null then
      begin
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
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_CMP_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 V_STR_MSG:='Value '||P_CMP_NAME||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Значение '||P_CMP_NAME||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    /*if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_HLD_CMP_T
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              HOLD_ID=P_HOLD_ID and
              id<>P_ID;

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
      end;
    end if;  */
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        insert into WB_TEMP_XML_ID_EX (id,
                                         ACTION_NAME,
                                           F_FLT_1,
                                             F_FLT_2,
                                               F_FLT_3,
                                                 F_STR_1)
        select distinct id,
                          'SP_WB_REF_WS_AIR_CMP_T_UPD',
                            id_ac,
                              id_ws,
                                id_bort,
                                  case when id=P_ID then P_CMP_NAME else CMP_NAME end
        from WB_REF_WS_AIR_HLD_CMP_T
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        select count(sbt.id) into V_R_COUNT
        from WB_REF_WS_AIR_SEC_BAY_T sbt
        where sbt.id_ac=P_ID_AC and
              sbt.id_ws=P_ID_WS and
              sbt.id_bort=P_ID_BORT and
              not exists(select tt.id
                         from WB_TEMP_XML_ID_EX tt
                         where tt.F_FLT_1=sbt.id_ac and
                               tt.F_FLT_2=sbt.id_ws and
                               tt.F_FLT_3=sbt.id_bort and
                               tt.F_STR_1=sbt.CMP_NAME);

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='On the field "Comp Name" referenced in the section "Sections/Bays"!';
              end;
            else
              begin
                V_STR_MSG:='На поле "Comp Name" имеются ссылки в блоке "Sections/Bays"!';
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
        update WB_REF_WS_AIR_HLD_CMP_T
        set HOLD_ID=P_HOLD_ID,
	          CMP_NAME=P_CMP_NAME,
	          MAX_WEIGHT=P_MAX_WEIGHT,
	          MAX_VOLUME=P_MAX_VOLUME,
            LA_CENTROID=P_LA_CENTROID,
            LA_FROM=P_LA_FROM,
            LA_TO=P_LA_TO,
            BA_CENTROID=P_BA_CENTROID,
            BA_FWD=P_BA_FWD,
            BA_AFT=P_BA_AFT,
            INDEX_PER_WT_UNIT=P_INDEX_PER_WT_UNIT,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
            U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;


    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_CMP_T_UPD;
/
