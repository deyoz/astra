create or replace PROCEDURE SP_WBA_GET_AHM_DEADLD_DECKS
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC, t1.ID_WS, t1.ID_BORT
  into vID_AC, vID_WS, vID_BORT
  from WB_SHED t1
  where t1.ID = P_ID and rownum <= 1;

  cXML_out := '<?xml version="1.0" ?><root>';

with tblDecks as
(
  select t1.ID, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.DECK_ID,
  t5.NAME deck_name,
  t2.ID HOLD_ID, t2.HOLD_NAME, t2.MAX_WEIGHT MaxWeight, t2.MAX_VOLUME MaxVolume, t2.LA_CENTROID LateralCentroid, t2.LA_FROM LateralArmLH, t2.LA_TO LateralArmLR, t2.BA_FWD BalanceArmFWD, t2.BA_AFT BalanceArmAFT,
  t3.CMP_NAME, t3.MAX_WEIGHT CMP_MaxWeight, t3.MAX_VOLUME CMP_MaxVolume, t3.LA_CENTROID CMP_LateralCentroid, t3.LA_FROM CMP_LateralArmLH, t3.LA_TO CMP_LateralArmLR, t3.BA_FWD CMP_BalanceArmFWD, t3.BA_AFT CMP_BalanceArmAFT,
  t7.NAME Sec_Type,
  t10.NAME ULD_TYPE, t4.SEC_BAY_NAME, t4.MAX_WEIGHT ULD_MaxWeight, t4.MAX_VOLUME ULD_MaxVolume, t4.LA_CENTROID ULD_LateralCentroid, t4.LA_FROM ULD_LateralArmLH, t4.LA_TO ULD_LateralArmLR, t4.BA_FWD ULD_BalanceArmFWD, t4.BA_AFT ULD_BalanceArmAFT,
  -- t9.NAME
  LISTAGG(t9.NAME, ',') within group (order by t9.NAME) IATAType
from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                              inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                              inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                              inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                              inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                              left join WB_REF_WS_AIR_SEC_BAY_TT t8 on t4.ID = t8.T_ID
                              left join WB_REF_ULD_IATA t9 on t8.ULD_IATA_ID = t9.ID
                              left join WB_REF_ULD_TYPES t10 on t9.TYPE_ID = t10.ID
where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
group by t1.ID, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.DECK_ID,
  t5.NAME,
  t2.ID, t2.HOLD_NAME, t2.MAX_WEIGHT, t2.MAX_VOLUME, t2.LA_CENTROID, t2.LA_FROM, t2.LA_TO, t2.BA_FWD, t2.BA_AFT, t3.CMP_NAME, t3.MAX_WEIGHT, t3.MAX_VOLUME, t3.LA_CENTROID, t3.LA_FROM, t3.LA_TO, t3.BA_FWD,
    t3.BA_AFT, t7.NAME, t10.NAME, t4.SEC_BAY_NAME, t4.MAX_WEIGHT, t4.MAX_VOLUME, t4.LA_CENTROID, t4.LA_FROM, t4.LA_TO, t4.BA_FWD, t4.BA_AFT
)
  select XMLAGG(
                  XMLELEMENT("deck ",
                              XMLATTRIBUTES(tt1.deck_name "Name"),

                              (SELECT XMLAGG(
                                  XMLELEMENT( "hold",
                                      XMLATTRIBUTES(
                                                      t1.HOLD_NAME as "Name",
                                                      t1.MAXWEIGHT as "MaxWeight",
                                                      t1.MAXVOLUME as "MaxVolume",
                                                      t1.LateralCentroid as "LateralCentroid",
                                                      t1.LateralArmLH as "LateralArmLH",
                                                      t1.LateralArmLR as "LateralArmLR",
                                                      t1.BalanceArmFWD as "BalanceArmFWD",
                                                      t1.BalanceArmAFT as "BalanceArmAFT"
                                                    ),
                                                    -- Следующий вложенный элемент
                                                    (SELECT XMLAGG(
                                                        XMLELEMENT( "compartment",
                                                            XMLATTRIBUTES(
                                                                            t2.CMP_NAME as "Name",
                                                                            t2.CMP_MAXWEIGHT as "MaxWeight",
                                                                            t2.CMP_MAXVOLUME as "MaxVolume",
                                                                            t2.CMP_LateralCentroid as "LateralCentroid",
                                                                            t2.CMP_LateralArmLH as "LateralArmLH",
                                                                            t2.CMP_LateralArmLR as "LateralArmLR",
                                                                            t2.CMP_BalanceArmFWD as "BalanceArmFWD",
                                                                            t2.CMP_BalanceArmAFT as "BalanceArmAFT"
                                                                          )
                                                                          ,
                                                                          -- Следующий вложенный элемент - Container
                                                                          (SELECT XMLAGG(
                                                                              XMLELEMENT( "uld",
                                                                                  XMLATTRIBUTES(
                                                                                                  t3.SEC_BAY_NAME as "Name",
                                                                                                  t3.IATATYPE as "PossibleIATATypes",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  nvl(t3.ULD_MAXVOLUME, 0) as "MaxVolume",
                                                                                                  t3.ULD_LateralCentroid as "LateralCentroid",
                                                                                                  t3.ULD_LateralArmLH as "LateralArmLH",
                                                                                                  t3.ULD_LateralArmLR as "LateralArmLR",
                                                                                                  t3.ULD_BalanceArmFWD as "BalanceArmFWD",
                                                                                                  t3.ULD_BalanceArmAFT as "BalanceArmAFT"
                                                                                                )
                                                                                                -- ,
                                                                                                -- Следующий вложенный элемент
                                                                                                -- (SELECT XMLAGG(
                                                                              ) ORDER BY t2.CMP_NAME
                                                                              )
                                                                            from
                                                                            (
                                                                              select distinct SEC_BAY_NAME, ULD_MAXWEIGHT, ULD_MAXVOLUME, ULD_LateralCentroid, ULD_LateralArmLH, ULD_LateralArmLR, ULD_BalanceArmFWD, ULD_BalanceArmAFT, IATATYPE
                                                                              from tblDecks
                                                                              WHERE CMP_NAME = t2.CMP_NAME and ID_AC = t2.ID_AC and ID_WS = t2.ID_WS and ID_BORT = t2.ID_BORT
                                                                                  and SEC_TYPE = 'ULD' and ULD_TYPE = 'Container'
                                                                            ) t3
                                                                          ) as cls,
                                                                          -- Следующий вложенный элемент - Pallets
                                                                          (SELECT XMLAGG(
                                                                              XMLELEMENT( "pallet",
                                                                                  XMLATTRIBUTES(
                                                                                                  t3.SEC_BAY_NAME as "Name",
                                                                                                  t3.IATATYPE as "PossibleIATATypes",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  nvl(t3.ULD_MAXVOLUME, 0) as "MaxVolume",
                                                                                                  t3.ULD_LateralCentroid as "LateralCentroid",
                                                                                                  t3.ULD_LateralArmLH as "LateralArmLH",
                                                                                                  t3.ULD_LateralArmLR as "LateralArmLR",
                                                                                                  t3.ULD_BalanceArmFWD as "BalanceArmFWD",
                                                                                                  t3.ULD_BalanceArmAFT as "BalanceArmAFT"
                                                                                                )
                                                                              ) ORDER BY t2.CMP_NAME
                                                                              )
                                                                            from
                                                                            (
                                                                              select distinct SEC_BAY_NAME, ULD_MAXWEIGHT, ULD_MAXVOLUME, ULD_LateralCentroid, ULD_LateralArmLH, ULD_LateralArmLR, ULD_BalanceArmFWD, ULD_BalanceArmAFT, IATATYPE
                                                                              from tblDecks
                                                                              WHERE CMP_NAME = t2.CMP_NAME and ID_AC = t2.ID_AC and ID_WS = t2.ID_WS and ID_BORT = t2.ID_BORT
                                                                                  and SEC_TYPE = 'ULD' and ULD_TYPE = 'Pallets'
                                                                            ) t3
                                                                          ) as cls,
                                                                          -- Следующий вложенный элемент - Bulk
                                                                          (SELECT XMLAGG(
                                                                              XMLELEMENT( "bulk",
                                                                                  XMLATTRIBUTES(
                                                                                                  t3.SEC_BAY_NAME as "Name",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  nvl(t3.ULD_MAXVOLUME, 0) as "MaxVolume",
                                                                                                  t3.ULD_LateralCentroid as "LateralCentroid",
                                                                                                  t3.ULD_LateralArmLH as "LateralArmLH",
                                                                                                  t3.ULD_LateralArmLR as "LateralArmLR",
                                                                                                  t3.ULD_BalanceArmFWD as "BalanceArmFWD",
                                                                                                  t3.ULD_BalanceArmAFT as "BalanceArmAFT"
                                                                                                )
                                                                              ) ORDER BY t2.CMP_NAME
                                                                              )
                                                                            from
                                                                            (
                                                                              select distinct SEC_BAY_NAME, ULD_MAXWEIGHT, ULD_MAXVOLUME, ULD_LateralCentroid, ULD_LateralArmLH, ULD_LateralArmLR, ULD_BalanceArmFWD, ULD_BalanceArmAFT
                                                                              from tblDecks
                                                                              WHERE CMP_NAME = t2.CMP_NAME and ID_AC = t2.ID_AC and ID_WS = t2.ID_WS and ID_BORT = t2.ID_BORT
                                                                                  and SEC_TYPE = 'Bulk'
                                                                            ) t3
                                                                          ) as cls


                                                        ) ORDER BY t2.CMP_NAME
                                                        )
                                                      from
                                                      (
                                                        select distinct ID_AC, ID_WS, ID_BORT,
                                                          CMP_NAME, CMP_MAXWEIGHT, CMP_MAXVOLUME, CMP_LateralCentroid, CMP_LateralArmLH, CMP_LateralArmLR, CMP_BalanceArmFWD, CMP_BalanceArmAFT
                                                        from tblDecks
                                                        WHERE HOLD_ID = t1.HOLD_ID
                                                      ) t2
                                                    ) as cls
                                  ) ORDER BY t1.BalanceArmFWD -- t1.HOLD_ID
                                  )
                                from
                                (
                                  select distinct HOLD_ID, HOLD_NAME, MAXWEIGHT, MAXVOLUME, LateralCentroid, LateralArmLH, LateralArmLR, BalanceArmFWD, BalanceArmAFT
                                  from tblDecks
                                  WHERE DECK_ID = tt1.DECK_ID
                                ) t1
                              ) as cls

                            )
                )
  INTO cXML_data
  from
  (
    select distinct t1.DECK_ID, t1.deck_name
    from tblDecks t1
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_deadload_decks" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_deadload_decks" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WBA_GET_AHM_DEADLD_DECKS;
/
