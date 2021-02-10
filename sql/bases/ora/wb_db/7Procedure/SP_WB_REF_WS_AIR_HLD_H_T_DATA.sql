create or replace procedure SP_WB_REF_WS_AIR_HLD_H_T_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data clob;
V_R_COUNT number:=0;

V_IS_AC number:=1;
V_IS_WS number:=1;
V_IS_BORT number:=1;
begin
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_DECK
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin

        select XMLAGG(XMLELEMENT("deck_list",
                    xmlattributes(to_char(q."ID") "id",
                                            q."NAME" "NAME"))).getClobVal() into cXML_data
        from (select distinct d.id,
                                d.name,
                                  d.SORT_PRIOR
              from WB_REF_WS_AIR_HLD_DECK h join WB_REF_WS_DECK d
              on h.id_ac=P_ID_AC and
                 h.id_ws=P_ID_WS and
                 h.id_bort=P_ID_BORT and
                 h.deck_id=d.id
              order by d.sort_prior) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_HLD_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.ID as "id",
                                    q.deck_id as "deck_id",
                                      q.deck_name as "deck_name",
                                        q.hold_name as "hold_name",
                                          q.max_weight as "max_weight",
                                            q.max_volume as "max_volume",
                                              q.la_centroid as "la_centroid",
                                                q.la_from as "la_from",
                                                  q.la_to as "la_to",
                                                    q.ba_centroid as "ba_centroid",
                                                      q.ba_fwd as "ba_fwd",
                                                        q.ba_aft as "ba_aft",
                                                          q.index_per_wt_unit as "index_per_wt_unit",
                                                            q.U_NAME as "U_NAME",
                                                              q.U_IP as "U_IP",
                                                                q.U_HOST_NAME as "U_HOST_NAME",
                                                                  q.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       to_char(h.deck_id) deck_id,
                         d.NAME deck_name,
                           h.hold_name,
                             nvl(to_char(h.max_weight), 'NULL') max_weight,
                               nvl(to_char(h.max_volume), 'NULL') max_volume,
                                 nvl(to_char(h.la_centroid), 'NULL') la_centroid,
                                   nvl(to_char(h.la_from), 'NULL') la_from,
                                     nvl(to_char(h.la_to), 'NULL') la_to,
                                       nvl(to_char(h.ba_centroid), 'NULL') ba_centroid,
                                         nvl(to_char(h.ba_fwd), 'NULL') ba_fwd,
                                           nvl(to_char(h.ba_aft), 'NULL') ba_aft,
                                             nvl(to_char(h.index_per_wt_unit), 'NULL') index_per_wt_unit,
                                               h.U_NAME,
                                                 h.U_IP,
                                                   h.U_HOST_NAME,
                                                     to_char(h.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_REF_WS_AIR_HLD_HLD_T h join WB_REF_WS_DECK d
              on h.id_ac=P_ID_AC and
                 h.id_ws=P_ID_WS and
                 h.id_bort=P_ID_BORT  and
                 h.deck_id=d.id
              order by d.sort_prior,
                       h.hold_name) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_HLD_H_T_DATA;
/