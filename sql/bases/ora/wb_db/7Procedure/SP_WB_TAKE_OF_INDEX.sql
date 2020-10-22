create or replace PROCEDURE SP_WB_TAKE_OF_INDEX(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
P_id_ac number:=-1;
P_id_ws number:=-1;
P_fuel number:=-1;
cXML_data clob:='';

BEGIN
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_id_ac[1]') into P_id_ac from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_id_ws[1]') into P_id_ws from dual;
  select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_fuel[1]') into P_fuel from dual;
  cXML_out := '<?xml version="1.0" ?>';

  select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.y) "y"))).getClobVal() into cXML_data
  from
  (
    with x0 as
    (
      select weight,index_unit, row_number() over (order by weight desc) aa
      from WB_REF_WS_AIR_CFV_TBL
      WHERE (ID_AC=P_id_ac) and (ID_WS=P_id_ws) AND (weight<=P_fuel)
    ),
    x1 as
    (
      select weight,index_unit, row_number() over (order by weight) aa
      from WB_REF_WS_AIR_CFV_TBL
      WHERE (ID_AC=P_id_ac) and (ID_WS=P_id_ws) AND (weight>P_fuel)
    )
    select (x0.index_unit + (x1.index_unit - x0.index_unit) / (x1.weight - x0.weight) * (P_fuel - x0.weight)) as y,
    x0.index_unit , (x1.index_unit - x0.index_unit) , (x1.weight - x0.weight) , (P_fuel - x0.weight), x0.aa,x1.aa
    from x1,x0
    where x1.aa=1 and x0.aa=1
  ) e;

  if cXML_data is not null then
  begin
     cXML_out := cXML_out || '<root name="get_take_of_index" result="ok">' || cXML_data || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" ?><root name="get_take_of_index" result="ok"></root>';
  end;
  end if;

END SP_WB_TAKE_OF_INDEX;
/
