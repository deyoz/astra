create or replace PROCEDURE SP_WB_REF_GET_ULD_BORT_TPS_OLD
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number;

BEGIN
  -- ������ �室��� ��ࠬ���
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ������� ᯨ᮪ ��ࠬ��஢ ��� �᫮��� �롮ન �� �६����� ⠡���� �ᯨᠭ��
  select t1.ID_AC
  into vID_AC
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  select t1.ID_WS
  into vID_WS
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  select t1.ID_BORT
  into vID_BORT
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

with tblDecks as
(
  select t1.ID, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.DECK_ID,
  case
    when t5.NAME = 'LOWER' then 'L'
    when t5.NAME = 'MAIN' then 'M'
    when t5.NAME = 'UPPER' then 'U'
  end deck_name,
  t2.ID HOLD_ID, t2.HOLD_NAME, t2.MAX_WEIGHT MaxWeight, t2.MAX_VOLUME MaxVolume, t2.LA_CENTROID LateralCentroid, t2.LA_FROM LateralArmLH, t2.LA_TO LateralArmLR, t2.BA_FWD BalanceArmFWD, t2.BA_AFT BalanceArmAFT,
  t3.CMP_NAME, t3.MAX_WEIGHT CMP_MaxWeight, t3.MAX_VOLUME CMP_MaxVolume, t3.LA_CENTROID CMP_LateralCentroid, t3.LA_FROM CMP_LateralArmLH, t3.LA_TO CMP_LateralArmLR, t3.BA_FWD CMP_BalanceArmFWD, t3.BA_AFT CMP_BalanceArmAFT,
  t7.NAME Sec_Type,
  t10.NAME ULD_TYPE, t4.SEC_BAY_NAME, t4.MAX_WEIGHT ULD_MaxWeight, t4.MAX_VOLUME ULD_MaxVolume, t4.LA_CENTROID ULD_LateralCentroid, t4.LA_FROM ULD_LateralArmLH, t4.LA_TO ULD_LateralArmLR, t4.BA_FWD ULD_BalanceArmFWD, t4.BA_AFT ULD_BalanceArmAFT,
  -- t9.NAME
  LISTAGG(t9.NAME, ',') within group (order by t9.NAME) IATAType, min(t9.ID) ULD_ID
from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                              inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                              inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                              inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                              inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                              left join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                              left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                              left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT
group by t1.ID, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.DECK_ID,
  case
    when t5.NAME = 'LOWER' then 'L'
    when t5.NAME = 'MAIN' then 'M'
    when t5.NAME = 'UPPER' then 'U'
  end,
  t2.ID, t2.HOLD_NAME, t2.MAX_WEIGHT, t2.MAX_VOLUME, t2.LA_CENTROID, t2.LA_FROM, t2.LA_TO, t2.BA_FWD, t2.BA_AFT, t3.CMP_NAME, t3.MAX_WEIGHT, t3.MAX_VOLUME, t3.LA_CENTROID, t3.LA_FROM, t3.LA_TO, t3.BA_FWD,
    t3.BA_AFT, t7.NAME, t10.NAME, t4.SEC_BAY_NAME, t4.MAX_WEIGHT, t4.MAX_VOLUME, t4.LA_CENTROID, t4.LA_FROM, t4.LA_TO, t4.BA_FWD, t4.BA_AFT
)

  select XMLAGG(
                  XMLELEMENT("uld_type",
                              XMLATTRIBUTES(tt1.ID "id", tt1.IATAType "IATAType", tt1.IsDefault "IsDefault", tt1.IsPallet "IsPallet", tt1.BeginSerial "BeginSerial",
                                            tt1.EndSerial "EndSerial", tt1.OwnerCode "OwnerCode", tt1.TareWeight "TareWeight", tt1.MaxWeight "MaxWeight")
                            )
                )
  INTO cXML_data
  from
  (
    select distinct t3.ID, t4.IATAType IATAType, -- t3.NAME IATAType,
      case when t1.BY_DEFAULT = 1 then 'Y' else 'N' end IsDefault,
      case when t1.ULD_TYPE_ID = 0 then 'Y' else 'N' end IsPallet,
      t1.BEG_SER_NUM BeginSerial,
      case
        when t1.END_SER_NUM = 'EMPTY_STRING' then ' '
        when t1.END_SER_NUM is null then ' '
        else t1.END_SER_NUM
      end EndSerial,
      t1.OWNER_CODE OwnerCode, t1.TARE_WEIGHT TareWeight, t1.MAX_WEIGHT MaxWeight
    from WB_REF_AIRCO_ULD t1 inner join WB_REF_ULD_TYPES t2 on t1.ULD_TYPE_ID = t2.ID
                              inner join WB_REF_ULD_IATA t3 on t1.ULD_IATA_ID = t3.ID
                              inner join tblDecks t4 on t3.ID = t4.ULD_ID
                              inner join (select t.ULD_IATA_ID, max(t.BY_DEFAULT) BY_DEFAULT from WB_REF_AIRCO_ULD t group by t.ULD_IATA_ID) t5 on t1.ULD_IATA_ID = t5.ULD_IATA_ID and t1.BY_DEFAULT = t5.BY_DEFAULT
    where t1.ID_AC = vID_AC
  ) tt1;

  if cXML_data is not NULL then
  begin
    select replace(cXML_data, 'EndSerial=" "', 'EndSerial=""') into cXML_out from dual;
    cXML_out := '<?xml version="1.0" ?><root name="get_ahm_airline_uld_types" result="ok">' || cXML_out || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_airline_uld_types" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_ULD_BORT_TPS_OLD;
/
