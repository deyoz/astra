create or replace PROCEDURE SP_WBA_GET_FLIGHT_DATA
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType := null; P_ELEM_ID number:=-1; vID_AC number; vID_WS number; vID_BORT number; cXML_temp clob := '';

BEGIN
  cXML_out := '';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  -- Настройки пользователя в центроке
  SP_WBA_USER_SETTINGS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_user_settings" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Единицы измерения ВС
  SP_WBA_AHM_AIRLINE_INFO(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_airline_info" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Единицы измерения борта
  SP_WBA_AHM_AIRCRAFT_INFO(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_aircraft_info" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Коды для багажа
  SP_WBA_AHM_LOAD_CODES(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_airline_load_codes" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- ULD-коды
  SP_WBA_AHM_ULD_TYPES(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_airline_uld_types" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Данные по багажным отделениям
  SP_WBA_AHM_DEADLOAD_DECKS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_deadload_decks" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Двери салона
  -- get_ahm_deadload_doors
  SP_WBA_AHM_DEADLOAD_DOORS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_deadload_doors" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Загрузка багажа
  SP_WBA_DEADLOAD_DETAILS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_deadload_details" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Весовые справочники
  -- get_ahm_dow_referencies
  SP_WBA_AHM_DOW_REFERENCIES(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_dow_referencies" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Весовые характеристики
  SP_WBA_DOW_DATA(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_dow_data" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Веса по классам
  SP_WBA_AHM_PASS_BAG_WEIGHT(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_passenger_and_baggage_weights" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- План салона
  SP_WBA_AHM_SETPLAN(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_ahm_seatplan" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Рассадка пассажиров
  SP_WBA_SEATING_DETAILS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_seating_details" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  -- Рассадка по классам
  -- get_passengers_details
  -- Здесь не нужно обрамлять block'ом, это делается внутри процедуры
  SP_WBA_PASSENGERS_DETAILS(cXML_in, cXML_temp);
  cXML_out := cXML_out || cXML_temp;

  -- Список аэропортов
  SP_WBA_AHM_AIRPORTS(cXML_in, cXML_temp);
  cXML_temp := '<block name="get_airports" result="ok">' || cXML_temp || '</block>';
  cXML_out := cXML_out || cXML_temp;

  if length(cXML_out) > 0 then
  begin
    cXML_out := concat(
                        concat(
                                to_clob('<?xml version="1.0"  encoding="utf-8"?><root name="get_flight_data" elem_id="' || to_char(P_ELEM_ID) || '" result="ok">'),
                                cXML_out
                              ), to_clob('</root> ')
                      );
  end;
  else
  begin
    cXML_out := '<?xml version="1.0"  encoding="utf-8"?><root name="get_flight_data" result="ok"/>';
  end;
  end if;

  commit;
END SP_WBA_GET_FLIGHT_DATA;
/
