create or replace PROCEDURE SP_WBA_SEATING_DETAILS(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- Блок данных с единицами измерения по борту
-- Создал: Набатов Т.Е.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; Units varchar2(100);
TABLEVAR varchar(40):='SeatingDetails';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- Получить входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ЗДЕСЬ НАЧИНАЕТСЯ ИНДИВИДУАЛЬНАЯ ЧАСТЬ
  cXML_out := '';

  -- Если в Calcs есть данные по рейсу - берем оттуда, иначе из AHM или из заглушки
  select count(t1.ELEM_ID)
  into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
  begin
    select t1.XML_DATA
    INTO cXML_out
    from
    (
      select t1.XML_DATA
      from WB_CALCS_XML t1
      where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
    ) t1
    where rownum <= 1;
  end;
  end if;

  if (length(cXML_out) = 0) or (cXML_out is null) then
  begin
    -- Получить список параметров для условий выборки из временной таблицы расписания
    select t1.ID_AC, t1.ID_WS, t1.ID_BORT --  , t1.ID_SL -- компоновка - на будущее
    into vID_AC, vID_WS, vID_BORT -- , vID_SL
    from WB_SHED t1
    where t1.ID = P_ID and rownum <= 1;

    -- Времменное значение ID конфигурации борта, пока в расписании не появится поле конфигурации
    select count(t2.ID)
    into REC_COUNT2
    from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                                inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
    where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS;

    if REC_COUNT2 > 0 then
    begin
      select t2.ID
      into vID_SL
      from WB_REF_WS_AIR_REG_WGT t1 inner join WB_REF_WS_AIR_S_L_C_IDN t2 on t1.S_L_ADV_ID = t2.ADV_ID
                                  inner join WB_REF_WS_AIR_DOW_ADV t3 on t1.ID_AC = t3.ID_AC and t1.ID_WS = t3.ID_WS
      where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and rownum <= 1;

      cXML_out := '<?xml version="1.0" ?><root>';

      -- Переделал, чтобы не выводились фиктивные кресла
      with tblSeats as
      (
        select distinct t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t1.Seat_Ident, t1.CLASS_CODE, t1.OccupiedBy
        from
        (
          select t1.ID_AC, t1.ID_WS, t1.IDN, t1.CABIN_SECTION, t1.ROW_NUMBER, t3.NAME Seat_Ident, t6.CLASS_CODE, 'AVAILABLE' OccupiedBy, t7.S_SEAT_ID
          from WB_REF_WS_AIR_S_L_PLAN t1 inner join WB_REF_WS_AIR_S_L_P_S t2 on t1.ID = t2.PLAN_ID
                                            inner join WB_REF_WS_SEATS_NAMES t3 on t2.SEAT_ID = t3.ID
                                            inner join WB_REF_WS_AIR_S_L_C_ADV t4 on t1.ID_AC = t4.ID_AC and t1.ID_WS = t4.ID_WS and t2.ADV_ID = t4.ADV_ID
                                            inner join WB_REF_WS_AIR_SL_CI_T t6 on t1.ID_AC = t6.ID_AC and t1.ID_WS = t6.ID_WS and t4.ID = t6.ADV_ID and (t1.ROW_NUMBER >= t6.FIRST_ROW) and (t1.ROW_NUMBER <= t6.LAST_ROW)
                                            inner join WB_REF_WS_AIR_S_L_P_S_P t7 on t2.ID = t7.S_SEAT_ID
                                            inner join WB_REF_WS_SEATS_PARAMS t8 on t7.PARAM_ID = t8.ID

          where t1.ID_AC = vID_AC and t1.ID_WS = vID_WS and t4.IDN = vID_SL and t8.NAME != 'Y' -- and t1.ID_BORT = vID_BORT  -- в дальнейшем предусмотреть IDN (м.б. несколько записей с датой изменения для каждого IDN)
        ) t1
        left join
        (
          select t7.S_SEAT_ID
          from WB_REF_WS_AIR_S_L_P_S_P t7 inner join WB_REF_WS_SEATS_PARAMS t8 on t7.PARAM_ID = t8.ID
          where t8.NAME = 'Y'
        ) t2 on t1.S_SEAT_ID = t2.S_SEAT_ID
        where t2.S_SEAT_ID is null
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

      if cXML_Data is not null then
      begin
        cXML_out := cXML_data.getClobVal();
      end;
      else
      begin
        cXML_out := '';
      end;
      end if;
    end;
    else
    begin
      cXML_out := '';
    end;
    end if;
  end;
  end if;
END SP_WBA_SEATING_DETAILS;
/
