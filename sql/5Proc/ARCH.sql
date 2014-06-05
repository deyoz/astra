create or replace PACKAGE arch
AS

TYPE TBagInfo IS RECORD
(
grp_id    arx_pax.grp_id%TYPE,
pax_id		arx_pax.pax_id%TYPE,
bagAmount	NUMBER(5),
bagWeight	NUMBER(6),
rkAmount	NUMBER(5),
rkWeight	NUMBER(6)
);

bagInfo		TBagInfo;

TYPE TIdsTable IS TABLE OF NUMBER(9);
TYPE TRowidsTable IS TABLE OF ROWID;
TYPE TRowidCursor IS REF CURSOR;

FUNCTION get_main_pax_id2(vpart_key       IN arx_pax_grp.part_key%TYPE,
                          vgrp_id         IN arx_pax_grp.grp_id%TYPE,
                          include_refused IN NUMBER DEFAULT 1) RETURN arx_pax.pax_id%TYPE;

FUNCTION get_bag_pool_pax_id(vpart_key     IN arx_pax.part_key%TYPE,
                             vgrp_id       IN arx_pax.grp_id%TYPE,
                             vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                             include_refused IN NUMBER DEFAULT 1) RETURN arx_pax.pax_id%TYPE;

FUNCTION bag_pool_refused(vpart_key     IN arx_bag2.part_key%TYPE,
                          vgrp_id       IN arx_bag2.grp_id%TYPE,
                          vbag_pool_num IN arx_bag2.bag_pool_num%TYPE,
                          vclass        IN arx_pax_grp.class%TYPE,
                          vbag_refuse   IN arx_pax_grp.bag_refuse%TYPE) RETURN NUMBER;

FUNCTION get_birks2(vpart_key     IN arx_pax.part_key%TYPE,
                    vgrp_id       IN arx_pax.grp_id%TYPE,
                    vpax_id 	    IN arx_pax.pax_id%TYPE,
                    vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                    pr_lat        IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_birks2(vpart_key     IN arx_pax.part_key%TYPE,
                    vgrp_id       IN arx_pax.grp_id%TYPE,
                    vpax_id 	    IN arx_pax.pax_id%TYPE,
                    vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                    vlang	        IN lang_types.code%TYPE) RETURN VARCHAR2;

FUNCTION get_excess(vpart_key     IN arx_pax.part_key%TYPE,
                    vgrp_id       IN arx_pax.grp_id%TYPE,
                    vpax_id       IN arx_pax.pax_id%TYPE) RETURN NUMBER;

FUNCTION get_bagInfo2(vpart_key     IN arx_pax.part_key%TYPE,
                      vgrp_id       IN arx_pax.grp_id%TYPE,
                      vpax_id 	    IN arx_pax.pax_id%TYPE,
                      vbag_pool_num IN arx_pax.bag_pool_num%TYPE) RETURN TBagInfo;
FUNCTION get_bagAmount2(vpart_key     IN arx_pax.part_key%TYPE,
                        vgrp_id       IN arx_pax.grp_id%TYPE,
                        vpax_id       IN arx_pax.pax_id%TYPE,
                        vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_bagWeight2(vpart_key     IN arx_pax.part_key%TYPE,
                        vgrp_id       IN arx_pax.grp_id%TYPE,
                        vpax_id       IN arx_pax.pax_id%TYPE,
                        vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_rkAmount2(vpart_key     IN arx_pax.part_key%TYPE,
                       vgrp_id       IN arx_pax.grp_id%TYPE,
                       vpax_id  	   IN arx_pax.pax_id%TYPE,
                       vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION get_rkWeight2(vpart_key     IN arx_pax.part_key%TYPE,
                       vgrp_id       IN arx_pax.grp_id%TYPE,
                       vpax_id  	   IN arx_pax.pax_id%TYPE,
                       vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER;

FUNCTION next_airp(vpart_key     IN arx_points.part_key%TYPE,
                   vfirst_point  IN arx_points.first_point%TYPE,
                   vpoint_num    IN arx_points.point_num%TYPE) RETURN arx_points.airp%TYPE;

/*ÄêïàÇ*/
PROCEDURE move(vmove_id  points.move_id%TYPE,
               vpart_key DATE,
               vdate_range move_arx_ext.date_range%TYPE);
PROCEDURE move(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER);
PROCEDURE tlg_trip(vpoint_id  tlg_trips.point_id%TYPE);
PROCEDURE norms_rates_etc(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER);
PROCEDURE tlgs_files_etc(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER);
PROCEDURE move_typeb_in(vid tlgs_in.id%TYPE);
END arch;
/
