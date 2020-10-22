create or replace PROCEDURE SP_WBA_GET_AHM_DEADLD_DOORS
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
  /*
  case
    when t5.NAME = 'LOWER' then 'L'
    when t5.NAME = 'MAIN' then 'M'
    when t5.NAME = 'UPPER' then 'U'
  end
  */
  t5.NAME deck_name,
  t2.ID HOLD_ID, t2.HOLD_NAME,
  t7.NAME Sec_Type,
  t4.ID SEC_ID, t4.DOOR_POSITION, t4.SEC_BAY_NAME, t4.BA_FWD ULD_BalanceArmFWD, t4.BA_AFT ULD_BalanceArmAFT
from WB_REF_WS_AIR_HLD_DECK t1 inner join WB_REF_WS_AIR_HLD_HLD_T t2 on t1.DECK_ID = t2.DECK_ID and t1.ID_AC = t2.ID_AC and t1.ID_WS = t2.ID_WS
                              inner join WB_REF_WS_AIR_HLD_CMP_T t3 on t2.ID = t3.HOLD_ID
                              inner join WB_REF_WS_AIR_SEC_BAY_T t4 on t3.CMP_NAME = t4.CMP_NAME and t3.ID_AC = t4.ID_AC and t3.ID_WS = t4.ID_WS
                              inner join WB_REF_WS_DECK t5 on t1.DECK_ID = t5.ID
                              inner join WB_REF_SEC_BAY_TYPE t7 on t4.SEC_BAY_TYPE_ID = t7.ID
                              inner join WB_REF_WS_DOORS_POSITIONS t8 on t4.DOOR_POSITION = t8.ID
where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS -- and t1.ID_BORT = vID_BORT
)

  select XMLAGG(
                  XMLELEMENT("deck ",
                              XMLATTRIBUTES(tt1.deck_name "Name"),

                              (SELECT XMLAGG(
                                  XMLELEMENT( "hold",
                                      XMLATTRIBUTES(
                                                      t1.HOLD_NAME as "Name"
                                                    ),
                                                    -- Следующий вложенный элемент
                                                    (SELECT XMLAGG(
                                                        XMLELEMENT( "door",
                                                            XMLATTRIBUTES(
                                                                            t2.ULD_BalanceArmFWD as "BalanceArmFWD",
                                                                            t2.ULD_BalanceArmAFT as "BalanceArmAFT",
                                                                            ' ' as "Side"
                                                                          )
                                                        ) ORDER BY t2.ULD_BalanceArmFWD -- t2.SEC_ID
                                                        )
                                                      from
                                                      (
                                                        select distinct ID_AC, ID_WS, ID_BORT, ULD_BalanceArmFWD, ULD_BalanceArmAFT -- SEC_ID,
                                                        from tblDecks
                                                        WHERE HOLD_ID = t1.HOLD_ID
                                                      ) t2
                                                    ) as cls
                                  ) ORDER BY t1.HOLD_NAME
                                  )
                                from
                                (
                                  select distinct HOLD_ID, HOLD_NAME
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
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_deadload_doors" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_deadload_doors" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WBA_GET_AHM_DEADLD_DOORS;
/
