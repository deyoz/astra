create or replace procedure SP_WB_SHED_FOR_LEGS_LIST
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_REIS number:=-1;

V_R_COUNT number:=0;
cXML_data CLOB:='';
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_REIS[1]')) into P_ID_REIS from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_SHED_MVL_TYPE;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("mvl_type_list", xmlattributes(to_char(e.id) "id",
                                                         e.TYPE_NAME "TYPE_NAME"))).getClobVal() into cXML_data
        from (select id,
                       TYPE_NAME
              from WB_SHED_MVL_TYPE
              order by TYPE_NAME) e
        order by e.TYPE_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_WS_TYPES
    where id_ac=P_ID_AC;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("ws_type_list", xmlattributes(to_char(e.id) "id",
                                                                 e.ws_name "WS_NAME"))).getClobVal() into cXML_data
        from (select ws.id,
                       case when P_LANG='RUS' then ws.NAME_RUS_SMALL
                            else ws.NAME_ENG_SMALL
                       end ws_name
              from WB_REF_AIRCO_WS_TYPES ac join WB_REF_WS_TYPES ws
              on ac.id_ac=P_ID_AC and
                 ac.id_ws=ws.id
              order by case when P_LANG='RUS' then ws.NAME_RUS_SMALL
                            else ws.NAME_ENG_SMALL
                       end   ) e
        order by e.ws_name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_AIRPORTS;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("port_list", xmlattributes(to_char(e.id) "id",
                                                                  e.PORT_NAME "PORT_NAME"))).getClobVal() into cXML_data
        from (select id,
                       case when P_LANG='RUS' then NAME_RUS_SMALL else NAME_ENG_SMALL end PORT_NAME
              from WB_REF_AIRPORTS
              order by case when P_LANG='RUS' then NAME_RUS_SMALL else NAME_ENG_SMALL end) e
        order by e.PORT_NAME;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    if P_ID_REIS>-1 then
      begin
        select count(id) into V_R_COUNT
        from WB_shed
        where id=P_ID_REIS;

        if V_R_COUNT>0 then
          begin
            select XMLAGG(XMLELEMENT("reis_data",
                    xmlattributes(q.ID as "id",
                                    q.nr as "NR",
                                      q.mvl_type as "MVL_TYPE",
                                        q.ws_name as "WS_NAME",
                                          q.bort_num as "BORT_NUM",
                                            q.ap_1 as "AP_1",
                                              q.ap_2 as "AP_2",
                                                to_char(q.ID_AP_1) as "ID_AP_1",
                                                  to_char(q.ID_AP_2) as "ID_AP_2",
                                                    q.TERM_1 as "TERM_1",
                                                      q.TERM_2 as "TERM_2",

                                                        q.S_DTU_1_D as "S_DTU_1_D",
                                                          q.S_DTU_1_M as "S_DTU_1_M",
                                                            q.S_DTU_1_Y as "S_DTU_1_Y",
                                                              q.S_DTU_1_H as "S_DTU_1_H",
                                                                q.S_DTU_1_MIN as "S_DTU_1_MIN",

                                                        q.S_DTL_1_D as "S_DTL_1_D",
                                                          q.S_DTL_1_M as "S_DTL_1_M",
                                                            q.S_DTL_1_Y as "S_DTL_1_Y",
                                                              q.S_DTL_1_H as "S_DTL_1_H",
                                                                q.S_DTL_1_MIN as "S_DTL_1_MIN",

                                                        q.S_DTU_2_D as "S_DTU_2_D",
                                                          q.S_DTU_2_M as "S_DTU_2_M",
                                                            q.S_DTU_2_Y as "S_DTU_2_Y",
                                                              q.S_DTU_2_H as "S_DTU_2_H",
                                                                q.S_DTU_2_MIN as "S_DTU_2_MIN",

                                                        q.S_DTL_2_D as "S_DTL_2_D",
                                                          q.S_DTL_2_M as "S_DTL_2_M",
                                                            q.S_DTL_2_Y as "S_DTL_2_Y",
                                                              q.S_DTL_2_H as "S_DTL_2_H",
                                                                q.S_DTL_2_MIN as "S_DTL_2_MIN",

                                                        q.E_DTU_1_D as "E_DTU_1_D",
                                                          q.E_DTU_1_M as "E_DTU_1_M",
                                                            q.E_DTU_1_Y as "E_DTU_1_Y",
                                                              q.E_DTU_1_H as "E_DTU_1_H",
                                                                q.E_DTU_1_MIN as "E_DTU_1_MIN",

                                                        q.E_DTL_1_D as "E_DTL_1_D",
                                                          q.E_DTL_1_M as "E_DTL_1_M",
                                                            q.E_DTL_1_Y as "E_DTL_1_Y",
                                                              q.E_DTL_1_H as "E_DTL_1_H",
                                                                q.E_DTL_1_MIN as "E_DTL_1_MIN",

                                                        q.E_DTU_2_D as "E_DTU_2_D",
                                                          q.E_DTU_2_M as "E_DTU_2_M",
                                                            q.E_DTU_2_Y as "E_DTU_2_Y",
                                                              q.E_DTU_2_H as "E_DTU_2_H",
                                                                q.E_DTU_2_MIN as "E_DTU_2_MIN",

                                                        q.E_DTL_2_D as "E_DTL_2_D",
                                                          q.E_DTL_2_M as "E_DTL_2_M",
                                                            q.E_DTL_2_Y as "E_DTL_2_Y",
                                                              q.E_DTL_2_H as "E_DTL_2_H",
                                                                q.E_DTL_2_MIN as "E_DTL_2_MIN",

                                                        q.F_DTU_1_D as "F_DTU_1_D",
                                                          q.F_DTU_1_M as "F_DTU_1_M",
                                                            q.F_DTU_1_Y as "F_DTU_1_Y",
                                                              q.F_DTU_1_H as "F_DTU_1_H",
                                                                q.F_DTU_1_MIN as "F_DTU_1_MIN",

                                                        q.F_DTL_1_D as "F_DTL_1_D",
                                                          q.F_DTL_1_M as "F_DTL_1_M",
                                                            q.F_DTL_1_Y as "F_DTL_1_Y",
                                                              q.F_DTL_1_H as "F_DTL_1_H",
                                                                q.F_DTL_1_MIN as "F_DTL_1_MIN",

                                                        q.F_DTU_2_D as "F_DTU_2_D",
                                                          q.F_DTU_2_M as "F_DTU_2_M",
                                                            q.F_DTU_2_Y as "F_DTU_2_Y",
                                                              q.F_DTU_2_H as "F_DTU_2_H",
                                                                q.F_DTU_2_MIN as "F_DTU_2_MIN",

                                                        q.F_DTL_2_D as "F_DTL_2_D",
                                                          q.F_DTL_2_M as "F_DTL_2_M",
                                                            q.F_DTL_2_Y as "F_DTL_2_Y",
                                                              q.F_DTL_2_H as "F_DTL_2_H",
                                                                q.F_DTL_2_MIN as "F_DTL_2_MIN"))).getClobVal() into cXML_data
            from (select to_char(s.id) id,
                           s.nr,
                             mvl.TYPE_NAME mvl_type,
                               case when P_LANG='RUS' then ws.NAME_RUS_SMALL else ws.NAME_ENG_SMALL end ws_name,
                                 nvl((select bc.BORT_NUM from WB_REF_AIRCO_WS_BORTS bc where bc.id=s.ID_BORT), 'NULL') bort_num,
                                   case when P_LANG='RUS' then p_1.NAME_RUS_SMALL else p_1.NAME_ENG_SMALL end AP_1,
                                     case when P_LANG='RUS' then p_2.NAME_RUS_SMALL else p_2.NAME_ENG_SMALL end AP_2,
                                       p_1.id ID_AP_1,
                                         p_2.id ID_AP_2,
                                           s.TERM_1,
                                             s.TERM_2,

                                               case when s.S_DTU_1 is null then 'NULL' else to_char(extract(day FROM s.S_DTU_1)) end S_DTU_1_D,
                                                 case when s.S_DTU_1 is null then 'NULL' else to_char(extract(month FROM s.S_DTU_1)) end S_DTU_1_M,
                                                   case when s.S_DTU_1 is null then 'NULL' else to_char(extract(year FROM s.S_DTU_1)) end S_DTU_1_Y,
                                                     case when s.S_DTU_1 is null then 'NULL' else to_char(s.S_DTU_1, 'hh24') end S_DTU_1_H,
                                                       case when s.S_DTU_1 is null then 'NULL' else to_char(s.S_DTU_1, 'mi') end S_DTU_1_MIN,

                                               case when s.S_DTL_1 is null then 'NULL' else to_char(extract(day FROM s.S_DTL_1)) end S_DTL_1_D,
                                                 case when s.S_DTL_1 is null then 'NULL' else to_char(extract(month FROM s.S_DTL_1)) end S_DTL_1_M,
                                                   case when s.S_DTL_1 is null then 'NULL' else to_char(extract(year FROM s.S_DTL_1)) end S_DTL_1_Y,
                                                     case when s.S_DTL_1 is null then 'NULL' else to_char(s.S_DTL_1, 'hh24') end S_DTL_1_H,
                                                       case when s.S_DTL_1 is null then 'NULL' else to_char(s.S_DTL_1, 'mi') end S_DTL_1_MIN,

                                               case when s.S_DTU_2 is null then 'NULL' else to_char(extract(day FROM s.S_DTU_2)) end S_DTU_2_D,
                                                 case when s.S_DTU_2 is null then 'NULL' else to_char(extract(month FROM s.S_DTU_2)) end S_DTU_2_M,
                                                   case when s.S_DTU_2 is null then 'NULL' else to_char(extract(year FROM s.S_DTU_2)) end S_DTU_2_Y,
                                                     case when s.S_DTU_2 is null then 'NULL' else to_char(s.S_DTU_2, 'hh24') end S_DTU_2_H,
                                                       case when s.S_DTU_2 is null then 'NULL' else to_char(s.S_DTU_2, 'mi') end S_DTU_2_MIN,

                                               case when s.S_DTL_2 is null then 'NULL' else to_char(extract(day FROM s.S_DTL_2)) end S_DTL_2_D,
                                                 case when s.S_DTL_2 is null then 'NULL' else to_char(extract(month FROM s.S_DTL_2)) end S_DTL_2_M,
                                                   case when s.S_DTL_2 is null then 'NULL' else to_char(extract(year FROM s.S_DTL_2)) end S_DTL_2_Y,
                                                     case when s.S_DTL_2 is null then 'NULL' else to_char(s.S_DTL_2, 'hh24') end S_DTL_2_H,
                                                       case when s.S_DTL_2 is null then 'NULL' else to_char(s.S_DTL_2, 'mi') end S_DTL_2_MIN,

                                               case when s.E_DTU_1 is null then 'NULL' else to_char(extract(day FROM s.E_DTU_1)) end E_DTU_1_D,
                                                 case when s.E_DTU_1 is null then 'NULL' else to_char(extract(month FROM s.E_DTU_1)) end E_DTU_1_M,
                                                   case when s.E_DTU_1 is null then 'NULL' else to_char(extract(year FROM s.E_DTU_1)) end E_DTU_1_Y,
                                                     case when s.E_DTU_1 is null then 'NULL' else to_char(s.E_DTU_1, 'hh24') end E_DTU_1_H,
                                                       case when s.E_DTU_1 is null then 'NULL' else to_char(s.E_DTU_1, 'mi') end E_DTU_1_MIN,

                                               case when s.E_DTL_1 is null then 'NULL' else to_char(extract(day FROM s.E_DTL_1)) end E_DTL_1_D,
                                                 case when s.E_DTL_1 is null then 'NULL' else to_char(extract(month FROM s.E_DTL_1)) end E_DTL_1_M,
                                                   case when s.E_DTL_1 is null then 'NULL' else to_char(extract(year FROM s.E_DTL_1)) end E_DTL_1_Y,
                                                     case when s.E_DTL_1 is null then 'NULL' else to_char(s.E_DTL_1, 'hh24') end E_DTL_1_H,
                                                       case when s.E_DTL_1 is null then 'NULL' else to_char(s.E_DTL_1, 'mi') end E_DTL_1_MIN,

                                               case when s.E_DTU_2 is null then 'NULL' else to_char(extract(day FROM s.E_DTU_2)) end E_DTU_2_D,
                                                 case when s.E_DTU_2 is null then 'NULL' else to_char(extract(month FROM s.E_DTU_2)) end E_DTU_2_M,
                                                   case when s.E_DTU_2 is null then 'NULL' else to_char(extract(year FROM s.E_DTU_2)) end E_DTU_2_Y,
                                                     case when s.E_DTU_2 is null then 'NULL' else to_char(s.E_DTU_2, 'hh24') end E_DTU_2_H,
                                                       case when s.E_DTU_2 is null then 'NULL' else to_char(s.E_DTU_2, 'mi') end E_DTU_2_MIN,

                                               case when s.E_DTL_2 is null then 'NULL' else to_char(extract(day FROM s.E_DTL_2)) end E_DTL_2_D,
                                                 case when s.E_DTL_2 is null then 'NULL' else to_char(extract(month FROM s.E_DTL_2)) end E_DTL_2_M,
                                                   case when s.E_DTL_2 is null then 'NULL' else to_char(extract(year FROM s.E_DTL_2)) end E_DTL_2_Y,
                                                     case when s.E_DTL_2 is null then 'NULL' else to_char(s.E_DTL_2, 'hh24') end E_DTL_2_H,
                                                       case when s.E_DTL_2 is null then 'NULL' else to_char(s.E_DTL_2, 'mi') end E_DTL_2_MIN,

                                               case when s.F_DTU_1 is null then 'NULL' else to_char(extract(day FROM s.F_DTU_1)) end F_DTU_1_D,
                                                 case when s.F_DTU_1 is null then 'NULL' else to_char(extract(month FROM s.F_DTU_1)) end F_DTU_1_M,
                                                   case when s.F_DTU_1 is null then 'NULL' else to_char(extract(year FROM s.F_DTU_1)) end F_DTU_1_Y,
                                                     case when s.F_DTU_1 is null then 'NULL' else to_char(s.F_DTU_1, 'hh24') end F_DTU_1_H,
                                                       case when s.F_DTU_1 is null then 'NULL' else to_char(s.F_DTU_1, 'mi') end F_DTU_1_MIN,

                                               case when s.F_DTL_1 is null then 'NULL' else to_char(extract(day FROM s.F_DTL_1)) end F_DTL_1_D,
                                                 case when s.F_DTL_1 is null then 'NULL' else to_char(extract(month FROM s.F_DTL_1)) end F_DTL_1_M,
                                                   case when s.F_DTL_1 is null then 'NULL' else to_char(extract(year FROM s.F_DTL_1)) end F_DTL_1_Y,
                                                     case when s.F_DTL_1 is null then 'NULL' else to_char(s.F_DTL_1, 'hh24') end F_DTL_1_H,
                                                       case when s.F_DTL_1 is null then 'NULL' else to_char(s.F_DTL_1, 'mi') end F_DTL_1_MIN,

                                               case when s.F_DTU_2 is null then 'NULL' else to_char(extract(day FROM s.F_DTU_2)) end F_DTU_2_D,
                                                 case when s.F_DTU_2 is null then 'NULL' else to_char(extract(month FROM s.F_DTU_2)) end F_DTU_2_M,
                                                   case when s.F_DTU_2 is null then 'NULL' else to_char(extract(year FROM s.F_DTU_2)) end F_DTU_2_Y,
                                                     case when s.F_DTU_2 is null then 'NULL' else to_char(s.F_DTU_2, 'hh24') end F_DTU_2_H,
                                                       case when s.F_DTU_2 is null then 'NULL' else to_char(s.F_DTU_2, 'mi') end F_DTU_2_MIN,

                                               case when s.F_DTL_2 is null then 'NULL' else to_char(extract(day FROM s.F_DTL_2)) end F_DTL_2_D,
                                                 case when s.F_DTL_2 is null then 'NULL' else to_char(extract(month FROM s.F_DTL_2)) end F_DTL_2_M,
                                                   case when s.F_DTL_2 is null then 'NULL' else to_char(extract(year FROM s.F_DTL_2)) end F_DTL_2_Y,
                                                     case when s.F_DTL_2 is null then 'NULL' else to_char(s.F_DTL_2, 'hh24') end F_DTL_2_H,
                                                       case when s.F_DTL_2 is null then 'NULL' else to_char(s.F_DTL_2, 'mi') end F_DTL_2_MIN
                  from WB_SHED s join WB_SHED_MVL_TYPE mvl
                  on P_ID_REIS=s.id and
                     mvl.id=s.MVL_TYPE join WB_REF_AIRPORTS p_1
                        on p_1.id=s.ID_AP_1 join WB_REF_AIRPORTS p_2
                           on p_2.id=s.ID_AP_2 join WB_REF_WS_TYPES ws
                              on ws.id=s.id_ws) q;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';

  end SP_WB_SHED_FOR_LEGS_LIST;
/
