create or replace procedure SP_WB_REF_WS_AIR_STLY_ADV_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_SEAT_LAY_ADV
  where id=P_ADV_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("adv_data",
                  xmlattributes(q.table_name "table_name",
                                  q.CH_INDEX_UNIT as "CH_INDEX_UNIT",
                                    q.CH_BALANCE_ARM as "CH_BALANCE_ARM",
                                      q.CABIN_ID as "CABIN_ID",
                                        q.NUM_OF_SEATS as "NUM_OF_SEATS",
                                          q.U_NAME as "U_NAME",
                                            q.U_IP as "U_IP",
                                              q.U_HOST_NAME as "U_HOST_NAME",
                                                q.DATE_WRITE as "DATE_WRITE"))).getClobVal() into cXML_data
      from (select a.table_name,
              to_char(a.CH_INDEX_UNIT) CH_INDEX_UNIT,
                to_char(a.CH_BALANCE_ARM) CH_BALANCE_ARM,
                  to_char(a.cabin_id) CABIN_ID,
                    nvl(to_char(a.NUM_OF_SEATS), 'NULL') NUM_OF_SEATS,
                      a.U_NAME,
                        a.U_IP,
                          a.U_HOST_NAME,
                            to_char(a.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
            from WB_REF_WS_AIR_SEAT_LAY_ADV a
            where id=P_ADV_ID) q;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(c.id) into V_R_COUNT
  from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_IDN c
  on a.id=P_ADV_ID and
     c.id_ac=a.id_ac and
     c.id_ws=a.id_ws and
     c.id_bort=a.id_bort;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("cabin_id_list",
                  xmlattributes(q.id as "id",
                                  q.table_name "table_name"))).getClobVal() into cXML_data
      from (select to_char(c.id) id,
                     c.table_name
            from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_IDN c
            on a.id=P_ADV_ID and
               c.id_ac=a.id_ac and
               c.id_ws=a.id_ws and
               c.id_bort=a.id_bort
            order by c.table_name) q
      order by q.table_name;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_SEATS_NAMES;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("SEATS_id_list",
                  xmlattributes(q.id as "id",
                                  q.name "name"))).getClobVal() into cXML_data
      from (select to_char(c.id) id,
                     c.name
            from WB_REF_WS_SEATS_NAMES c
            order by c.name) q
      order by q.name;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_A_U_S
  where adv_id=p_ADV_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("SEATS_USING_LIST",
                  xmlattributes(q.seat_id as "seat_id"))).getClobVal() into cXML_data
      from (select to_char(c.seat_id) seat_id
            from WB_REF_WS_AIR_S_L_A_U_S c
            where adv_id=p_ADV_ID) q;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_STLY_ADV_DATA;
/
