create or replace PROCEDURE SP_WB_REF_GET_AHM_DDLD_DCKS_O
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType;
P_ID number:=-1;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("deck ",
                              XMLATTRIBUTES(tt1.deck_name "name"),

                              (SELECT XMLAGG(
                                  XMLELEMENT( "hold",
                                      XMLATTRIBUTES(
                                                      t1.HOLD_NAME as "name",
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
                                                                            t2.CMP_NAME as "name",
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
                                                                                                  t3.SEC_BAY_NAME as "name",
                                                                                                  t3.IATATYPE as "PossibleIATATypes",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  t3.ULD_MAXVOLUME as "MaxVolume",
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
                                                                              from vwDecks
                                                                              WHERE CMP_NAME = t2.CMP_NAME and ID_AC = t2.ID_AC and ID_WS = t2.ID_WS and ID_BORT = t2.ID_BORT
                                                                                  and SEC_TYPE = 'ULD' and ULD_TYPE = 'Container'
                                                                            ) t3
                                                                          ) as cls
                                                                          ,

                                                                          -- Следующий вложенный элемент - Pallets
                                                                          (SELECT XMLAGG(
                                                                              XMLELEMENT( "pallet",
                                                                                  XMLATTRIBUTES(
                                                                                                  t3.SEC_BAY_NAME as "name",
                                                                                                  t3.IATATYPE as "PossibleIATATypes",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  t3.ULD_MAXVOLUME as "MaxVolume",
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
                                                                              from vwDecks
                                                                              WHERE CMP_NAME = t2.CMP_NAME and ID_AC = t2.ID_AC and ID_WS = t2.ID_WS and ID_BORT = t2.ID_BORT
                                                                                  and SEC_TYPE = 'ULD' and ULD_TYPE = 'Pallets'
                                                                            ) t3
                                                                          ) as cls
                                                                          ,

                                                                          -- Следующий вложенный элемент - Bulk
                                                                          (SELECT XMLAGG(
                                                                              XMLELEMENT( "bulk",
                                                                                  XMLATTRIBUTES(
                                                                                                  t3.SEC_BAY_NAME as "name",
                                                                                                  t3.ULD_MAXWEIGHT as "MaxWeight",
                                                                                                  t3.ULD_MAXVOLUME as "MaxVolume",
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
                                                                              from vwDecks
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
                                                        from vwDecks
                                                        WHERE HOLD_ID = t1.HOLD_ID
                                                      ) t2
                                                    ) as cls
                                  ) ORDER BY t1.HOLD_NAME
                                  )
                                from
                                (
                                  select distinct HOLD_ID, HOLD_NAME, MAXWEIGHT, MAXVOLUME, LateralCentroid, LateralArmLH, LateralArmLR, BalanceArmFWD, BalanceArmAFT
                                  from vwDecks
                                  WHERE DECK_ID = tt1.DECK_ID
                                ) t1
                              ) as cls

                            )
                )
  INTO cXML_data
  from
  (
    select distinct t1.DECK_ID, t1.deck_name
    from vwDecks t1
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"><root name="get_ahm_deadload_decks" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_AHM_DDLD_DCKS_O;
/
