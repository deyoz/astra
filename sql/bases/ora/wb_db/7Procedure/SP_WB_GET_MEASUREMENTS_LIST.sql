create or replace PROCEDURE SP_WB_GET_MEASUREMENTS_LIST(cXML_in IN clob,
                                                         cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
cXML_data clob:='';
BEGIN
    P_LANG := cXML_in;

    cXML_out := '';

    ---------------------------DENSITY------------------------------------------
    select XMLAGG(XMLELEMENT("density"
        , xmlattributes(to_char(qq."ID") "id", qq."MEAS_NAME" "MEAS_NAME"))).getClobVal()
    into cXML_data
    from (select id
            , case when P_LANG = 'RUS'
              then NAME_RUS_SMALL || '...[' || NAME_RUS_FULL || ']'
              else NAME_ENG_SMALL || '...[' || NAME_ENG_FULL || ']'
              end "MEAS_NAME"
          from WB_REF_MEASUREMENT_DENSITY
          order by "MEAS_NAME") qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------DENSITY------------------------------------------

    ---------------------------LENGTH-------------------------------------------
    select XMLAGG(XMLELEMENT("length"
        , xmlattributes(to_char(qq."ID") "id", qq.MEAS_NAME "MEAS_NAME"))).getClobVal()
    into cXML_data
    from (select id
            , case when P_LANG = 'RUS'
              then NAME_RUS_SMALL || '...[' || NAME_RUS_FULL || ']'
              else NAME_ENG_SMALL || '...[' || NAME_ENG_FULL || ']'
              end "MEAS_NAME"
          from WB_REF_MEASUREMENT_LENGTH
          order by "MEAS_NAME") qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------LENGTH-------------------------------------------

    ---------------------------VOLUME-------------------------------------------
    select XMLAGG(XMLELEMENT("volume"
        , xmlattributes(to_char(qq."ID") "id", qq.MEAS_NAME "MEAS_NAME"))).getClobVal()
    into cXML_data
    from (select id
            , case when P_LANG = 'RUS'
              then NAME_RUS_SMALL || '...[' || NAME_RUS_FULL || ']'
              else NAME_ENG_SMALL || '...[' || NAME_ENG_FULL || ']'
              end "MEAS_NAME"
          from WB_REF_MEASUREMENT_VOLUME
          order by "MEAS_NAME") qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------VOLUME-------------------------------------------

    ---------------------------WEIGHT-------------------------------------------
    select XMLAGG(XMLELEMENT("weight"
        , xmlattributes(to_char(qq."ID") "id", qq.MEAS_NAME "MEAS_NAME"))).getClobVal()
    into cXML_data
    from (select id
          , case when P_LANG = 'RUS'
              then NAME_RUS_SMALL || '...[' || NAME_RUS_FULL || ']'
              else NAME_ENG_SMALL || '...[' || NAME_ENG_FULL || ']'
              end "MEAS_NAME"
          from WB_REF_MEASUREMENT_WEIGHT
          order by "MEAS_NAME") qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------WEIGHT-------------------------------------------

    ---------------------------MOMENTS------------------------------------------
    select XMLAGG(XMLELEMENT("moments"
        , xmlattributes(to_char(qq."ID") "id", qq.MEAS_NAME "MEAS_NAME"))).getClobVal()
    into cXML_data
    from (select id
          , case when P_LANG = 'RUS'
              then NAME_RUS_SMALL || '...[' || NAME_RUS_FULL || ']'
              else NAME_ENG_SMALL || '...[' || NAME_ENG_FULL || ']'
              end "MEAS_NAME"
          from WB_REF_MEASUREMENT_MOMENTS
          order by "MEAS_NAME") qq;

    if cXML_data is not NULL then begin
        cXML_out := cXML_out || cXML_data;
        end;
    end if;
    ---------------------------MOMENTS------------------------------------------

    if cXML_out is not NULL then begin
        cXML_out := '<?xml version="1.0" ?><root>' || cXML_out || '</root>';
        end;
    end if;

END SP_WB_GET_MEASUREMENTS_LIST;
/
