create or replace PACKAGE sopp
AS
FUNCTION get_birks(vpoint_id 	        IN points.point_id%TYPE,
                   vlang	IN lang_types.code%TYPE) RETURN VARCHAR2;

PROCEDURE set_flight_sets(vpoint_id IN points.point_id%TYPE,
                          use_seances      IN NUMBER,
                          vf IN trip_sets.f%TYPE DEFAULT 0,
                          vc IN trip_sets.f%TYPE DEFAULT 0,
                          vy IN trip_sets.f%TYPE DEFAULT 0 );
END sopp;
/
