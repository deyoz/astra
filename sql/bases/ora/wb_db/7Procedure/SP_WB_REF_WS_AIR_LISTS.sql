create or replace PROCEDURE SP_WB_REF_WS_AIR_LISTS(
	cXML_in IN clob,
	cXML_out OUT CLOB)
AS
cXML_data clob := '';
R_COUNT number := 0;
BEGIN
    cXML_out := '';

    ---------------------------WB_REF_WS_TRANSP_KATEG---------------------------
    select XMLAGG(XMLELEMENT("TRANSP_KATEG"
        , xmlattributes(to_char(qq."ID") "id", qq."NAME" "NAME"))).getClobVal()
    into cXML_data
    from (select "ID", "NAME"
          from WB_REF_WS_TRANSP_KATEG
          order by name) qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------WB_REF_WS_TRANSP_KATEG---------------------------

    ---------------------------WB_REF_WS_TYPE_OF_LOADING------------------------
    select XMLAGG(XMLELEMENT("TYPE_OF_LOADING"
        , xmlattributes(to_char(qq."ID") "id", qq."NAME" "NAME"))).getClobVal()
    into cXML_data
    from (select "ID", "NAME"
          from WB_REF_WS_TYPE_OF_LOADING
          order by name) qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------WB_REF_WS_TYPE_OF_LOADING------------------------

    if cXML_out is not NULL then begin
        cXML_out := '<?xml version="1.0" ?><root>' || cXML_out || '</root>';
        end;
    end if;

END SP_WB_REF_WS_AIR_LISTS;
/
