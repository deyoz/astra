create or replace procedure SP_WB_REF_WS_AIR_DOORS_T_DATA
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
    from WB_REF_WS_AIR_HLD_HLD_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("hold_list",
                    xmlattributes(to_char(q.ID) as "id",
                                            q.hold_name as "hold_name"))).getClobVal() into cXML_data
        from (select distinct h.id,
                                h.hold_name
              from WB_REF_WS_AIR_HLD_HLD_T h
              where h.id_ac=P_ID_AC and
                    h.id_ws=P_ID_WS and
                    h.id_bort=P_ID_BORT
              order by h.hold_name) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_DOORS_POSITIONS;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("doors_pos_list",
                    xmlattributes(to_char(q.ID) as "id",
                                            q.name as "name"))).getClobVal() into cXML_data
        from (select id,
                       name
              from WB_REF_WS_DOORS_POSITIONS
              order by SORT_PRIOR) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOORS_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.ID as "id",
                                    q.hold_id as "hold_id",
                                      q.hold_name as "hold_name",
                                        q.dp_name as "dp_name",
                                          q.dp_id as "dp_id",
                                            q.door_name as "d_name",
                                              q.ba_fwd as "ba_fwd",
                                                q.ba_aft as "ba_aft",
                                                  q.HEIGHT as "height",
                                                    q.WIDTH as "width",
                                                      q.U_NAME as "U_NAME",
                                                        q.U_IP as "U_IP",
                                                          q.U_HOST_NAME as "U_HOST_NAME",
                                                            q.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(h.id) id,
                       to_char(h.hold_id) hold_id,
                         d.hold_NAME,
                           dp.name dp_name,
                             to_char(dp.id) dp_id,
                               h.door_name,
                                 nvl(to_char(h.ba_fwd), 'NULL') ba_fwd,
                                   nvl(to_char(h.ba_aft), 'NULL') ba_aft,
                                     nvl(to_char(h.HEIGHT), 'NULL') HEIGHT,
                                       nvl(to_char(h.WIDTH), 'NULL') WIDTH,
                                         h.U_NAME,
                                           h.U_IP,
                                             h.U_HOST_NAME,
                                               to_char(h.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_REF_WS_AIR_DOORS_T h join WB_REF_WS_AIR_HLD_HLD_T d
              on h.id_ac=P_ID_AC and
                 h.id_ws=P_ID_WS and
                 h.id_bort=P_ID_BORT  and
                 h.hold_id=d.id join WB_REF_WS_DOORS_POSITIONS dp
                 on dp.id=h.D_POS_ID
              order by h.DOOR_NAME,
                       d.hold_name) q;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_DOORS_T_DATA;
/
