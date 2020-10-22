create or replace PROCEDURE SP_WBA_GET_AHM_SEATPLAN
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; Units varchar2(100); vID_AC number; vID_WS number; vID_BORT number; vID_SL number;

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC, t1.ID_WS, t1.ID_BORT -- , t1.ID_SL -- компоновка - на будущее
  into vID_AC, vID_WS, vID_BORT -- , vID_SL
  from WB_SHED t1
  where t1.ID = P_ID and rownum <= 1;

  -- Времменное значение ID конфигурации борта, пока в расписании не появится поле конфигурации
  select t2.ID
  into vID_SL
  from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                              inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
  where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and rownum <= 1;

  select NAME_ENG_SMALL into Units
  from
  (
    select t2.NAME_ENG_SMALL
    from WB_REF_WS_AIR_MEASUREMENT t1 inner join WB_REF_MEASUREMENT_LENGTH t2 on t1.LENGTH_ID = t2.ID
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and SYSDATE >= t1.DATE_FROM -- and t1.ID_BORT = vID_BORT
    order by t1.DATE_FROM desc
  )
  where rownum <= 1;

  cXML_out := '<?xml version="1.0" ?><root>';

  with tblSeats as
  (
    select tt1.Units, tt1.ID_AC, tt1.ID_WS, tt1.ID_BORT, tt1.CABIN_SECTION, tt1.ROW_NUMBER,
      tt1.Seat_Ident, nvl(tt2.LengthOfArm, 0) LengthOfArm, nvl(tt2.Codes, '-') Codes, to_char(nvl(tt2.AisleLeft, 'N')) AisleLeft, nvl(tt2.AisleRight, 'N') AisleRight
    from
    (
      select Units, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t4.CABIN_SECTION, t4.ROW_NUMBER, t2.NAME Seat_Ident
      from WB_REF_WS_AIR_S_L_A_U_S t1 inner join WB_REF_WS_SEATS_NAMES t2 on t1.SEAT_ID = t2.ID
                                      inner join WB_REF_WS_AIR_S_L_C_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.ADV_ID = t3.ADV_ID
                                      inner join WB_REF_WS_AIR_S_L_PLAN t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ADV_ID = t4.ADV_ID
      where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t3.IDN = vID_SL -- компоновка - на будущее -- and t1.ID_BORT = vID_BORT
    ) tt1
    left join
    (
      select Units, tt1.ID_AC, tt1.ID_WS, tt1.ID_BORT, tt1.IDN, tt1.CABIN_SECTION, tt1.ROW_NUMBER, tt1.LengthOfArm, tt1.Seat_Ident,
        tt1.Codes
        ||
        case
          when (tt1.AisleLeft= 'Y') or (tt1.AisleRight = 'Y') then 'A' else ''
        end Codes,
        case when tt1.AisleLeft is null then 'N' else tt1.AisleLeft end AisleLeft,
        case when tt1.AisleRight is null then 'N' else tt1.AisleRight end AisleRight
      from
      (
        select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.LengthOfArm, t1.Seat_Ident,
          LISTAGG(t1.Code, '') within group (order by t1.Code) Codes,
          LISTAGG(t1.AisleLeft, '') within group (order by t1.AisleLeft) AisleLeft,
          LISTAGG(t1.AisleRight, '') within group (order by t1.AisleRight) AisleRight
        from
        (
          select t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER,
                case
                  when t1.BALANCE_ARM is not null then t1.BALANCE_ARM -- пока колонка не готова 12.01.2017
                  -- when t1.LENGTH_OF_ARM is null then 100
                  else t1.LENGTH_OF_ARM
                end LengthOfArm,
                t3.NAME Seat_Ident,
                case
                  when t5.NAME in ('AL', 'AR') then ''
                  else t5.NAME
                end Code,
                case
                  when t5.NAME = 'AL' then 'Y'
                  else ''
                end AisleLeft,
                case
                  when t5.NAME = 'AR' then 'Y'
                  else ''
                end AisleRight
          from WB_REF_WS_AIR_S_L_PLAN t1 inner join WB_REF_WS_AIR_S_L_P_S t2 on t1.ID = t2.PLAN_ID
                                            inner join WB_REF_WS_AIR_S_L_P_S_P t4 on t2.ID = t4.S_SEAT_ID
                                            inner join WB_REF_WS_SEATS_NAMES t3 on t2.SEAT_ID = t3.ID
                                            inner join WB_REF_WS_SEATS_PARAMS t5 on t4.PARAM_ID = t5.ID
                                            inner join WB_REF_WS_AIR_S_L_C_ADV t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t1.ADV_ID = t6.ADV_ID -- доп.связка ADV_ID
          where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t6.IDN = vID_SL -- компоновка - на будущее -- and t1.ID_BORT = vID_BORT
        ) t1
        group by t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.LengthOfArm, t1.Seat_Ident
      ) tt1
    ) tt2 on tt1.ID_AC = tt2.ID_AC and tt1.ID_WS = tt2.ID_WS and tt1.ID_BORT = tt2.ID_BORT and tt1.CABIN_SECTION = tt2.CABIN_SECTION and tt1.ROW_NUMBER = tt2.ROW_NUMBER and tt1.Seat_Ident = tt2.Seat_Ident
  )

  -- t1.ID_AC, t1.ID_WS, t1.ID_BORT, t4.CABIN_SECTION, t4.ROW_NUMBER, t2.NAME Seat_Ident

  select XMLAGG(
                  XMLELEMENT("seatplan",
                              XMLATTRIBUTES(Units "Units"),
                              -- Следующий вложенный элемент
                              (SELECT XMLAGG(
                                              XMLELEMENT("section",
                                                          XMLATTRIBUTES(t2.CABIN_SECTION as "Name"),
                                                          -- Следующий вложенный элемент
                                                          (SELECT XMLAGG(
                                                                          XMLELEMENT("row",
                                                                                      XMLATTRIBUTES(t3.ROW_NUMBER as "RowNo", t3.LengthOfArm as "LengthOfArm"),

                                                                                      -- Следующий вложенный элемент
                                                                                      (SELECT XMLAGG(
                                                                                                      XMLELEMENT("seat",
                                                                                                                  XMLATTRIBUTES(t4.Seat_Ident as "Ident",
                                                                                                                                t4.Codes as "Codes",
                                                                                                                                t4.AisleLeft as "AisleLeft",
                                                                                                                                t4.AisleRight as "AisleRight"
                                                                                                                                )
                                                                                                                ) ORDER BY t3.ROW_NUMBER
                                                                                                    )
                                                                                        from
                                                                                        (
                                                                                          select distinct Seat_Ident, Codes, AisleLeft, AisleRight
                                                                                          from tblSeats
                                                                                          WHERE CABIN_SECTION = t2.CABIN_SECTION and ROW_NUMBER = t3.ROW_NUMBER
                                                                                        ) t4
                                                                                      ) as node



                                                                                    ) ORDER BY t3.ROW_NUMBER
                                                                        )
                                                            from
                                                            (
                                                              -- select distinct ROW_NUMBER, LengthOfArm
                                                              select ROW_NUMBER, max(LengthOfArm) LengthOfArm
                                                              from tblSeats
                                                              WHERE CABIN_SECTION = t2.CABIN_SECTION
                                                              group by ROW_NUMBER
                                                            ) t3
                                                          ) as node

                                                        ) ORDER BY t2.CABIN_SECTION
                                            )
                                from
                                (
                                  select distinct CABIN_SECTION
                                  from tblSeats
                                  WHERE Units = t1.Units
                                ) t2
                              ) as node
                            )
                )
  INTO cXML_data
  from
  (
    select distinct Units from tblSeats
  ) t1;

  if cXML_data is not NULL then
  begin
   -- select replace(cXML_data, 'AisleLeft=" "', 'AisleLeft=""') into cXML_out from dual;
    -- select replace(cXML_out, 'AisleRight=" "', 'AisleRight=""') into cXML_out from dual;
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_seatplan" result="ok">' || cXML_data.getClobVal() || '</root>';
    --cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_seatplan" result="ok">' || cXML_out || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_seatplan" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WBA_GET_AHM_SEATPLAN;
/
