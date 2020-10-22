create or replace PROCEDURE SP_WB_GET_CH_USE_LIST(cXML_in IN clob,
                                                    cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
cXML_data clob:='';
BEGIN
    P_LANG:=cXML_in;

    cXML_out:='';
    /*
    -----------------------------MATH_COMP_OPERATORS---------------------------
    select XMLAGG(XMLELEMENT("math_comp_operators",
                    xmlattributes(to_char(q."ID") "id",
                                            q."SYMBOL" "symbol"))).getClobVal()
    into cXML_data
    from (select id,
                 SYMBOL
          from WB_REF_MATH_COMP_OPERATORS
          order by SORT_PRIOR) q;

     if cXML_data is not NULL then
       begin
         cXML_out := cXML_out || cXML_data;
       end;
     end if;
     -----------------------------MATH_COMP_OPERATORS---------------------------
     */
     -----------------------------USE_ITEMS-------------------------------------
     select XMLAGG(XMLELEMENT("ch_use_items",
                    xmlattributes(to_char(q."ID") "id",
                                            q."ITEM_NAME" "item_name"))).getClobVal()
    into cXML_data
    from (select id,
                 ITEM_NAME
          from WB_REF_WS_AIR_GR_CH_USE_ITEMS
          order by SORT_PRIOR) q;

     if cXML_data is not NULL then
       begin
         cXML_out := cXML_out || cXML_data;
       end;
     end if;
     -----------------------------USE_ITEMS-------------------------------------

    if cXML_out is not NULL then begin
        cXML_out := '<?xml version="1.0" ?><root>' || cXML_out || '</root>';
        end;
    end if;

END  SP_WB_GET_CH_USE_LIST;
/
