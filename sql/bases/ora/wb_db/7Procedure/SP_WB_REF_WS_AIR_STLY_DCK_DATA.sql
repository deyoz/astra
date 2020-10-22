create or replace procedure SP_WB_REF_WS_AIR_STLY_DCK_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

  select count(d.id) into V_R_COUNT
  from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_S_L_PLAN p
  on a.id=P_ADV_ID and
     p.adv_id=a.id join WB_REF_WS_AIR_CABIN_ADV ca
     on ca.idn=a.cabin_id join WB_REF_WS_AIR_CABIN_CD cd
        on cd.adv_id=ca.id and
           cd.section=p.cabin_section join WB_REF_WS_DECK d
           on d.id=cd.deck_id;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("deck_data",
                  xmlattributes(q.id "id",
                                  q.name as "name"))).getClobVal() into cXML_data
      from (select distinct d.id,
                              d.name,
                                d.sort_prior
            from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_S_L_PLAN p
            on a.id=P_ADV_ID and
               p.adv_id=a.id join WB_REF_WS_AIR_CABIN_ADV ca
               on ca.idn=a.cabin_id join WB_REF_WS_AIR_CABIN_CD cd
                  on cd.adv_id=ca.id and
                     cd.section=p.cabin_section join WB_REF_WS_DECK d
                     on d.id=cd.deck_id
            order by d.sort_prior) q
      order by q.sort_prior;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_STLY_DCK_DATA;
/
