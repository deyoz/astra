create or replace PROCEDURE SP_WB_FOR_CALLS(cXML_in IN clob, cXML_out OUT CLOB)
AS
sQuery varchar2(100);
nElem_id number;
nNewLS number := 1;
P_AC_ID number;
P_WS_ID number;
P_BORT_ID number;
P_DATE varchar(20);
P_TIME varchar(20);
cXML_Proc_in clob;
cXML_curr clob;

-- Список запросов:
/*
get_load_control
get_airco_list
get_airport_list
get_flight_types_list
get_borts_list
get_config_list
get_ahm_airline_uld_types
get_airco_codes_list
get_loadsheet_text
get_loadsheet_list
get_ahm_airline_info
get_ahm_aircraft_info
get_ahm_airline_load_codes
get_ahm_airline_uld_types
get_ahm_deadload_decks
get_ahm_deadload_doors
get_ahm_passenger_and_baggage_weights
get_ahm_seatplan
get_deadload_details
get_dow_data
get_ahm_dow_referencies
get_flight_stages
get_passengers_details
get_seating_details
get_user_settings
*/

CURSOR cProcList IS
    select distinct EXTRACTVALUE(value(b), '/proc/@name') procname, EXTRACTVALUE(value(b), '/proc') procname2
    from table(XMLSequence(Extract(xmltype(cXML_in), '/root/proc'))) b;

