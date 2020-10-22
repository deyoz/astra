create or replace PROCEDURE SP_WB_GET_EQUIP_LISTS(cXML_in IN clob,
                                                      cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
cXML_data clob:='';
BEGIN
    P_LANG:=cXML_in;

    cXML_out:='';

    -----------------------------MATH_COMP_OPERATORS---------------------------
    select XMLAGG(XMLELEMENT("deck_list",
                    xmlattributes(to_char(q."ID") "id",
                                            q."NAME" "NAME"))).getClobVal()
    into cXML_data
    from (select id,
                 NAME
          from WB_REF_WS_DECK
          order by SORT_PRIOR) q;

     if cXML_data is not NULL then
       begin
         cXML_out := cXML_out || cXML_data;
       end;
     end if;
     -----------------------------MATH_COMP_OPERATORS---------------------------

     -----------------------------USE_ITEMS-------------------------------------
     select XMLAGG(XMLELEMENT("gol_locations",
                    xmlattributes(to_char(q."ID") "id",
                                            q."NAME" "name"))).getClobVal()
    into cXML_data
    from (select id,
                 NAME
          from WB_REF_WS_GOL_LOCATIONS
          order by SORT_PRIOR) q;

     if cXML_data is not NULL then
       begin
         cXML_out := cXML_out || cXML_data;
       end;
     end if;
     -----------------------------USE_ITEMS-------------------------------------

    if cXML_out is not NULL then begin
        cXML_out:='<?xml version="1.0" ?><root>'||cXML_out||'</root>';
        end;
    end if;

END SP_WB_GET_EQUIP_LISTS;
/
