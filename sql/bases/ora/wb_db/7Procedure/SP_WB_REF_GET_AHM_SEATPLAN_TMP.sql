create or replace PROCEDURE SP_WB_REF_GET_AHM_SEATPLAN_TMP
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; Units varchar2(100); vID_AC number; vID_WS number; vID_BORT number; vID_SL number;

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

  select t1.ID_SL
  into vID_SL
  from WB_REF_WS_SCHED_TEMP t1
  where t1.ID = P_ID;

  select NAME_ENG_SMALL into Units
  from
  (
    select t2.NAME_ENG_SMALL
    from WB_REF_WS_AIR_MEASUREMENT t1 inner join WB_REF_MEASUREMENT_LENGTH t2 on t1.LENGTH_ID = t2.ID
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT and SYSDATE >= t1.DATE_FROM
    order by t1.DATE_FROM desc
  )
  where rownum <= 1;

  cXML_out := '<?xml version="1.0" ?><root>';

  with tblSeats as
  (
    select tt1.Units, tt1.ID_AC, tt1.ID_WS, tt1.ID_BORT, tt1.CABIN_SECTION, tt1.ROW_NUMBER, tt1.Seat_Ident, tt2.LengthOfArm, tt2.Codes, tt2.AisleLeft, tt2.AisleRight
    from
    (
      select Units, t1.ID_AC, t1.ID_WS, t1.ID_BORT, t4.CABIN_SECTION, t4.ROW_NUMBER, t2.NAME Seat_Ident
      from WB_REF_WS_AIR_S_L_A_U_S t1 inner join WB_REF_WS_SEATS_NAMES t2 on t1.SEAT_ID = t2.ID
                                      inner join WB_REF_WS_AIR_S_L_C_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS and t1.ADV_ID = t3.ADV_ID
                                      inner join WB_REF_WS_AIR_S_L_PLAN t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t1.ADV_ID = t4.ADV_ID
      where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT and t3.ID = vID_SL
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
                  when t1.BALANCE_ARM is not null then t1.BALANCE_ARM -- ���� ������� �� ��⮢� 12.01.2017
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
          where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT
        ) t1
        group by t1.ID_AC, t1.ID_WS, t1.ID_BORT, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.LengthOfArm, t1.Seat_Ident
      ) tt1
    ) tt2 on tt1.ID_AC = tt2.ID_AC and tt1.ID_WS = tt2.ID_WS and tt1.ID_BORT = tt2.ID_BORT and tt1.CABIN_SECTION = tt2.CABIN_SECTION and tt1.ROW_NUMBER = tt2.ROW_NUMBER and tt1.Seat_Ident = tt2.Seat_Ident
  )

  -- t1.ID_AC, t1.ID_WS, t1.ID_BORT, t4.CABIN_SECTION, t4.ROW_NUMBER, t2.NAME Seat_Ident

  select XMLAGG(
                  XMLELEMENT("seatplan",
                              XMLATTRIBUTES(Units "Units"),
                              -- ������騩 �������� �����
                              (SELECT XMLAGG(
                                              XMLELEMENT("section",
                                                          XMLATTRIBUTES(t2.CABIN_SECTION as "Name"),
                                                          -- ������騩 �������� �����
                                                          (SELECT XMLAGG(
                                                                          XMLELEMENT("row",
                                                                                      XMLATTRIBUTES(t3.ROW_NUMBER as "RowNo", t3.LengthOfArm as "LengthOfArm"),

                                                                                      -- ������騩 �������� �����
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
                                                              select distinct ROW_NUMBER, LengthOfArm
                                                              from tblSeats
                                                              WHERE CABIN_SECTION = t2.CABIN_SECTION
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
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_seatplan" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_seatplan" result="ok"></root>';
  end;
  end if;

  commit;
END SP_WB_REF_GET_AHM_SEATPLAN_TMP;
/
