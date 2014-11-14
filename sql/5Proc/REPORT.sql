create or replace PACKAGE report
AS
FUNCTION get_TKNO(vpax_id IN crs_pax.pax_id%TYPE,
                  et_term IN CHAR DEFAULT '/',
                  only_TKNE IN NUMBER DEFAULT NULL) RETURN VARCHAR2;

FUNCTION get_TKNO2(vpax_id IN crs_pax.pax_id%TYPE,
                   et_term IN CHAR DEFAULT '/',
                   only_TKNE IN NUMBER DEFAULT NULL) RETURN VARCHAR2;

FUNCTION get_PSPT(vpax_id IN crs_pax.pax_id%TYPE,
                  with_issue_country IN NUMBER DEFAULT 0,
                  vlang   IN lang_types.code%TYPE DEFAULT 'RU') RETURN VARCHAR2;

FUNCTION get_PSPT2(vpax_id IN crs_pax.pax_id%TYPE,
                   with_issue_country IN NUMBER DEFAULT 0,
                   vlang   IN lang_types.code%TYPE DEFAULT 'RU') RETURN VARCHAR2;

FUNCTION get_trfer_airline(str	        IN airlines.code%TYPE,
                           pr_lat       IN INTEGER DEFAULT NULL) RETURN airlines.code%TYPE;
PRAGMA RESTRICT_REFERENCES (get_trfer_airline, WNDS ,WNPS ,RNPS);

FUNCTION get_trfer_airp(str	        IN airps.code%TYPE,
                        pr_lat          IN INTEGER DEFAULT NULL) RETURN airps.code%TYPE;
PRAGMA RESTRICT_REFERENCES (get_trfer_airp, WNDS ,WNPS ,RNPS);

FUNCTION get_trfer_airp_name(str	IN airps.code%TYPE,
                             pr_lat     IN INTEGER) RETURN airps.name%TYPE;
PRAGMA RESTRICT_REFERENCES (get_trfer_airp_name, WNDS ,WNPS ,RNPS);

FUNCTION get_last_trfer(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2;
FUNCTION get_last_trfer_airp(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2;
FUNCTION get_last_tckin_seg(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2;
FUNCTION get_airp_period_last_date(vairp pacts.airp%TYPE,
                                   period_first_date pacts.first_date%TYPE) RETURN DATE;
FUNCTION get_airline_period_last_date(vairline pacts.airline%TYPE,
                                      period_first_date pacts.first_date%TYPE) RETURN DATE;

END report;
/
