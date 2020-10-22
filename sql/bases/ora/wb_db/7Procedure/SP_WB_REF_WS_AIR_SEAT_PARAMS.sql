create or replace procedure SP_WB_REF_WS_AIR_SEAT_PARAMS
(cXML_in in clob,
   cXML_out out clob)
as
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select count(id) into V_R_COUNT
  from WB_REF_WS_SEATS_PARAMS;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("SEATS_PARAMS_LIST",
                  xmlattributes(q.id as "id",
                                  q.name as "name",
                                    q.remark as "remark"))).getClobVal() into cXML_data
      from (select to_char(id) id,
                     name,
                       remark,
                         sort_prior
            from WB_REF_WS_SEATS_PARAMS
            order by sort_prior) q
      order by q.sort_prior;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SEAT_PARAMS;
/
