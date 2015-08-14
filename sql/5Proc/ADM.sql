create or replace PACKAGE adm
AS

SUBTYPE TUserType IS user_types.code%TYPE;
utSupport       CONSTANT TUserType := 0;
utAirport       CONSTANT TUserType := 1;
utAirline       CONSTANT TUserType := 2;

PROCEDURE modify_originator(
       vid              typeb_originators.id%TYPE,
       vlast_date       typeb_originators.last_date%TYPE,
       vlang            lang_types.code%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             typeb_originators.tid%TYPE DEFAULT NULL);
PROCEDURE delete_originator(
       vid              typeb_originators.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             typeb_originators.tid%TYPE DEFAULT NULL);
PROCEDURE add_originator(
       vid       IN OUT typeb_originators.id%TYPE,
       vairline         typeb_originators.airline%TYPE,
       vairp_dep        typeb_originators.airp_dep%TYPE,
       vtlg_type        typeb_originators.tlg_type%TYPE,
       vfirst_date      typeb_originators.first_date%TYPE,
       vlast_date       typeb_originators.last_date%TYPE,
       vaddr            typeb_originators.addr%TYPE,
       vdouble_sign     typeb_originators.double_sign%TYPE,
       vdescr           typeb_originators.descr%TYPE,
       vtid             typeb_originators.tid%TYPE,
       vlang            lang_types.code%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

procedure insert_pact(vid             in pacts.id%type,
                      vsys_user_id    in users2.user_id%TYPE,
                      vfirst_date     in pacts.first_date%type,
                      vlast_date      in pacts.last_date%type,
                      vairline        in pacts.airline%type,
                      vairline_view   in VARCHAR2,
                      vairp           in pacts.airp%type,
                      vairp_view      in VARCHAR2,
                      vdescr          in pacts.descr%TYPE,
                      vsetting_user   in history_events.open_user%TYPE,
                      vstation        in history_events.open_desk%TYPE);

PROCEDURE check_hall_airp(vhall_id       IN halls2.id%TYPE,
                          vpoint_id      IN points.point_id%TYPE);
FUNCTION check_hall_airp(vhall_id       IN halls2.id%TYPE,
                         vairp          IN airps.code%TYPE) RETURN airps.code%TYPE;

FUNCTION check_right_access(vright_id IN rights_list.ida%TYPE,
                            vuser_id IN users2.user_id%TYPE,
                            vexception IN NUMBER) RETURN rights_list.ida%TYPE;

FUNCTION check_role_view_access(vrole_id IN roles.role_id%TYPE,
                                vuser_id IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_role_aro_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_role_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_role_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN roles.role_id%TYPE;

FUNCTION check_user_view_access(vuser_id1 IN users2.user_id%TYPE,
                                vuser_id2 IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_user_access(vuser_id1 IN users2.user_id%TYPE,
                           vuser_id2 IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_user_access(vuser_id1 IN users2.user_id%TYPE,
                           vuser_id2 IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN users2.user_id%TYPE;

FUNCTION check_extra_view_access(vuser_type_from IN user_types.code%TYPE,
                                 vairline_from IN airlines.code%TYPE,
                                 vairp_from IN airps.code%TYPE,
                                 vuser_type_to IN user_types.code%TYPE,
                                 vairline_to IN airlines.code%TYPE,
                                 vairp_to IN airps.code%TYPE,
                                 vuser_id IN users2.user_id%TYPE) RETURN NUMBER;
PROCEDURE check_extra_access(vuser_type_from    IN user_types.code%TYPE,
                             vairline_from      IN airlines.code%TYPE,
                             vairline_from_view IN VARCHAR2,
                             vairp_from         IN airps.code%TYPE,
                             vairp_from_view    IN VARCHAR2,
                             vuser_type_to      IN user_types.code%TYPE,
                             vairline_to        IN airlines.code%TYPE,
                             vairline_to_view   IN VARCHAR2,
                             vairp_to           IN airps.code%TYPE,
                             vairp_to_view      IN VARCHAR2,
                             vuser_id           IN users2.user_id%TYPE,
                             vexception         IN NUMBER);

FUNCTION check_airline_access(vairline  IN airlines.code%TYPE,
                              vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_airline_access(vairline  IN airlines.code%TYPE,
                              vairline_view IN VARCHAR2,
                              vuser_id  IN users2.user_id%TYPE,
                              vexception IN NUMBER) RETURN airlines.code%TYPE;
PROCEDURE check_airline_access(vairline1      IN airlines.code%TYPE,
                               vairline1_view IN VARCHAR2,
                               vairline2      IN airlines.code%TYPE,
                               vairline2_view IN VARCHAR2,
                               vuser_id       IN users2.user_id%TYPE,
                               vexception     IN NUMBER);

FUNCTION check_city_access(vcity      IN cities.code%TYPE,
                           vcity_view IN VARCHAR2,
                           vuser_id   IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN cities.code%TYPE;
FUNCTION check_city_access(vcity  IN cities.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;

FUNCTION check_airp_access(vairp  IN airps.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_airp_access(vairp      IN airps.code%TYPE,
                           vairp_view IN VARCHAR2,
                           vuser_id   IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN airps.code%TYPE;
PROCEDURE check_airp_access(vairp1      IN airps.code%TYPE,
                            vairp1_view IN VARCHAR2,
                            vairp2      IN airps.code%TYPE,
                            vairp2_view IN VARCHAR2,
                            vuser_id    IN users2.user_id%TYPE,
                            vexception  IN NUMBER);

FUNCTION check_desk_view_access(vdesk IN desks.code%TYPE,
                                vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_desk_access(vdesk IN desks.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_desk_access(vdesk IN desks.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN desks.code%TYPE;

FUNCTION check_desk_grp_view_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                                    vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_desk_grp_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                               vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_desk_grp_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                               vuser_id  IN users2.user_id%TYPE,
                               vexception IN NUMBER) RETURN desk_grp.grp_id%TYPE;

FUNCTION check_desk_code(vcode IN desks.code%TYPE) RETURN desks.code%TYPE;

FUNCTION check_desk_grp_id(vdesk     IN desks.code%TYPE,
                           vdesk_grp IN desk_grp.grp_id%TYPE) RETURN desk_grp.grp_id%TYPE;

PROCEDURE check_typeb_addrs(vtlg_type      IN typeb_addrs.tlg_type%TYPE,
                            vairline       IN typeb_addrs.airline%TYPE,
                            vairline_view  IN VARCHAR2,
                            vairp_dep      IN typeb_addrs.airp_dep%TYPE,
                            vairp_dep_view IN VARCHAR2,
                            vairp_arv      IN typeb_addrs.airp_arv%TYPE,
                            vairp_arv_view IN VARCHAR2,
                            vuser_id       IN users2.user_id%TYPE,
                            vexception     IN NUMBER);

FUNCTION check_edi_addr(vaddr IN et_addr_set.edi_addr%TYPE) RETURN et_addr_set.edi_addr%TYPE;
FUNCTION check_apis_edi_addr(vaddr IN apis_sets.edi_addr%TYPE) RETURN apis_sets.edi_addr%TYPE;
PROCEDURE check_apis_sets(vairline      IN apis_sets.airline%TYPE,
                          vcountry_dep  IN apis_sets.country_dep%TYPE,
                          vcountry_arv  IN apis_sets.country_arv%TYPE,
                          vformat       IN apis_sets.format%TYPE,
                          vedi_addr     IN OUT apis_sets.edi_addr%TYPE,
                          vedi_own_addr IN OUT apis_sets.edi_own_addr%TYPE,
                          vlang         IN lang_types.code%TYPE);


FUNCTION check_validator_access(vvalidator IN validator_types.code%TYPE,
                                vuser_id  IN users2.user_id%TYPE) RETURN NUMBER;

FUNCTION check_misc_set_access(vtype IN misc_set.type%TYPE,
                               vuser_id IN users2.user_id%TYPE) RETURN NUMBER;
FUNCTION check_misc_set_access(vtype IN misc_set.type%TYPE,
                               vuser_id IN users2.user_id%TYPE,
                               vexception IN NUMBER) RETURN misc_set.type%TYPE;

PROCEDURE insert_user(vlogin             IN users2.login%TYPE,
                      vdescr             IN users2.descr%TYPE,
                      vtype              IN users2.type%TYPE,
                      vpr_denial         IN users2.pr_denial%TYPE,
                      vsys_user_id       IN users2.user_id%TYPE,
                      vtime_fmt  	 IN user_sets.time%TYPE,
                      vdisp_airline_fmt  IN user_sets.disp_airline%TYPE,
                      vdisp_airp_fmt  	 IN user_sets.disp_airp%TYPE,
                      vdisp_craft_fmt  	 IN user_sets.disp_craft%TYPE,
                      vdisp_suffix_fmt   IN user_sets.disp_suffix%TYPE,
                      vsetting_user      IN history_events.open_user%TYPE,
                      vstation           IN history_events.open_desk%TYPE);

PROCEDURE update_user(vold_user_id       IN users2.user_id%TYPE,
                      vlogin             IN users2.login%TYPE,
                      vtype              IN users2.type%TYPE,
                      vpr_denial         IN users2.pr_denial%TYPE,
                      vsys_user_id       IN users2.user_id%TYPE,
                      vtime_fmt  	 IN user_sets.time%TYPE,
                      vdisp_airline_fmt  IN user_sets.disp_airline%TYPE,
                      vdisp_airp_fmt  	 IN user_sets.disp_airp%TYPE,
                      vdisp_craft_fmt  	 IN user_sets.disp_craft%TYPE,
                      vdisp_suffix_fmt   IN user_sets.disp_suffix%TYPE,
                      vsetting_user      IN history_events.open_user%TYPE,
                      vstation           IN history_events.open_desk%TYPE);

PROCEDURE delete_user(vold_user_id  IN users2.user_id%TYPE,
                      vsys_user_id  IN users2.user_id%TYPE,
                      vsetting_user IN history_events.open_user%TYPE,
                      vstation      IN history_events.open_desk%TYPE);

PROCEDURE check_airline_codes(vid               IN airlines.id%TYPE,
                              vcode             IN airlines.code%TYPE,
                              vcode_lat         IN airlines.code_lat%TYPE,
                              vcode_icao        IN airlines.code_icao%TYPE,
                              vcode_icao_lat    IN airlines.code_icao_lat%TYPE,
                              vaircode          IN airlines.aircode%TYPE,
                              vlang             IN lang_types.code%TYPE);

PROCEDURE check_country_codes(vid               IN countries.id%TYPE,
                              vcode             IN countries.code%TYPE,
                              vcode_lat         IN countries.code_lat%TYPE,
                              vcode_iso         IN countries.code_iso%TYPE,
                              vlang             IN lang_types.code%TYPE);

PROCEDURE check_city_codes(vid               IN cities.id%TYPE,
                           vcode             IN cities.code%TYPE,
                           vcode_lat         IN cities.code_lat%TYPE,
                           vlang             IN lang_types.code%TYPE);

PROCEDURE check_airp_codes(vid               IN airps.id%TYPE,
                           vcode             IN airps.code%TYPE,
                           vcode_lat         IN airps.code_lat%TYPE,
                           vcode_icao        IN airps.code_icao%TYPE,
                           vcode_icao_lat    IN airps.code_icao_lat%TYPE,
                           vlang             IN lang_types.code%TYPE);

PROCEDURE check_craft_codes(vid               IN crafts.id%TYPE,
                            vcode             IN crafts.code%TYPE,
                            vcode_lat         IN crafts.code_lat%TYPE,
                            vcode_icao        IN crafts.code_icao%TYPE,
                            vcode_icao_lat    IN crafts.code_icao_lat%TYPE,
                            vlang             IN lang_types.code%TYPE);

TYPE TFieldTitles IS TABLE OF cache_fields.title%TYPE INDEX BY cache_fields.name%TYPE;

TYPE TCacheInfo IS RECORD
(
  title		cache_tables.title%TYPE,
  field_title	TFieldTitles
);

FUNCTION get_cache_info(vcode	cache_tables.code%TYPE) RETURN TCacheInfo;

FUNCTION check_trfer_sets_interval(str         IN VARCHAR2,
                                   cache_table IN cache_tables.code%TYPE,
                                   cache_field IN cache_fields.name%TYPE,
                                   vlang       IN lang_types.code%TYPE) RETURN trfer_sets.min_interval%TYPE;

FUNCTION get_locale_text(vid   IN locale_messages.id%TYPE,
                        vlang IN locale_messages.lang%TYPE) RETURN locale_messages.text%TYPE;

FUNCTION get_trfer_set_flts(vtrfer_set_id trfer_set_flts.trfer_set_id%TYPE,
                            vpr_onward    trfer_set_flts.pr_onward%TYPE) RETURN VARCHAR2;

PROCEDURE insert_trfer_sets(vid		      IN trfer_sets.id%TYPE,
                            vairline_in       IN trfer_sets.airline_in%TYPE,
                            vairline_in_view  IN VARCHAR2,
                            vflt_no_in 	      IN VARCHAR2,
                            vairp 	      IN trfer_sets.airp%TYPE,
                            vairp_view        IN VARCHAR2,
                            vairline_out      IN trfer_sets.airline_out%TYPE,
                            vairline_out_view IN VARCHAR2,
                            vflt_no_out       IN VARCHAR2,
                            vtrfer_permit     IN trfer_sets.trfer_permit%TYPE,
                            vtrfer_outboard   IN trfer_sets.trfer_outboard%TYPE,
                            vtckin_permit     IN trfer_sets.tckin_permit%TYPE,
                            vtckin_waitlist   IN trfer_sets.tckin_waitlist%TYPE,
                            vtckin_norec      IN trfer_sets.tckin_norec%TYPE,
                            vmin_interval     IN VARCHAR2,
                            vmax_interval     IN VARCHAR2,
                            vsys_user_id      IN users2.user_id%TYPE,
                            vlang             IN lang_types.code%TYPE,
                            vsetting_user     IN history_events.open_user%TYPE,
                            vstation          IN history_events.open_desk%TYPE);

PROCEDURE delete_trfer_sets(vid		      IN trfer_sets.id%TYPE,
                            vsetting_user     IN history_events.open_user%TYPE,
                            vstation          IN history_events.open_desk%TYPE);

PROCEDURE add_codeshare_set(
       vid       IN OUT codeshare_sets.id%TYPE,
       vairline_oper    codeshare_sets.airline_oper%TYPE,
       vflt_no_oper     codeshare_sets.flt_no_oper%TYPE,
       vairp_dep        codeshare_sets.airp_dep%TYPE,
       vairline_mark    codeshare_sets.airline_mark%TYPE,
       vflt_no_mark     codeshare_sets.flt_no_mark%TYPE,
       vpr_mark_norms   codeshare_sets.pr_mark_norms%TYPE,
       vpr_mark_bp      codeshare_sets.pr_mark_bp%TYPE,
       vpr_mark_rpt     codeshare_sets.pr_mark_rpt%TYPE,
       vdays            codeshare_sets.days%TYPE,
       vfirst_date      codeshare_sets.first_date%TYPE,
       vlast_date       codeshare_sets.last_date%TYPE,
       vnow             DATE,
       vtid             codeshare_sets.tid%TYPE,
       vpr_denial       NUMBER,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

PROCEDURE modify_codeshare_set(
       vid              codeshare_sets.id%TYPE,
       vlast_date       codeshare_sets.last_date%TYPE,
       vnow             DATE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             codeshare_sets.tid%TYPE DEFAULT NULL);

PROCEDURE delete_codeshare_set(
       vid              codeshare_sets.id%TYPE,
       vnow             DATE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             codeshare_sets.tid%TYPE DEFAULT NULL);

PROCEDURE check_pers_weights(vid            IN pers_weights.id%TYPE,
                             vairline       IN pers_weights.airline%TYPE,
                             vcraft         IN pers_weights.craft%TYPE,
                             vbort          IN OUT pers_weights.bort%TYPE,
                             vclass         IN pers_weights.class%TYPE,
                             vsubclass      IN pers_weights.subclass%TYPE,
                             vpr_summer     IN pers_weights.pr_summer%TYPE,
                             vfirst_date    IN VARCHAR2,
                             vlast_date     IN VARCHAR2,
                             first_date_out OUT pers_weights.first_date%TYPE,
                             last_date_out  OUT pers_weights.last_date%TYPE,
                             vlang          IN lang_types.code%TYPE);

PROCEDURE check_not_airp_user(vuser_id   IN users2.user_id%TYPE,
                              vexception IN NUMBER);

PROCEDURE modify_rem_event_sets(old_set_id    rem_event_sets.set_id%TYPE,
                                old_airline   rem_event_sets.airline%TYPE,
                                old_rem_code  rem_event_sets.rem_code%TYPE,
                                vairline      rem_event_sets.airline%TYPE,
                                vrem_code     rem_event_sets.rem_code%TYPE,
                                bp            rem_event_sets.event_value%TYPE,
                                alarm_ss      rem_event_sets.event_value%TYPE,
                                pnl_sel       rem_event_sets.event_value%TYPE,
                                brd_view      rem_event_sets.event_value%TYPE,
                                brd_warn      rem_event_sets.event_value%TYPE,
                                rpt_ss        rem_event_sets.event_value%TYPE,
                                rpt_pm        rem_event_sets.event_value%TYPE,
                                ckin_view     rem_event_sets.event_value%TYPE,
                                typeb_psm     rem_event_sets.event_value%TYPE,
                                typeb_pil     rem_event_sets.event_value%TYPE,
                                vsetting_user history_events.open_user%TYPE,
                                vstation      history_events.open_desk%TYPE);

PROCEDURE check_stage_access(vstage_id     IN graph_stages.stage_id%TYPE,
                             vairline      IN airlines.code%TYPE,
                             vairline_view IN VARCHAR2,
                             vairp         IN airps.code%TYPE,
                             vairp_view    IN VARCHAR2,
                             vuser_id      IN users2.user_id%TYPE,
                             vexception    IN NUMBER);

PROCEDURE check_sita_addr(str         IN VARCHAR2,
                          cache_table IN cache_tables.code%TYPE,
                          cache_field IN cache_fields.name%TYPE,
                          vlang       IN lang_types.code%TYPE);
PROCEDURE check_canon_name(str         IN VARCHAR2,
                           cache_table IN cache_tables.code%TYPE,
                           cache_field IN cache_fields.name%TYPE,
                           vlang       IN lang_types.code%TYPE);

PROCEDURE delete_create_points(vid            typeb_addrs.id%TYPE,
                               vsetting_user  history_events.open_user%TYPE,
                               vstation       history_events.open_desk%TYPE);

FUNCTION get_typeb_option(vid         typeb_addrs.id%TYPE,
                          vbasic_type typeb_addr_options.tlg_type%TYPE,
                          vcategory   typeb_addr_options.category%TYPE) RETURN typeb_addr_options.value%TYPE;

PROCEDURE delete_typeb_options(vid            typeb_addrs.id%TYPE,
                               vsetting_user  history_events.open_user%TYPE,
                               vstation       history_events.open_desk%TYPE);

PROCEDURE sync_typeb_options(vid            typeb_addrs.id%TYPE,
                             vbasic_type    typeb_addr_options.tlg_type%TYPE,
                             vsetting_user  history_events.open_user%TYPE,
                             vstation       history_events.open_desk%TYPE);

PROCEDURE sync_LDM_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vcabin_baggage typeb_addr_options.value%TYPE,
                           vgender        typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE);

PROCEDURE sync_LCI_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vequipment     typeb_addr_options.value%TYPE,
                           vweignt_avail  typeb_addr_options.value%TYPE,
                           vseating       typeb_addr_options.value%TYPE,
                           vweight_mode   typeb_addr_options.value%TYPE,
                           vseat_restrict typeb_addr_options.value%TYPE,
                           vpas_totals    typeb_addr_options.value%TYPE,
                           vbag_totals    typeb_addr_options.value%TYPE,
                           vpas_distrib   typeb_addr_options.value%TYPE,
                           vseat_plan     typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE);

PROCEDURE sync_PRL_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vcreate_point  typeb_addr_options.value%TYPE,
                           vpax_state     typeb_addr_options.value%TYPE,
                           vrbd           typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE);

PROCEDURE sync_BSM_options(vid              typeb_addrs.id%TYPE,
                           vbasic_type      typeb_addr_options.tlg_type%TYPE,
                           vclass_of_travel typeb_addr_options.value%TYPE,
                           vsetting_user    history_events.open_user%TYPE,
                           vstation         history_events.open_desk%TYPE);

PROCEDURE sync_PNL_options(vid              typeb_addrs.id%TYPE,
                           vbasic_type      typeb_addr_options.tlg_type%TYPE,
                           vforwarding      typeb_addr_options.value%TYPE,
                           vsetting_user    history_events.open_user%TYPE,
                           vstation         history_events.open_desk%TYPE);

PROCEDURE modify_airline_offices(vid           airline_offices.id%TYPE,
                                 vairline      airline_offices.airline%TYPE,
                                 vcountry      airline_offices.country%TYPE,
                                 vairp         airline_offices.airp%TYPE,
                                 vcontact_name airline_offices.contact_name%TYPE,
                                 vphone        airline_offices.phone%TYPE,
                                 vfax          airline_offices.fax%TYPE,
                                 vto_apis      airline_offices.to_apis%TYPE,
                                 vlang         lang_types.code%TYPE,
                                 vsetting_user history_events.open_user%TYPE,
                                 vstation      history_events.open_desk%TYPE);

PROCEDURE insert_roles(vname          roles.name%TYPE,
                       vairline       airlines.code%TYPE,
                       vairp          airps.code%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE);

PROCEDURE modify_roles(vrole_id       roles.role_id%TYPE,
                       vname          roles.name%TYPE,
                       vairline       airlines.code%TYPE,
                       vairp          airps.code%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE);

PROCEDURE delete_roles(vrole_id       roles.role_id%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE);

END adm;
/
