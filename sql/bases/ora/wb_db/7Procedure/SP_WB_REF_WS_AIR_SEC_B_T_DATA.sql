create or replace procedure SP_WB_REF_WS_AIR_SEC_B_T_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

cXML_data clob;
V_R_COUNT number:=0;
begin


    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_HLD_CMP_T
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("cmp_list",
                    xmlattributes(q.cmp_name as "cmp_name"))).getClobVal() into cXML_data
        from (select distinct h.cmp_name
              from WB_REF_WS_AIR_HLD_CMP_T h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.cmp_name) q
        order by q.cmp_name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_SEC_BAY_TYPE;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("SEC_BAY_TYPE_LIST",
                    xmlattributes(to_char(q.ID) as "id",
                                            q.name as "name"))).getClobVal() into cXML_data
        from (select distinct h.id,
                                h.name
              from WB_REF_SEC_BAY_TYPE h
              order by h.name) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.ID as "id",
                                    q.SEC_BAY_TYPE_ID as "SEC_BAY_TYPE_ID",
                                      q.SEC_BAY_TYPE_NAME as "SEC_BAY_TYPE_NAME",
                                        q.cmp_name as "cmp_name",
                                          q.SEC_BAY_NAME as "SEC_BAY_NAME",
                                            q.ULD_IATA_TRING as "ULD_IATA_STRING",
                                              q.max_weight as "max_weight",
                                                q.max_volume as "max_volume",
                                                  q.la_centroid as "la_centroid",
                                                    q.la_from as "la_from",
                                                      q.la_to as "la_to",
                                                        q.ba_centroid as "ba_centroid",
                                                          q.ba_fwd as "ba_fwd",
                                                            q.ba_aft as "ba_aft",
                                                              q.index_per_wt_unit as "index_per_wt_unit",
                                                                q.DOOR_POSITION as "DOOR_POSITION",
                                                                  q.is_uld_iata as "IS_ULD_IATA",
                                                                    q.color as "COLOR",
                                                                      q.U_NAME as "U_NAME",
                                                                        q.U_IP as "U_IP",
                                                                          q.U_HOST_NAME as "U_HOST_NAME",
                                                                            q.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       to_char(h.SEC_BAY_TYPE_ID) SEC_BAY_TYPE_ID,
                         d.name SEC_BAY_TYPE_NAME,
                           h.CMP_NAME,
                             h.SEC_BAY_NAME,
                               WB_WS_AIR_SEC_BAY_T_ULD_STR(h.id) ULD_IATA_TRING,
                                 nvl(to_char(h.max_weight), 'NULL') max_weight,
                                   nvl(to_char(h.max_volume), 'NULL') max_volume,
                                     nvl(to_char(h.la_centroid), 'NULL') la_centroid,
                                       nvl(to_char(h.la_from), 'NULL') la_from,
                                         nvl(to_char(h.la_to), 'NULL') la_to,
                                           nvl(to_char(h.ba_centroid), 'NULL') ba_centroid,
                                             nvl(to_char(h.ba_fwd), 'NULL') ba_fwd,
                                               nvl(to_char(h.ba_aft), 'NULL') ba_aft,
                                                 nvl(to_char(h.index_per_wt_unit), 'NULL') index_per_wt_unit,
                                                   to_char(h.DOOR_POSITION) DOOR_POSITION,
                                                     to_char(d.IS_ULD_IATA) IS_ULD_IATA,
                                                       to_char(nvl(h.color, 0)) COLOR,
                                                         h.U_NAME,
                                                           h.U_IP,
                                                             h.U_HOST_NAME,
                                                               to_char(h.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_REF_WS_AIR_SEC_BAY_T h join WB_REF_SEC_BAY_TYPE d
              on h.id_ac=P_ID_AC and
                 h.id_ws=P_ID_WS and
                 h.id_bort=P_ID_BORT  and
                 h.SEC_BAY_TYPE_ID=d.id
              order by h.SEC_BAY_NAME,
                       h.CMP_NAME) q
      order by q.SEC_BAY_NAME,
               q.CMP_NAME;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SEC_B_T_DATA;
/