BEGIN
  FOR ProcList IN cProcList
  LOOP
    -- Вытащить все параметры для процедуры ProcList.procname, найдя нужный узел, построить входную строку и выполнить процедуру
    sQuery := '/root[1]/proc[@name="' || ProcList.procname || '"]';

    case ProcList.procname
      -- ListControl
      when 'get_load_control' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
          into nElem_id
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

          SP_WB_REF_GET_LOAD_CONTROL(cXML_Proc_in, cXML_curr);
        end;
      -- Список авиакомпаний
      when 'get_airco_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
          into nElem_id
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

          SP_WB_GET_AIRCO_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Список аэропортов
      when 'get_airport_list' then
        begin
          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname || '"></root>';

          SP_WB_GET_AIRPORT_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Список типов ВС в авиакомпании
      when 'get_flight_types_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@ac_id'))
          into nElem_id
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" ac_id="' || to_char(nElem_id)
                          || '"></root>';

          SP_WB_GET_FLIGHT_TYPES_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Список бортов для типа ВС
      when 'get_borts_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@ac_id')), to_number(extractValue(xmltype(cXML_in), sQuery || '/@ws_id'))
          into P_AC_ID, P_WS_ID
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" ac_id="' || to_char(P_AC_ID)
                                            || '" ws_id="' || to_char(P_WS_ID)
                          || '"></root>';

          SP_WB_GET_BORTS_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Список конфигураций для борта
      when 'get_config_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@ac_id')), to_number(extractValue(xmltype(cXML_in), sQuery || '/@ws_id')), to_number(extractValue(xmltype(cXML_in), sQuery || '/@bort_id'))
          into P_AC_ID, P_WS_ID, P_BORT_ID
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" ac_id="' || to_char(P_AC_ID)
                                            || '" ws_id="' || to_char(P_WS_ID)
                                            || '" bort_id="' || to_char(P_BORT_ID)
                          || '"></root>';

          SP_WB_GET_CONFIG_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Список ULD типов багажа
      when 'get_ahm_airline_uld_types' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
          into nElem_id
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

          SP_WB_REF_GET_ULD_TYPES(cXML_Proc_in, cXML_curr);
        end;
      -- Список префиксов для авиакомпании
      when 'get_airco_codes_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@ac_id'))
          into nElem_id
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" ac_id="' || to_char(nElem_id)
                          || '"></root>';

          SP_WB_GET_AIRCO_CODES_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- Текст для LoadSheet
      when 'get_loadsheet_text' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id')), extractValue(xmltype(cXML_in),'/@date'), extractValue(xmltype(cXML_in),'/@time')
          into nElem_id, P_DATE, P_TIME
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" elem_id="' || to_char(nElem_id)
                                            || '" date="' || P_DATE
                                            || '" time="' || P_TIME
                          || '"></root>';

          SP_WB_GET_LOADSHEET_TEXT(cXML_Proc_in, cXML_curr);
        end;
      -- Список документов LoadSheet для рейса
      when 'get_loadsheet_list' then
        begin
          select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id')), to_number(extractValue(xmltype(cXML_in), sQuery || '/@new'))
          into nElem_id, nNewLS
          from dual;

          cXML_Proc_in := '<?xml version="1.0"?>'
                          || '<root name="' || ProcList.procname
                                            || '" elem_id="' || to_char(nElem_id)
                                            || '" new="' || to_char(nNewLS)
                          || '"></root>';

          SP_WB_GET_LOADSHEET_LIST(cXML_Proc_in, cXML_curr);
        end;
      -- список параметров для условий выборки из временной таблицы расписания
      when 'get_ahm_airline_info' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_AHM_AIRLINE_INFO(cXML_Proc_in, cXML_curr);
      end;
      -- данные по борту
      when 'get_ahm_aircraft_info' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_AHM_AIRCR_INFO(cXML_Proc_in, cXML_curr);
      end;
      -- Коды багажа
      when 'get_ahm_airline_load_codes' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_LOAD_CODES(cXML_Proc_in, cXML_curr);
      end;
      -- ULD типы
      when 'get_ahm_airline_uld_types' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_ULD_BORT_TYPES(cXML_Proc_in, cXML_curr);
      end;
      -- Информация по багажу
      when 'get_ahm_deadload_decks' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_AHM_DEADLD_DECKS(cXML_Proc_in, cXML_curr);
      end;
      -- Двери
      when 'get_ahm_deadload_doors' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_AHM_DEADLD_DOORS(cXML_Proc_in, cXML_curr);
      end;
      -- Классы по пассажирам и багажу
      when 'get_ahm_passenger_and_baggage_weights' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_FLIGHT_CLASSES(cXML_Proc_in, cXML_curr);
      end;
      -- Расположение кресел
      when 'get_ahm_seatplan' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_AHM_SEATPLAN(cXML_Proc_in, cXML_curr);
      end;
      -- Багаж ????? Доработать???
      when 'get_deadload_details' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_DEAD_DETAILS(cXML_Proc_in, cXML_curr);
      end;
      -- Настройки DOW
      when 'get_dow_data' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_DOW_DATA(cXML_Proc_in, cXML_curr);
      end;
      -- Данные по DOW в AHM
      when 'get_ahm_dow_referencies' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_DOW_REF(cXML_Proc_in, cXML_curr);
      end;
      -- Общие настройки по рейсу
      when 'get_flight_stages' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_FLIGHT_STAGES(cXML_Proc_in, cXML_curr);
      end;
      -- Информация о пассажирах
      when 'get_passengers_details' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_PASSENGERS(cXML_Proc_in, cXML_curr);
      end;
      -- Пассажиры на местах
      when 'get_seating_details' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_SEATING_DETAILS(cXML_Proc_in, cXML_curr);
      end;
      -- Масштаб
      when 'get_user_settings' then
      begin
        select to_number(extractValue(xmltype(cXML_in), sQuery || '/@elem_id'))
        into nElem_id
        from dual;

        cXML_Proc_in := '<?xml version="1.0"?>'
                        || '<root name="' || ProcList.procname || '" elem_id="' || to_char(nElem_id) || '"></root>';

        SP_WB_REF_GET_USER_SETTINGS(cXML_Proc_in, cXML_curr);
      end;
      else
        cXML_curr := '';
    end case;

    if (cXML_curr is not NULL) then
    begin
      select replace(cXML_curr, '<?xml', '<xml') into cXML_curr from dual;
      select replace(cXML_curr, '?>', '/>') into cXML_curr from dual;
    end;
    end if;

    cXML_out := cXML_out || cXML_curr;
  END LOOP;

  cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_query_list" result="ok">' || cXML_out || '</root>';
END SP_WB_FOR_CALLS;
/
