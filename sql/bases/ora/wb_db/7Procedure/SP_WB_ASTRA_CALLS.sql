create or replace PROCEDURE SP_WB_ASTRA_CALLS(cXML_in IN clob, cXML_out OUT clob)
AS
l_XML clob;

-- для обращений со стороны Астры
temp varchar(1000);

sKey varchar(100) := '';

/*
-- Список запросов:
get_airports
get_load_control
get_airco_list
get_airport_list
get_flight_types_list
get_borts_list
get_config_list
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
set_deadload_details
get_dow_data
set_dow_data
get_ahm_dow_referencies
get_flight_stages
get_passengers_details
get_seating_details
set_seating_details
get_user_settings
set_user_settings
get_fuel_info
set_fuel_info
set_configuration
get_query_list
get_query_set_flight
get_schedule
get_cabin_baggage_by_zone
set_cabin_baggage_by_zone
save_loadinstruction
get_loadinstruction_by_date
get_query_save_set_flight
get_take_of_index
get_max_fuel
get_test
*/
BEGIN
  -- Форматирование чисовых значений (вместо ',' ставится '.')
  select t1.value into temp from v$nls_parameters t1 where parameter = 'NLS_NUMERIC_CHARACTERS';
  execute immediate 'ALTER SESSION SET NLS_NUMERIC_CHARACTERS = ''.,''';

  DBMS_LOB.CREATETEMPORARY(l_XML, false, 2); -- 2 makes the temporary only available in this call
  DBMS_LOB.COPY(l_XML, cXML_in, dbms_lob.getlength(cXML_in), 1, 1);

  select extractValue(xmltype(l_XML),'/root[1]/@name')
  into sKey
  from dual;

  case sKey
    when 'get_airports' then
      begin
        SP_WB_REF_GET_AIRPORTS(l_XML, cXML_out);
      end;
    when 'get_load_control' then
      begin
        SP_WBA_GET_LOAD_CONTROL(l_XML, cXML_out); -- SP_WB_REF_GET_LOAD_CONTROL
      end;
    when 'get_airco_list' then
      begin
        SP_WB_GET_AIRCO_LIST(l_XML, cXML_out);
      end;
    when 'get_airport_list' then
      begin
        SP_WB_GET_AIRPORT_LIST(l_XML, cXML_out);
      end;
    when 'get_flight_types_list' then
      begin
        SP_WB_GET_FLIGHT_TYPES_LIST(l_XML, cXML_out);
      end;
    when 'get_borts_list' then
      begin
        SP_WB_GET_BORTS_LIST(l_XML, cXML_out);
      end;
    when 'get_config_list' then
      begin
        SP_WB_GET_CONFIG_LIST(l_XML, cXML_out);
      end;
    when 'get_ahm_airline_uld_types' then
      begin
        SP_WBA_GET_ULD_TYPES(l_XML, cXML_out); -- SP_WB_REF_GET_ULD_TYPES
      end;
    when 'get_airco_codes_list' then
      begin
        SP_WB_GET_AIRCO_CODES_LIST(l_XML, cXML_out);
      end;
    when 'get_loadsheet_text' then
      begin
        SP_WB_GET_LOADSHEET_TEXT(l_XML, cXML_out);
      end;
      when 'get_loadsheet_list' then
        begin
          SP_WB_GET_LOADSHEET_LIST(l_XML, cXML_out);
        end;
    when 'get_ahm_airline_info' then
      begin
        SP_WBA_GET_AHM_AIRLINE_INFO(l_XML, cXML_out); -- SP_WB_REF_GET_AHM_AIRLINE_INFO
      end;
    when 'get_ahm_aircraft_info' then
      begin
        SP_WBA_GET_AHM_AIRCR_INFO(l_XML, cXML_out); -- SP_WB_REF_GET_AHM_AIRCR_INFO
      end;
    when 'get_ahm_airline_load_codes' then
      begin
        SP_WBA_GET_LOAD_CODES(l_XML, cXML_out); -- SP_WB_REF_GET_LOAD_CODES
      end;
    when 'get_ahm_airline_uld_types' then
      begin
        SP_WBA_GET_ULD_BORT_TYPES(l_XML, cXML_out); -- SP_WB_REF_GET_ULD_BORT_TYPES
      end;
    when 'get_ahm_deadload_decks' then
      begin
        SP_WBA_GET_AHM_DEADLD_DECKS(l_XML, cXML_out); -- SP_WB_REF_GET_AHM_DEADLD_DECKS
      end;
    when 'get_ahm_deadload_doors' then
      begin
        SP_WBA_GET_AHM_DEADLD_DOORS(l_XML, cXML_out); -- SP_WB_REF_GET_AHM_DEADLD_DOORS
      end;
    when 'get_ahm_passenger_and_baggage_weights' then
      begin
        SP_WBA_GET_FLIGHT_CLASSES(l_XML, cXML_out); -- SP_WB_REF_GET_FLIGHT_CLASSES
      end;
    when 'get_ahm_seatplan' then
      begin
        SP_WBA_GET_AHM_SEATPLAN(l_XML, cXML_out); -- SP_WB_REF_GET_AHM_SEATPLAN
      end;
    when 'get_deadload_details' then
      begin
        SP_WBA_GET_DEAD_DETAILS(l_XML, cXML_out); -- SP_WB_REF_GET_DEAD_DETAILS
      end;
    when 'set_deadload_details' then
      begin
        SP_WB_SET_DEADLOAD_DETAILS(l_XML, cXML_out); -- SP_WB_REF_GET_DEAD_DETAILS
      end;
    when 'get_dow_data' then
      begin
        SP_WBA_GET_DOW_DATA(l_XML, cXML_out); -- SP_WB_REF_GET_DOW_DATA
      end;
    when 'set_dow_data' then
      begin
        SP_WBA_SET_DOW_DATA(l_XML, cXML_out); -- SP_WB_SET_DOW_DATA
      end;
    when 'get_ahm_dow_referencies' then
      begin
        SP_WBA_GET_DOW_REF(l_XML, cXML_out); -- SP_WB_REF_GET_DOW_REF
      end;
    when 'get_flight_stages' then
      begin
        SP_WBA_GET_FLIGHT_STAGES(l_XML, cXML_out); -- SP_WB_REF_GET_FLIGHT_STAGES
      end;
    when 'get_passengers_details' then
      begin
        SP_WBA_GET_PASSENGERS(l_XML, cXML_out); -- SP_WB_REF_GET_PASSENGERS
      end;
    when 'set_passengers_details' then
      begin
        SP_WB_SET_PASSENGERS(l_XML, cXML_out); -- SP_WB_REF_GET_PASSENGERS
      end;
    when 'get_seating_details' then
      begin
        SP_WBA_GET_SEATING_DETAILS(l_XML, cXML_out); -- SP_WB_REF_GET_SEATING_DETAILS
      end;
    when 'set_seating_details' then
      begin
        SP_WB_SET_SEATING_DETAILS(l_XML, cXML_out); -- SP_WB_REF_GET_SEATING_DETAILS
      end;
    when 'get_user_settings' then
      begin
        SP_WB_REF_GET_USER_SETTINGS(l_XML, cXML_out);
      end;
    when 'set_user_settings' then
      begin
        SP_WB_SET_USER_SETTINGS(l_XML, cXML_out);
      end;
    when 'get_fuel_info' then
      begin
        SP_WBA_GET_FUEL_INFO(l_XML, cXML_out); -- SP_WB_REF_GET_FUEL_INFO
      end;
    when 'set_fuel_info' then
      begin
        SP_WB_SET_FUEL_INFO(l_XML, cXML_out);
      end;
    when 'set_configuration' then
      begin
        SP_WBA_SET_CONFIGURATION(l_XML, cXML_out); -- SP_WB_SET_CONFIGURATION
      end;
    when 'get_query_list' then
      begin
        SP_WB_FOR_CALLS(l_XML, cXML_out);
      end;
    when 'get_query_set_flight' then
      begin
        SP_WBA_GET_SET_FLIGHT(l_XML, cXML_out); -- SP_WB_GET_SET_FLIGHT
      end;
    when 'get_query_save_set_flight' then
      begin
        SP_WBA_SET_FLIGHT(l_XML, cXML_out);
      end;
    when 'get_schedule' then
      begin
        SP_WB_GET_SCHEDULE(l_XML, cXML_out); -- SP_WB_REF_GET_SHEDULE
      end;
    when 'get_cabin_baggage_by_zone' then
      begin
        SP_WBA_GET_CABIN_BAGG_ZONE(l_XML, cXML_out);
      end;
    when 'set_cabin_baggage_by_zone' then
      begin
        SP_WB_SET_CABIN_BAGG_BY_ZONE(l_XML, cXML_out);
      end;
    when 'save_loadinstruction' then
      begin
        SP_WBA_SAVE_LOADINSTRUCTION(l_XML, cXML_out);
      end;
    when 'get_loadinstruction_by_date' then
      begin
        SP_WB_GET_LOADINSTRUCT_BY_DATE(l_XML, cXML_out);
      end;
    when 'save_loadsheet' then
      begin
        SP_WBA_SAVE_LOADSHEET(l_XML, cXML_out);
      end;
    when 'save_loadsheet_doc' then
      begin
        SP_WB_SAVE_LOADSHEET_DOC(l_XML, cXML_out);
      end;
    when 'get_query_save_set_flight' then
      begin
        SP_WB_SAVE_FLIGHT(l_XML, cXML_out);
      end;
    when 'get_take_of_index' then
      begin
        SP_WB_TAKE_OF_INDEX(l_XML, cXML_out);
      end;
    when 'get_max_fuel' then
      begin
        SP_WB__MAX_FUEL(l_XML, cXML_out);
      end;
    when 'get_query_set_flight' then
      begin
        SP_WBA_SET_FLIGHT(l_XML, cXML_out);
      end;
    when 'get_test' then
      begin
        SP_WB___TEST(l_XML, cXML_out);
      end;
    when 'get_flight_data' then
      begin
        SP_WBA_GET_FLIGHT_DATA(l_XML, cXML_out);
      end;
      else
        cXML_out := '';
  end case;

  -- Возвращаем исходные параметры форматирования числовых значений
  execute immediate 'ALTER SESSION SET NLS_NUMERIC_CHARACTERS = ''' || temp || '''';
END SP_WB_ASTRA_CALLS;
/
