create or replace procedure SP_WB_REF_WS_AIR_SEC_BAY_T_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_SEC_BAY_TYPE_ID number:=-1;
P_DOOR_POSITION number:=0;
P_CMP_NAME varchar2(200):='';
P_SEC_BAY_NAME varchar2(200):='';
P_COLOR number:=0;

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
V_SEC_BAY_NAME varchar2(200):='';
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SEC_BAY_TYPE_ID[1]')) into P_SEC_BAY_TYPE_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOOR_POSITION[1]')) into P_DOOR_POSITION from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_COLOR[1]')) into P_COLOR from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CMP_NAME[1]') into P_CMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SEC_BAY_NAME[1]') into P_SEC_BAY_NAME from dual;


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

    if P_INDEX_PER_WT_UNIT_INT_PART<>'NULL' then P_INDEX_PER_WT_UNIT:=to_number(P_INDEX_PER_WT_UNIT_INT_PART||'.'||P_INDEX_PER_WT_UNIT_DEC_PART); end if;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_T
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record� is removed!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='��࠭��� ������ 㤠����!'; end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_HLD_CMP_T
        where ID_AC=P_ID_AC and
              ID_WS=P_ID_WS and
              ID_BORT=P_ID_BORT and
              CMP_NAME=P_CMP_NAME;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Selected value "Comp" is removed from the block "Compartments"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='��࠭��� ���祭�� "Comp" 㤠���� �� ����� "Compartments"!'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_SEC_BAY_TYPE
        where id=P_SEC_BAY_TYPE_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Selected value "Sec/Bay Type" is removed from the directory!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='��࠭��� ���祭�� "Sec/Bay Type" 㤠���� �� ����� "Compartments"!'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_SEC_BAY_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Meaning "Sec/Bay Name" is a phrase reserved!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='���祭�� "Sec/Bay Name" ���� ��१�ࢨ஢����� �ࠧ��!'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_SEC_BAY_T
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              SEC_BAY_NAME=P_SEC_BAY_NAME and
              id<>P_ID;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='This value of "Sec/Bay Name" already exists!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='����� ���祭�� "Sec/Bay Name" 㦥 �������!'; end if;
          end;
        end if;
      end;
    end if;

    if (V_STR_MSG is null) then
      begin
        select IS_ULD_IATA into V_R_COUNT
        from WB_REF_SEC_BAY_TYPE
        where id=P_SEC_BAY_TYPE_ID;

        if V_R_COUNT=0 then
          begin
            insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
	                                                     U_NAME_,
	                                                       U_IP_,
	                                                         U_HOST_NAME_,
	                                                           DATE_WRITE_,
                                                               OPERATION_,
	                                                               ACTION_,
	                                                                 ID,
	                                                                   ID_AC,
	                                                                     ID_WS,
		                                                                     ID_BORT,
                                                                           T_ID,
	                                                                           ULD_IATA_ID,
	                                                                             U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
            select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
                     P_U_NAME,
	                     P_U_IP,
	                       P_U_HOST_NAME,
                           SYSDATE(),
                             'delete',
                               'delete',
                                 i.id,
                                   i.ID_AC,
	                                   i.ID_WS,
		                                   i.ID_BORT,
                                         i.T_ID,
	                                         i.ULD_IATA_ID,
                                             i.U_NAME,
		                                         	 i.U_IP,
		                                         	   i.U_HOST_NAME,
		                                     	         i.DATE_WRITE
            from WB_REF_WS_AIR_SEC_BAY_TT i
            where i.T_ID=P_ID;

            delete from WB_REF_WS_AIR_SEC_BAY_TT where T_ID=P_ID;
          end;
        end if;

        select SEC_BAY_NAME into V_SEC_BAY_NAME
        from WB_REF_WS_AIR_SEC_BAY_T
        where id=p_id;

        update WB_REF_WS_AIR_ULD_OVER
        set OVERLAY=P_SEC_BAY_NAME
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              OVERLAY=V_SEC_BAY_NAME;

        update WB_REF_WS_AIR_ULD_OVER
        set U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
            U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        update WB_REF_WS_AIR_SEC_BAY_T
        set CMP_NAME=P_CMP_NAME,
	          SEC_BAY_NAME=P_SEC_BAY_NAME,
	          SEC_BAY_TYPE_ID=P_SEC_BAY_TYPE_ID,
	          MAX_WEIGHT=P_MAX_WEIGHT,
	          MAX_VOLUME=P_MAX_VOLUME,
            LA_CENTROID=P_LA_CENTROID,
            LA_FROM=P_LA_FROM,
            LA_TO=P_LA_TO,
            BA_CENTROID=P_BA_CENTROID,
            BA_FWD=P_BA_FWD,
            BA_AFT=P_BA_AFT,
            INDEX_PER_WT_UNIT=P_INDEX_PER_WT_UNIT,
            DOOR_POSITION=P_DOOR_POSITION,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
            U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate(),
            COLOR=P_COLOR
        where id=p_id;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_SEC_BAY_T_UPD;
/
