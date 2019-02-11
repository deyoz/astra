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

FUNCTION next_airp(vfirst_point 	IN points.first_point%TYPE,
                   vpoint_num        	IN points.point_num%TYPE) RETURN points.airp%TYPE;

FUNCTION tranzitable(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE;

FUNCTION get_pr_tranzit(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE;

FUNCTION get_pr_tranz_reg(vpoint_id	IN points.point_id%TYPE) RETURN trip_sets.pr_tranz_reg%TYPE;

FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    pr_lat        IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    vlang	        IN lang_types.code%TYPE) RETURN VARCHAR2;

FUNCTION need_for_payment(vgrp_id        IN pax_grp.grp_id%TYPE,
                          vclass         IN pax_grp.class%TYPE,
                          vbag_refuse    IN pax_grp.bag_refuse%TYPE,
                          vpiece_concept IN pax_grp.piece_concept%TYPE,
                          vexcess_wt     IN pax_grp.excess_wt%TYPE,
                          vexcess_pc     IN pax_grp.excess_pc%TYPE,
                          vpax_id        IN pax.pax_id%TYPE) RETURN NUMBER;

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

FUNCTION get_excess_wt(vgrp_id         IN pax.grp_id%TYPE,
                       vpax_id         IN pax.pax_id%TYPE,
                       vexcess_wt      IN pax_grp.excess_wt%TYPE DEFAULT NULL,
                       vbag_refuse     IN pax_grp.bag_refuse%TYPE DEFAULT NULL) RETURN NUMBER;

FUNCTION get_excess_pc(vgrp_id         IN pax.grp_id%TYPE,
                       vpax_id         IN pax.pax_id%TYPE,
                       include_all_svc IN NUMBER DEFAULT 0) RETURN NUMBER;

FUNCTION get_crs_priority(vcrs           IN crs_set.crs%TYPE,
                          vairline       IN crs_set.airline%TYPE,
                          vflt_no        IN crs_set.flt_no%TYPE,
                          vairp_dep      IN crs_set.airp_dep%TYPE) RETURN NUMBER;
PRAGMA RESTRICT_REFERENCES (get_crs_priority, WNDS, WNPS, RNPS);

FUNCTION get_crs_ok(vpoint_id	IN points.point_id%TYPE) RETURN NUMBER;

FUNCTION delete_grp_trfer(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER;
FUNCTION delete_grp_tckin_segs(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER;
PROCEDURE check_grp(vgrp_id     pax_grp.grp_id%TYPE);

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

TYPE birks_cursor_ref IS REF CURSOR;

FUNCTION build_birks_str(cur birks_cursor_ref) RETURN VARCHAR2;

PROCEDURE set_additional_list_id(vgrp_id IN pax.grp_id%TYPE);

END ckin;
/
