create or replace PACKAGE utils
AS

/*
DROP TABLE update_code_progress;
CREATE TABLE update_code_progress
(
  table_name VARCHAR2(50),
  column_name VARCHAR2(50),
  num_rows NUMBER,
  partition_name VARCHAR2(50),
  updated NUMBER,
  update_priority NUMBER(1)
);
*/

PROCEDURE airline_tab_num_rows(show_oper BOOLEAN,
                               show_hist BOOLEAN,
                               show_arx BOOLEAN);
PROCEDURE airp_tab_num_rows(show_oper BOOLEAN,
                            show_hist BOOLEAN,
                            show_arx BOOLEAN);
PROCEDURE city_tab_num_rows(show_oper BOOLEAN,
                            show_hist BOOLEAN,
                            show_arx BOOLEAN);
PROCEDURE airline_count(airline_code airlines.code%TYPE,
                        max_num_rows user_tables.num_rows%TYPE,
                        show_oper BOOLEAN,
                        show_hist BOOLEAN,
                        show_arx BOOLEAN);
PROCEDURE airp_count(airp_code airps.code%TYPE,
                     max_num_rows user_tables.num_rows%TYPE,
                     show_oper BOOLEAN,
                     show_hist BOOLEAN,
                     show_arx BOOLEAN);
PROCEDURE city_count(city_code cities.code%TYPE,
                     max_num_rows user_tables.num_rows%TYPE,
                     show_oper BOOLEAN,
                     show_hist BOOLEAN,
                     show_arx BOOLEAN);
PROCEDURE airline_print_updates(old_airline_code airlines.code%TYPE,
                                new_airline_code airlines.code%TYPE,
                                max_rows user_tables.num_rows%TYPE,
                                show_oper BOOLEAN,
                                show_hist BOOLEAN,
                                show_arx BOOLEAN);
PROCEDURE airp_print_updates(old_airp_code airps.code%TYPE,
                             new_airp_code airps.code%TYPE,
                             max_rows user_tables.num_rows%TYPE,
                             show_oper BOOLEAN,
                             show_hist BOOLEAN,
                             show_arx BOOLEAN);
PROCEDURE city_print_updates(old_city_code cities.code%TYPE,
                             new_city_code cities.code%TYPE,
                             max_rows user_tables.num_rows%TYPE,
                             show_oper BOOLEAN,
                             show_hist BOOLEAN,
                             show_arx BOOLEAN);
PROCEDURE airline_update_oper(old_airline_code airlines.code%TYPE,
                              new_airline_code airlines.code%TYPE,
                              max_rows user_tables.num_rows%TYPE,
                              with_commit BOOLEAN);
PROCEDURE airp_update_oper(old_airp_code airps.code%TYPE,
                           new_airp_code airps.code%TYPE,
                           max_rows user_tables.num_rows%TYPE,
                           with_commit BOOLEAN);
PROCEDURE city_update_oper(old_city_code cities.code%TYPE,
                           new_city_code cities.code%TYPE,
                           max_rows user_tables.num_rows%TYPE,
                           with_commit BOOLEAN);
PROCEDURE airline_update_arx(old_airline_code airlines.code%TYPE,
                             new_airline_code airlines.code%TYPE,
                             max_rows user_tables.num_rows%TYPE);
PROCEDURE airp_update_arx(old_airp_code airps.code%TYPE,
                          new_airp_code airps.code%TYPE,
                          max_rows user_tables.num_rows%TYPE);
PROCEDURE city_update_arx(old_city_code cities.code%TYPE,
                          new_city_code cities.code%TYPE,
                          max_rows user_tables.num_rows%TYPE);
PROCEDURE view_update_progress;
PROCEDURE users_logoff_al(new_airline_code airlines.code%TYPE);
PROCEDURE users_logoff_ap(new_airp_code airps.code%TYPE);

PROCEDURE disable_constraints(codes NUMBER);
PROCEDURE enable_constraints(codes NUMBER);

card_like_pattern CONSTANT VARCHAR2(100)   :='([1-9][0-9][0-9]|[0-9][1-9][0-9]|[0-9][0-9][1-9])[0-9] *[0-9]{2}[0-9]{2} *[0-9]{4} *[0-9]{4}';
card_instr_pattern CONSTANT VARCHAR2(100)  :=card_like_pattern;
card_replace_pattern CONSTANT VARCHAR2(100):='([0-9]{4})( *)([0-9]{2})([0-9]{2})( *)([0-9]{4})( *)([0-9]{4})';
card_replace_string CONSTANT VARCHAR2(100) :='\1\2\300\50000\7\8';
rem_like_pattern CONSTANT VARCHAR2(20)     :='(FQT.|OTHS|FOID)';

FUNCTION masking_cards(src IN VARCHAR2) RETURN VARCHAR2;
PROCEDURE masking_cards_update(vtab IN VARCHAR2);

END utils;
/
