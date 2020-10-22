create or replace PROCEDURE SP_WB_REF_GET_SEATING_DET_1201
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
TABLEVAR varchar(40):='SeatingDetails';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
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

    cXML_out := '<?xml version="1.0" ?><root>';

    with tblSeats as
    (
      select t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.Seat_Ident, t1.CLASS_CODE, t1.OccupiedBy
      from
      (
        select t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t3.NAME Seat_Ident, t6.CLASS_CODE, 'AVAILABLE' OccupiedBy
        from WB_REF_WS_AIR_S_L_PLAN t1 inner join WB_REF_WS_AIR_S_L_P_S t2 on t1.ID = t2.PLAN_ID
                                          inner join WB_REF_WS_SEATS_NAMES t3 on t2.SEAT_ID = t3.ID
                                          inner join WB_REF_WS_AIR_S_L_C_ADV t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t2.ADV_ID = t4.ADV_ID
                                          inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t4.ID = t6.ADV_ID and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
        where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t1.ID_BORT = vID_BORT and t4.ID = vID_SL -- в дальнейшем предусмотреть IDN (м.б. несколько записей с датой изменения для каждого IDN)
      ) t1
    )

    select XMLAGG(
                  XMLELEMENT("section",
                              XMLATTRIBUTES(t1.CABIN_SECTION as "Name"),
                              -- Следующий вложенный элемент
                              (SELECT XMLAGG(
                                              XMLELEMENT("row",
                                                          XMLATTRIBUTES(t3.ROW_NUMBER as "RowNo"),
                                                                        -- Следующий вложенный элемент
                                                                        (SELECT XMLAGG(
                                                                                        XMLELEMENT("seat",
                                                                                                    XMLATTRIBUTES(t4.Seat_Ident as "Ident",
                                                                                                                  t4.CLASS_CODE as "ClassCode",
                                                                                                                  t4.OccupiedBy as "OccupiedBy"
                                                                                                                  )
                                                                                                  ) ORDER BY t4.Seat_Ident
                                                                                      )
                                                                          from
                                                                          (
                                                                            select distinct Seat_Ident, CLASS_CODE, OccupiedBy
                                                                            from tblSeats
                                                                            WHERE CABIN_SECTION = t1.CABIN_SECTION and ROW_NUMBER = t3.ROW_NUMBER
                                                                          ) t4
                                                                        ) as node
                                                        ) ORDER BY t3.ROW_NUMBER
                                            )
                                from
                                (
                                  select distinct ROW_NUMBER
                                  from tblSeats
                                  WHERE CABIN_SECTION = t1.CABIN_SECTION
                                ) t3
                              ) as node
                            ) ORDER BY t1.CABIN_SECTION
                )
    INTO cXML_data
    from
    (
      select distinct CABIN_SECTION
      from tblSeats
    ) t1;

    if cXML_data is not NULL then
    begin
      cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_seating_details" result="ok">' || cXML_data.getClobVal() || '</root>';
    end;
    else
    begin
      cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_seating_details" result="ok"></root>';
    end;
    end if;
  end;
  end if;
END SP_WB_REF_GET_SEATING_DET_1201;
/
