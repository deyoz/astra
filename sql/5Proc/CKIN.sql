create or replace PACKAGE ckin
AS
TYPE TBagInfo IS RECORD
(
grp_id    pax.grp_id%TYPE,
pax_id		pax.pax_id%TYPE,
bagAmount	NUMBER(5),
bagWeight	NUMBER(6),
rkAmount	NUMBER(5),
rkWeight	NUMBER(6)
);

bagInfo		TBagInfo;

FUNCTION get_pnr_addr(vpnr_id 	IN crs_pnr.pnr_id%TYPE,
                      vairline 	IN airlines.code%TYPE DEFAULT NULL,
                      pr_all	IN NUMBER DEFAULT 0) RETURN VARCHAR2;

FUNCTION get_pax_pnr_addr(vpax_id 	IN crs_pax.pax_id%TYPE,
                          vairline 	IN airlines.code%TYPE DEFAULT NULL,
                          pr_all	IN NUMBER DEFAULT 0) RETURN VARCHAR2;

FUNCTION get_pnr(vpax_id in crs_pax.pax_id%type) return varchar2;
PRAGMA RESTRICT_REFERENCES (get_pnr, WNDS, WNPS, RNPS);

FUNCTION next_airp(vfirst_point 	IN points.first_point%TYPE,
                   vpoint_num        	IN points.point_num%TYPE) RETURN points.airp%TYPE;

FUNCTION tranzitable(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE;

FUNCTION get_pr_tranzit(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE;

FUNCTION get_pr_tranz_reg(vpoint_id	IN points.point_id%TYPE) RETURN trip_sets.pr_tranz_reg%TYPE;

FUNCTION get_comp_id(vpoint_id	IN points.point_id%TYPE) RETURN trip_sets.comp_id%TYPE;

FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    pr_lat        IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    vlang	        IN lang_types.code%TYPE) RETURN VARCHAR2;

FUNCTION need_for_payment(vgrp_id     IN pax_grp.grp_id%TYPE,
                          vclass      IN pax_grp.class%TYPE,
                          vbag_refuse IN pax_grp.bag_refuse%TYPE,
                          vexcess     IN pax_grp.excess%TYPE) RETURN NUMBER;

FUNCTION get_excess(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id       IN pax.pax_id%TYPE) RETURN NUMBER;

FUNCTION get_bagInfo2(vgrp_id       IN pax.grp_id%TYPE,
                      vpax_id  	    IN pax.pax_id%TYPE,
                      vbag_pool_num IN pax.bag_pool_num%TYPE) RETURN TBagInfo;
FUNCTION get_bagAmount2(vgrp_id       IN pax.grp_id%TYPE,
                        vpax_id       IN pax.pax_id%TYPE,
                        vbag_pool_num IN pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_bagWeight2(vgrp_id       IN pax.grp_id%TYPE,
                        vpax_id       IN pax.pax_id%TYPE,
                        vbag_pool_num IN pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_rkAmount2(vgrp_id       IN pax.grp_id%TYPE,
                       vpax_id  	   IN pax.pax_id%TYPE,
                       vbag_pool_num IN pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_rkWeight2(vgrp_id       IN pax.grp_id%TYPE,
                       vpax_id  	   IN pax.pax_id%TYPE,
                       vbag_pool_num IN pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_crs_priority(vcrs           IN crs_set.crs%TYPE,
                          vairline       IN crs_set.airline%TYPE,
                          vflt_no        IN crs_set.flt_no%TYPE,
                          vairp_dep      IN crs_set.airp_dep%TYPE) RETURN NUMBER;
PRAGMA RESTRICT_REFERENCES (get_crs_priority, WNDS, WNPS, RNPS);

FUNCTION get_crs_ok(vpoint_id	IN points.point_id%TYPE) RETURN NUMBER;

PROCEDURE crs_recount(vpoint_id	IN points.point_id%TYPE);
PROCEDURE recount(vpoint_id	IN points.point_id%TYPE);

FUNCTION delete_grp_trfer(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER;
FUNCTION delete_grp_tckin_segs(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER;
PROCEDURE check_grp(vgrp_id     pax_grp.grp_id%TYPE);

PROCEDURE set_trip_sets(vpoint_id	  IN points.point_id%TYPE,
                        use_seances IN NUMBER);

FUNCTION get_main_pax_id(vgrp_id IN pax_grp.grp_id%TYPE,
                         include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE;
FUNCTION get_main_pax_id2(vgrp_id IN pax_grp.grp_id%TYPE,
                          include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE;
FUNCTION get_bag_pool_pax_id(vgrp_id       IN pax.grp_id%TYPE,
                             vbag_pool_num IN pax.bag_pool_num%TYPE,
                             include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE;

FUNCTION bag_pool_refused(vgrp_id       IN bag2.grp_id%TYPE,
                          vbag_pool_num IN bag2.bag_pool_num%TYPE,
                          vclass        IN pax_grp.class%TYPE,
                          vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER;

FUNCTION bag_pool_boarded(vgrp_id       IN bag2.grp_id%TYPE,
                          vbag_pool_num IN bag2.bag_pool_num%TYPE,
                          vclass        IN pax_grp.class%TYPE,
                          vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER;

FUNCTION excess_boarded(vgrp_id       IN pax_grp.grp_id%TYPE,
                        vclass        IN pax_grp.class%TYPE,
                        vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER;

PROCEDURE delete_typeb_data(vpoint_id  tlg_trips.point_id%TYPE,
                            vsystem    typeb_sender_systems.system%TYPE,
                            vsender    typeb_sender_systems.sender%TYPE,
                            delete_trip_comp_layers BOOLEAN);

PROCEDURE save_pax_docs(vpax_id     IN pax.pax_id%TYPE,
                        vdocument   IN VARCHAR2,
                        full_insert IN NUMBER DEFAULT 1);

TYPE birks_cursor_ref IS REF CURSOR;

FUNCTION build_birks_str(cur birks_cursor_ref) RETURN VARCHAR2;

END ckin;
/
