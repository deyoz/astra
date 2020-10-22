create or replace procedure SP_WB_REF_WS_AIR_S_L_PLAN_DATA
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
  from WB_REF_WS_AIR_S_L_A_U_S
  where adv_id=p_ADV_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("SEATS_USING_LIST",
                  xmlattributes(q.seat_id as "seat_id",
                                  q.name as "name"))).getClobVal() into cXML_data
      from (select to_char(c.seat_id) seat_id,
                     s.name
            from WB_REF_WS_AIR_S_L_A_U_S c join WB_REF_WS_SEATS_NAMES s
            on c.adv_id=p_ADV_ID and
               c.seat_id=s.id
            order by s.name) q
      order by q.name;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(cd.id) into V_R_COUNT
  from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
  on a.id=p_ADV_ID and
     a.cabin_id=cd.idn;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("CABIN_SECTION_LIST",
                  xmlattributes(q.section as "section",
                                  to_char(rows_from) as "rows_from",
                                    to_char(rows_to) as "rows_to"))).getClobVal() into cXML_data
      from (select distinct cd.section,
                              cd.rows_from,
                                cd.rows_to
            from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
            on a.id=p_ADV_ID and
               a.cabin_id=cd.idn
            order by cd.section,
                     cd.rows_from,
                     cd.rows_to) q
      order by q.section,
               q.rows_from,
               q.rows_to;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  select count(p.id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_PLAN p join WB_REF_WS_AIR_S_L_P_S s
  on p.adv_id=p_ADV_ID and
     p.id=s.PLAN_ID join WB_REF_WS_SEATS_NAMES sn
     on sn.id=s.SEAT_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("TABLE_DATA",
                  xmlattributes(to_char(q.plan_id) as "PLAN_ID",
                                  q.CABIN_SECTION as "CABIN_SECTION",
                                    to_char(q.ROW_NUMBER) as "ROW_NUMBER",
                                      to_char(q.MAX_WEIGHT) as "MAX_WEIGHT",
                                        to_char(q.MAX_SEATS) as "MAX_SEATS",
                                          to_char(q.BALANCE_ARM) as "BALANCE_ARM",
                                            to_char(q.INDEX_PER_WT_UNIT) as "INDEX_PER_WT_UNIT",
                                              to_char(q.SEAT_PLAN_ID) as "SEAT_PLAN_ID",
                                                to_char(q.IS_SEAT) as "IS_SEAT",
                                                  to_char(q.SEAT_ID) as "SEAT_ID",
                                                    q.seat_name as "SEAT_NAME",
                                                      q.P_U_NAME as "P_U_NAME",
                                                        q.P_U_IP as "P_U_IP",
                                                          q.P_U_HOST_NAME as "P_U_HOST_NAME",
                                                            q.P_DATE_WRITE as "P_DATE_WRITE",
                                                              q.S_U_NAME as "S_U_NAME",
                                                                q.S_U_IP as "S_U_IP",
                                                                  q.S_U_HOST_NAME as "S_U_HOST_NAME",
                                                                    q.S_DATE_WRITE as "S_DATE_WRITE",
                                                                      q.param_str as "PARAM_STR",
                                                                        q.s_max_weight as "S_MAX_WEIGHT"))).getClobVal() into cXML_data
      from (select p.id plan_id,
                     p.CABIN_SECTION,
                       p.ROW_NUMBER,
                         p.MAX_WEIGHT,
	                         p.MAX_SEATS,
	                           p.BALANCE_ARM,
	                             p.INDEX_PER_WT_UNIT,
                                 s.id seat_plan_id,
                                   s.is_seat,
                                     s.SEAT_ID,
                                       sn.name seat_name,
                                         p.U_NAME P_U_NAME,
                                           p.U_IP P_U_IP,
                                             p.U_HOST_NAME p_U_HOST_NAME,
                                               to_char(p.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') P_DATE_WRITE,
                                                 s.U_name S_U_NAME,
                                                   s.U_IP S_U_IP,
                                                     s.U_HOST_NAME S_U_HOST_NAME,
                                                       to_char(s.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') S_DATE_WRITE,
                                                         WB_WS_AIR_SEAT_PARAM_STR(s.id) PARAM_STR,
                                                           to_char(s.max_weight) s_max_weight
            from WB_REF_WS_AIR_S_L_PLAN p join WB_REF_WS_AIR_S_L_P_S s
            on p.adv_id=p_ADV_ID and
               p.id=s.PLAN_ID join WB_REF_WS_SEATS_NAMES sn
               on sn.id=s.SEAT_ID
            order by p.CABIN_SECTION,
                     p.ROW_NUMBER,
                     p.id,
                     sn.name,
                     sn.id,
                     s.id) q
      order by q.CABIN_SECTION,
               q.ROW_NUMBER,
               q.plan_id,
               q.seat_name,
               q.SEAT_ID,
               q.seat_plan_id;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_S_L_PLAN_DATA;
/
