create or replace procedure SP_WB_REF_WS_AIR_SEC_B_A_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_REMARKS clob:='';
P_IS_REMARKS_EMPTY number:=0;

P_CH_BALANCE_ARM number:=0;
P_CH_INDEX_UNIT number:=0;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
V_ID number:=-1;
begin
    cXML_out:='';

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARKS[1]') into P_REMARKS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARKS_EMPTY[1]')) into P_IS_REMARKS_EMPTY from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CH_BALANCE_ARM[1]')) into P_CH_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CH_INDEX_UNIT[1]')) into P_CH_INDEX_UNIT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
    select id into V_ID
    from WB_REF_WS_AIR_SEC_BAY_A
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

   if V_ID=-1 then
     begin
       if P_LANG='ENG' then
          begin
            V_str_msg:='This record is removed!';
          end;
        else
          begin
            V_str_msg:='Эта запись удалена!';
          end;
        end if;
     end;
   end if;
   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        if p_IS_REMARKS_EMPTY=1 then
          begin

            P_REMARKS:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REMARKS_EMPTY=0 then
          begin
            if P_REMARKS='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    V_STR_MSG:='Value field "Remarks" is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение поля "Примечание" является зарезервированной фразой!';
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
    if V_STR_MSG is null then
      begin
        update WB_REF_WS_AIR_SEC_BAY_A
        set REMARKS=P_REMARKS,
            CH_BALANCE_ARM=P_CH_BALANCE_ARM,
            CH_INDEX_UNIT=P_CH_INDEX_UNIT,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=V_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_SEC_B_A_UPD;
/
