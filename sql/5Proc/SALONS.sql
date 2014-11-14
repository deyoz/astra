create or replace PACKAGE salons
AS
SUBTYPE TCompLayerType IS comp_layer_types.code%TYPE;
cltBlockCent      CONSTANT TCompLayerType := 'BLOCK_CENT';
cltTranzit        CONSTANT TCompLayerType := 'TRANZIT';
cltCheckin        CONSTANT TCompLayerType := 'CHECKIN';
cltTCheckin       CONSTANT TCompLayerType := 'TCHECKIN';
cltGoShow         CONSTANT TCompLayerType := 'GOSHOW';
cltBlockTrzt      CONSTANT TCompLayerType := 'BLOCK_TRZT';
cltSOMTrzt        CONSTANT TCompLayerType := 'SOM_TRZT';
cltPRLTrzt        CONSTANT TCompLayerType := 'PRL_TRZT';
cltProtBeforePay  CONSTANT TCompLayerType := 'PROT_BPAY';
cltProtAfterPay   CONSTANT TCompLayerType := 'PROT_APAY';
cltPNLBeforePay   CONSTANT TCompLayerType := 'PNL_BPAY';
cltPNLAfterPay    CONSTANT TCompLayerType := 'PNL_APAY';
cltProtTrzt       CONSTANT TCompLayerType := 'PROT_TRZT';
cltPNLCkin        CONSTANT TCompLayerType := 'PNL_CKIN';
cltProtCkin       CONSTANT TCompLayerType := 'PROT_CKIN';
cltProtect        CONSTANT TCompLayerType := 'PROTECT';
cltUncomfort      CONSTANT TCompLayerType := 'UNCOMFORT';
cltSmoke          CONSTANT TCompLayerType := 'SMOKE';

TYPE TCRSSeatInfo IS RECORD
(
point_id        NUMBER(9),
pr_lat_seat     trip_sets.pr_lat_seat%TYPE,
pr_paid_ckin    trip_paid_ckin.point_id%TYPE
);

TYPE TTableCompLayerType IS TABLE OF comp_layer_types.code%TYPE INDEX BY BINARY_INTEGER;

TYPE TSeatInfo IS RECORD
(
point_id        NUMBER(9),
pr_lat_seat     trip_sets.pr_lat_seat%TYPE,
pr_tranzit      points.pr_tranzit%TYPE,
pr_tranz_reg	trip_sets.pr_tranz_reg%TYPE,
pr_block_trzt	trip_sets.pr_block_trzt%TYPE,
pr_tranzit_salon NUMBER(1),
pr_free_seating trip_sets.pr_free_seating%TYPE,
tlg_point_dep	trip_comp_layers.point_dep%TYPE,
layers TTableCompLayerType
);

crsSeatInfo	TCRSSeatInfo;
seatInfo        TSeatInfo;

FUNCTION normalize_xname(vxname comp_elems.xname%TYPE) RETURN comp_elems.xname%TYPE;
FUNCTION normalize_yname(vyname comp_elems.yname%TYPE) RETURN comp_elems.yname%TYPE;
FUNCTION denormalize_xname(vxname comp_elems.xname%TYPE,
                           pr_lat NUMBER) RETURN comp_elems.xname%TYPE;
FUNCTION denormalize_yname(vyname comp_elems.yname%TYPE,
                           add_ch VARCHAR2) RETURN comp_elems.yname%TYPE;

FUNCTION get_crs_seat_no(vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         fmt	 IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_crs_seat_no(vpax_id     IN crs_pax.pax_id%TYPE,
                         vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         fmt	 IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_crs_seat_no(vpax_id     IN crs_pax.pax_id%TYPE,
                         vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         vlayer_type OUT tlg_comp_layers.layer_type%TYPE,
                         fmt	     IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_seatno(vpax_id     IN pax.pax_id%TYPE,
                    vseats      IN pax.seats%TYPE,
                    vpoint_id   IN trip_sets.point_id%TYPE,
                    fmt	 IN VARCHAR2,
                    row	 IN NUMBER DEFAULT 1,
                    only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2;
FUNCTION get_seat_no(vpax_id     IN pax.pax_id%TYPE,
                     vseats      IN pax.seats%TYPE,
                     vgrp_status IN pax_grp.status%TYPE,
                     vpoint_id   IN trip_sets.point_id%TYPE,
                     fmt	 IN VARCHAR2,
                     row	 IN NUMBER DEFAULT 1,
                     only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2;

FUNCTION is_waitlist(vpax_id     IN pax.pax_id%TYPE,
                     vseats      IN pax.seats%TYPE,
                     vgrp_status IN pax_grp.status%TYPE,
                     vpoint_id   IN trip_sets.point_id%TYPE,
                     row	 IN NUMBER DEFAULT 1) RETURN NUMBER;
FUNCTION normalize_bort(str points.bort%TYPE) RETURN points.bort%TYPE;
FUNCTION check_bort(str VARCHAR2) RETURN points.bort%TYPE;

END salons;
/
