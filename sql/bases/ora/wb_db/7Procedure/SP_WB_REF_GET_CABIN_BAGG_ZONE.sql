create or replace PROCEDURE SP_WB_REF_GET_CABIN_BAGG_ZONE(cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data XMLType;
P_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
TABLEVAR varchar(40):='CabinBaggageByZone';
REC_COUNT number:=0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cXML_out
    from
    (
      select t1.XML_VALUE
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
    ) t1
    where rownum <= 1;
  end;
  else
  -- Берем данные из AHM
  begin
    -- Получить список параметров для условий выборки из временной таблицы расписания
    select t1.ID_AC
    into vID_AC
    from WB_SCHED t1 -- WB_REF_WS_SCHED_TEMP
    where t1.ID = P_ID;

    select t1.ID_WS
    into vID_WS
    from WB_SCHED t1
    where t1.ID = P_ID;

    select t1.ID_BORT
    into vID_BORT
    from WB_SCHED t1
    where t1.ID = P_ID;

    select t1.ID_SL
    into vID_SL
    from WB_SCHED t1
    where t1.ID = P_ID;

    cXML_out := '<?xml version="1.0" ?><root>';

    with tblSeats as
    (
      select t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.Seat_Ident, t1.ID_CLASS, t1.CLASS_CODE, t1.CabinBaggage
      from
      (
        select t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t3.NAME Seat_Ident, t6.ID ID_CLASS, t6.CLASS_CODE, '0' CabinBaggage
        from WB_REF_WS_AIR_S_L_PLAN t1 inner join WB_REF_WS_AIR_S_L_P_S t2 on t1.ID = t2.PLAN_ID
                                          inner join WB_REF_WS_SEATS_NAMES t3 on t2.SEAT_ID = t3.ID
                                          inner join WB_REF_WS_AIR_S_L_C_ADV t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t2.ADV_ID = t4.ADV_ID
                                          inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t4.ID = t6.ADV_ID and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
        where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t4.ID = vID_SL -- and t1.ID_BORT = vID_BORT  -- в дальнейшем предусмотреть IDN (м.б. несколько записей с датой изменения для каждого IDN)
      ) t1
    )

    select XMLAGG(
                  XMLELEMENT("class",
                              XMLATTRIBUTES(t1.CLASS_CODE as "code"),
                              -- Следующий вложенный элемент
                              (SELECT XMLAGG(
                                              XMLELEMENT("section",
                                                          XMLATTRIBUTES(
                                                                          t3.CABIN_SECTION as "Name",
                                                                          t3.CabinBaggage as "CabinBaggage"
                                                                        )
                                                        ) ORDER BY t3.CABIN_SECTION
                                            )
                                from
                                (
                                  select distinct CABIN_SECTION, CabinBaggage
                                  from tblSeats
                                  WHERE CLASS_CODE = t1.CLASS_CODE
                                ) t3
                              ) as node
                            ) ORDER BY t1.ID_CLASS
                )
    INTO cXML_data
    from
    (
      select distinct ID_CLASS, CLASS_CODE
      from tblSeats
    ) t1;

    if cXML_data is not NULL then
    begin
      cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_cabin_baggage_by_zone" result="ok">' || cXML_data.getClobVal() || '</root>';
    end;
    else
    begin
      cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_cabin_baggage_by_zone" result="ok"></root>';
    end;
    end if;
  end;
  end if;
END SP_WB_REF_GET_CABIN_BAGG_ZONE;
/
