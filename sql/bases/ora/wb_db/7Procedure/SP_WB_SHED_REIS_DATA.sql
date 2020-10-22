create or replace procedure SP_WB_SHED_REIS_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='ENG';

cXML_data clob;
V_R_COUNT number:=0;
begin
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

   select count(id) into V_R_COUNT
   from WB_SHED
   where id=P_ID;

   if V_R_COUNT>0 then
     begin
       select XMLAGG(XMLELEMENT("table_data",
                    xmlattributes(q.ID as "id",
                                    q.airline_name as "AIRLINE_NAME",
                                      q.nr as "NR",
                                        q.mvl_type as "MVL_TYPE",
                                          q.ws_name as "WS_NAME",
                                            q.bort_num as "BORT_NUM",
                                              q.ap_1 as "AP_1",
                                                q.ap_2 as "AP_2",
                                                  q.TERM_1 as "TERM_1",
                                                    q.TERM_2 as "TERM_2",
                                                      q.S_DTU_1 as "S_DTU_1",
                                                        q.S_DTU_2 as "S_DTU_2",
                                                          q.S_DTL_1 as "S_DTL_1",
                                                            q.S_DTL_2 as "S_DTL_2",
                                                              q.E_DTU_1 as "E_DTU_1",
                                                                q.E_DTU_2 as "E_DTU_2",
                                                                  q.E_DTL_1 as "E_DTL_1",
                                                                    q.E_DTL_2 as "E_DTL_2",
                                                                      q.F_DTU_1 as "F_DTU_1",
                                                                        q.F_DTU_2 as "F_DTU_2",
                                                                          q.F_DTL_1 as "F_DTL_1",
                                                                            q.F_DTL_2 as "F_DTL_2",
                                                                              q.U_NAME as "U_NAME",
                                                                                q.U_IP as "U_IP",
                                                                                  q.U_HOST_NAME as "U_HOST_NAME",
                                                                                    q.date_write as "date_write"))).getClobVal() into cXML_data
        from   (select to_char(s.id) id,
                         case when P_LANG='RUS' then ac.NAME_RUS_FULL else ac.NAME_ENG_FULL end airline_name,
                           s.nr,
                             mvl.TYPE_NAME mvl_type,
                               case when P_LANG='RUS' then ws.NAME_RUS_SMALL else ws.NAME_ENG_SMALL end ws_name,
                                 nvl((select bc.BORT_NUM from WB_REF_AIRCO_WS_BORTS bc where bc.id=s.ID_BORT), 'NULL') bort_num,
                                   case when P_LANG='RUS' then p_1.NAME_RUS_SMALL else p_1.NAME_ENG_SMALL end AP_1,
                                     case when P_LANG='RUS' then p_2.NAME_RUS_SMALL else p_2.NAME_ENG_SMALL end AP_2,
                                       s.TERM_1,
                                         s.TERM_2,
                                           nvl(to_char(S_DTU_1, 'dd.mm.yyyy hh24:mi'), 'NULL') S_DTU_1,
                                             nvl(to_char(S_DTU_2, 'dd.mm.yyyy hh24:mi'), 'NULL') S_DTU_2,
                                               nvl(to_char(S_DTL_1, 'dd.mm.yyyy hh24:mi'), 'NULL') S_DTL_1,
                                                 nvl(to_char(S_DTL_2, 'dd.mm.yyyy hh24:mi'), 'NULL') S_DTL_2,
                                                   nvl(to_char(E_DTU_1, 'dd.mm.yyyy hh24:mi'), 'NULL') E_DTU_1,
                                                     nvl(to_char(E_DTU_2, 'dd.mm.yyyy hh24:mi'), 'NULL') E_DTU_2,
                                                       nvl(to_char(E_DTL_1, 'dd.mm.yyyy hh24:mi'), 'NULL') E_DTL_1,
                                                         nvl(to_char(E_DTL_2, 'dd.mm.yyyy hh24:mi'), 'NULL') E_DTL_2,
                                                           nvl(to_char(F_DTU_1, 'dd.mm.yyyy hh24:mi'), 'NULL') F_DTU_1,
                                                             nvl(to_char(F_DTU_2, 'dd.mm.yyyy hh24:mi'), 'NULL') F_DTU_2,
                                                               nvl(to_char(F_DTL_1, 'dd.mm.yyyy hh24:mi'), 'NULL') F_DTL_1,
                                                                 nvl(to_char(F_DTL_2, 'dd.mm.yyyy hh24:mi'), 'NULL') F_DTL_2,
                                                                   s.U_NAME,
                                                                     s.U_IP,
                                                                       s.U_HOST_NAME,
                                                                         to_char(s.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
              from WB_SHED s join WB_SHED_MVL_TYPE mvl
              on P_ID=s.id and
                 mvl.id=s.MVL_TYPE join WB_REF_AIRPORTS p_1
                 on p_1.id=s.ID_AP_1 join WB_REF_AIRPORTS p_2
                    on p_2.id=s.ID_AP_2 join WB_REF_AIRCOMPANY_ADV_INFO ac
                       on ac.id_ac=s.ID_AC and
                          ac.date_from=nvl((select max(acc.date_from)
                                            from WB_REF_AIRCOMPANY_ADV_INFO acc
                                            where acc.id_ac=ac.id_ac and
                                                  acc.date_from<=sysdate), (select min(acc.date_from)
                                                                            from WB_REF_AIRCOMPANY_ADV_INFO acc
                                                                            where acc.id_ac=ac.id_ac and
                                                                                  acc.date_from>sysdate)) join WB_REF_WS_TYPES ws
                          on ws.id=s.id_ws) q;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_SHED_REIS_DATA;
/
