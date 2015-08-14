create or replace PACKAGE sopp
AS
FUNCTION get_birks(vpoint_id 	        IN points.point_id%TYPE,
                   vlang	IN lang_types.code%TYPE) RETURN VARCHAR2;

END sopp;
/
