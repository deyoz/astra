create or replace PACKAGE statist
AS

function get_trfer_route(agrp_id pax_grp.grp_id%type) return varchar2;
PROCEDURE get_trfer_stat(vpoint_id IN points.point_id%TYPE);
PROCEDURE get_kiosk_stat(vpoint_id IN points.point_id%TYPE);
PROCEDURE get_full_stat(vpoint_id IN points.point_id%TYPE,
                        vpr_stat  IN trip_sets.pr_stat%TYPE);
END statist;
/
