create or replace procedure SP_WB_REF_WS_AIR_MIN_WGT_T_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_ID_BORT_T number:=-1;
P_IS_DEFAULT number:=-1;
P_IDN number:=-1;

P_MIN_RMP_WEIGHT number:=null;
P_MIN_ZF_WEIGHT number:=null;
P_MIN_TO_WEIGHT number:=null;
P_MIN_LND_WEIGHT number:=null;

P_MIN_RMP_WEIGHT_INT_PART varchar2(50):='NULL';
P_MIN_RMP_WEIGHT_DEC_PART varchar2(50):='NULL';

P_MIN_ZF_WEIGHT_INT_PART varchar2(50):='NULL';
P_MIN_ZF_WEIGHT_DEC_PART varchar2(50):='NULL';

P_MIN_TO_WEIGHT_INT_PART varchar2(50):='NULL';
P_MIN_TO_WEIGHT_DEC_PART varchar2(50):='NULL';

P_MIN_LND_WEIGHT_INT_PART varchar2(50):='NULL';
P_MIN_LND_WEIGHT_DEC_PART varchar2(50):='NULL';

P_REMARK varchar2(2000):='';
P_IS_REMARK_EMPTY number:=0;

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

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT_T[1]')) into P_ID_BORT_T from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IDN[1]')) into P_IDN from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_DEFAULT[1]')) into P_IS_DEFAULT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_RMP_WEIGHT_INT_PART[1]') into P_MIN_RMP_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_RMP_WEIGHT_DEC_PART[1]') into P_MIN_RMP_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_ZF_WEIGHT_INT_PART[1]') into P_MIN_ZF_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_ZF_WEIGHT_DEC_PART[1]') into P_MIN_ZF_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_TO_WEIGHT_INT_PART[1]') into P_MIN_TO_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_TO_WEIGHT_DEC_PART[1]') into P_MIN_TO_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_LND_WEIGHT_INT_PART[1]') into P_MIN_LND_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MIN_LND_WEIGHT_DEC_PART[1]') into P_MIN_LND_WEIGHT_DEC_PART from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK[1]') into P_REMARK from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

   P_MIN_RMP_WEIGHT:=to_number(P_MIN_RMP_WEIGHT_INT_PART||'.'||P_MIN_RMP_WEIGHT_DEC_PART);
   P_MIN_ZF_WEIGHT:=to_number(P_MIN_ZF_WEIGHT_INT_PART||'.'||P_MIN_ZF_WEIGHT_DEC_PART);
   P_MIN_TO_WEIGHT:=to_number(P_MIN_TO_WEIGHT_INT_PART||'.'||P_MIN_TO_WEIGHT_DEC_PART);
   P_MIN_LND_WEIGHT:=to_number(P_MIN_LND_WEIGHT_INT_PART||'.'||P_MIN_LND_WEIGHT_DEC_PART);

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_MIN_WGHT_T
    where ID=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='The record has been deleted!';
          end;
        else
          begin
            V_STR_MSG:='Выбранная запись  удалена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRCO_WS_BORTS
        where ID=P_ID_BORT_T;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Bort" has been deleted!';
              end;
            else
              begin
                V_STR_MSG:='Запись для выбранного бортового номера удалена!';
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
        from WB_REF_WS_AIR_MIN_WGHT_T
        where ID_BORT=P_ID_BORT_T and
              IDN=P_IDN and
              ID<>P_ID;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Bort" already exists!';
              end;
            else
              begin
                V_STR_MSG:='Запись для выбранного бортового номера уже существует!';
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
        if P_IS_REMARK_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if P_IS_REMARK_EMPTY=0 then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_REMARK;


            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_REMARK||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение '||P_REMARK||' является зарезервированной фразой!';
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
    if (V_STR_MSG is null) then
      begin
        update WB_REF_WS_AIR_MIN_WGHT_T
        set ID_BORT=P_ID_BORT_T,
            MIN_RMP_WEIGHT=P_MIN_RMP_WEIGHT,
	          MIN_ZF_WEIGHT=P_MIN_ZF_WEIGHT,
	          MIN_TO_WEIGHT=P_MIN_TO_WEIGHT,
	          MIN_LND_WEIGHT=P_MIN_LND_WEIGHT,
	          IS_DEFAULT=P_IS_DEFAULT,
	          REMARK=P_REMARK,
	          U_NAME=P_U_NAME,
		        U_IP=P_U_IP,
		        U_HOST_NAME=P_U_HOST_NAME,
		        DATE_WRITE=sysdate()
        where ID=P_ID;

        if P_IS_DEFAULT=1 then
          begin
            UPDATE WB_REF_WS_AIR_MIN_WGHT_T
            set IS_DEFAULT=0,
                U_NAME=P_U_NAME,
		            U_IP=P_U_IP,
		            U_HOST_NAME=P_U_HOST_NAME,
		            DATE_WRITE=sysdate()
            where ID_BORT=P_ID_BORT_T and
                  ID<>P_ID AND
                  IS_DEFAULT=1;
          end;
        end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_MIN_WGT_T_UPD;
/
