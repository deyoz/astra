create or replace procedure SP_WB_REF_WS_AIR_S_L_C_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_IDN number:=-1;
P_LANG varchar2(50):='';
P_TABLE_NAME varchar2(1000):='';

P_CH_USE_AS_DEFAULT number:=-1;
P_CH_CAI_BALANCE_ARM number:=-1;
P_CH_CAI_INDEX_UNIT number:=-1;
P_CH_CI_BALANCE_ARM number:=-1;
P_CH_CI_INDEX_UNIT number:=-1;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
begin
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IDN[1]')) into P_IDN from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_USE_AS_DEFAULT[1]')) into P_CH_USE_AS_DEFAULT from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CAI_BALANCE_ARM[1]')) into P_CH_CAI_BALANCE_ARM from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CAI_INDEX_UNIT[1]')) into P_CH_CAI_INDEX_UNIT from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CI_BALANCE_ARM[1]')) into P_CH_CI_BALANCE_ARM from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_CI_INDEX_UNIT[1]')) into P_CH_CI_INDEX_UNIT from dual;

  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


  cXML_out:='<?xml version="1.0" ?><root>';
  ------------------------------------------------------------------------------
  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_C_ADV
  where id=P_ID;

  if V_R_COUNT=0 then
    begin
      if P_LANG='ENG' then V_STR_MSG:='This record is deleted!'; end if;
      if P_LANG='RUS' then V_STR_MSG:='Эта запись удалена!'; end if;
    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if V_STR_MSG is null then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_RESERVED_PHRASE
      where PHRASE=P_TABLE_NAME;

      if V_R_COUNT>0 then
        begin
          if P_LANG='ENG' then
            begin
               V_STR_MSG:='Value '||P_TABLE_NAME||' is a phrase reserved!';
            end;
          else
            begin
              V_STR_MSG:='Значение '||P_TABLE_NAME||' является зарезервированной фразой!';
            end;
          end if;
        end;
      end if;

    end;
  end if;
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  if (V_STR_MSG is null) then
    begin
      update WB_REF_WS_AIR_S_L_C_ADV
      set CH_CAI_BALANCE_ARM=P_CH_CAI_BALANCE_ARM,
        	CH_CAI_INDEX_UNIT=P_CH_CAI_INDEX_UNIT,
	        CH_CI_BALANCE_ARM=P_CH_CI_BALANCE_ARM,
	        CH_CI_INDEX_UNIT=P_CH_CI_INDEX_UNIT,
	        CH_USE_AS_DEFAULT=P_CH_USE_AS_DEFAULT,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id=P_ID;

      update WB_REF_WS_AIR_S_L_C_IDN
      set TABLE_NAME=P_TABLE_NAME,
          U_NAME=P_U_NAME,
	        U_IP=P_U_IP,
	        U_HOST_NAME=P_U_HOST_NAME,
	        DATE_WRITE=sysdate()
      where id=P_IDN;

      if P_CH_USE_AS_DEFAULT=1 then
        begin
          update WB_REF_WS_AIR_S_L_C_ADV
          set CH_USE_AS_DEFAULT=0,
              U_NAME=P_U_NAME,
	            U_IP=P_U_IP,
	            U_HOST_NAME=P_U_HOST_NAME,
	            DATE_WRITE=sysdate()
          where id=(select distinct a.id
                    from WB_REF_WS_AIR_S_L_C_IDN i join WB_REF_WS_AIR_S_L_C_ADV a
                    on i.id=P_IDN and
                       i.adv_id=a.adv_id and
                       a.id<>P_ID);
        end;
      end if;

      V_STR_MSG:='EMPTY_STRING';
    end;
  end if;

  cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

  commit;
end SP_WB_REF_WS_AIR_S_L_C_ADV_UPD;
/
