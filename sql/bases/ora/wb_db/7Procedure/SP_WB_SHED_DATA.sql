create or replace procedure SP_WB_SHED_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_DATE_FROM date:=null;
P_DATE_TO date:=null;
P_IS_DATE_UTC number:=1;
P_SORT_MODE number:=1;
P_SORT_MODE_EX varchar2(50):='ASC';
P_LANG varchar2(50):='ENG';

P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_DATE_TO_D varchar2(50);
P_DATE_TO_M varchar2(50);
P_DATE_TO_Y varchar2(50);

cXML_data clob;
V_R_COUNT number:=0;
V_SQL_STRING clob;
begin

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SORT_MODE[1]')) into P_SORT_MODE from dual;
   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_DATE_UTC[1]')) into P_IS_DATE_UTC from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SORT_MODE_EX[1]') into P_SORT_MODE_EX from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_D[1]') into P_DATE_TO_D from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_M[1]') into P_DATE_TO_M from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_TO_Y[1]') into P_DATE_TO_Y from dual;

   P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y||' 0:00', 'dd.mm.yyyy hh24:mi');
   P_DATE_TO:=to_date(P_DATE_TO_D||'.'||P_DATE_TO_M||'.'||P_DATE_TO_Y||' 23:59', 'dd.mm.yyyy hh24:mi');

    cXML_out:='<?xml version="1.0" ?><root>';

   if P_IS_DATE_UTC=1 then
     begin
       insert into WB_TEMP_XML_ID (ID,
                                     num,
                                       DATE_FROM)
       select id,
                1,
                  nvl(F_DTU_1, nvl(E_DTU_1, S_DTU_1))
       from WB_SHED
       where ((P_ID_AC=-1) or (P_ID_AC=ID_AC)) and
             nvl(F_DTU_1, nvl(E_DTU_1, S_DTU_1)) between P_DATE_FROM and P_DATE_TO;
     end;
   else
     begin
       insert into WB_TEMP_XML_ID (ID,
                                     num,
                                       DATE_FROM)
       select id,
                -1,
                  nvl(F_DTL_1, nvl(E_DTL_1, S_DTL_1))
       from WB_SHED
       where ((P_ID_AC=-1) or (P_ID_AC=ID_AC)) and
             nvl(F_DTL_1, nvl(E_DTL_1, S_DTL_1)) between P_DATE_FROM and P_DATE_TO;
     end;
   end if;

   select count(id) into V_R_COUNT
   from WB_TEMP_XML_ID;

   if V_R_COUNT>0 then
     begin
       V_SQL_STRING:='insert into WB_TEMP_XML_ID (ID, num) '||
                     'select s.id, row_number() over(';

     if P_SORT_MODE=1 then
       begin
         if P_LANG='RUS' then
           begin
             V_SQL_STRING:=V_SQL_STRING||'order by ac.NAME_RUS_FULL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         else
           begin
              V_SQL_STRING:=V_SQL_STRING||'order by ac.NAME_ENG_FULL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         end if;
       end;
     end if;

     if P_SORT_MODE=2 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.nr '||P_SORT_MODE_EX||',t.DATE_FROM';
       end;
     end if;

     if P_SORT_MODE=3 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by mvl.TYPE_NAME '||P_SORT_MODE_EX||',s.nr,t.DATE_FROM';
       end;
     end if;

      if P_SORT_MODE=4 then
       begin
         if P_LANG='RUS' then
           begin
             V_SQL_STRING:=V_SQL_STRING||'order by ws.NAME_RUS_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         else
           begin
              V_SQL_STRING:=V_SQL_STRING||'order by ws.NAME_ENG_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         end if;
       end;
     end if;

     if P_SORT_MODE=5 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by (select bc.BORT_NUM from WB_REF_AIRCO_WS_BORTS bc where bc.id=s.ID_BORT) '||P_SORT_MODE_EX||',s.nr,t.DATE_FROM';
       end;
     end if;

     if P_SORT_MODE=6 then
       begin
         if P_LANG='RUS' then
           begin
             V_SQL_STRING:=V_SQL_STRING||'order by p_1.NAME_RUS_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         else
           begin
              V_SQL_STRING:=V_SQL_STRING||'order by p_1.NAME_ENG_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         end if;
       end;
     end if;

     if P_SORT_MODE=7 then
       begin
         if P_LANG='RUS' then
           begin
             V_SQL_STRING:=V_SQL_STRING||'order by p_2.NAME_RUS_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         else
           begin
              V_SQL_STRING:=V_SQL_STRING||'order by p_2.NAME_ENG_SMALL '||P_SORT_MODE_EX||',t.DATE_FROM';
           end;
         end if;
       end;
     end if;

     if P_SORT_MODE=8 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.S_DTU_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=9 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.E_DTU_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=10 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.F_DTU_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=11 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.S_DTU_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=12 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.E_DTU_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=13 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.F_DTU_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=14 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.S_DTL_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=15 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.E_DTL_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=16 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.F_DTL_1 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=17 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.S_DTL_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=18 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.E_DTL_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=19 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.F_DTL_2 '||P_SORT_MODE_EX||',s.nr';
       end;
     end if;

     if P_SORT_MODE=20 then
       begin
         V_SQL_STRING:=V_SQL_STRING||'order by s.U_NAME '||P_SORT_MODE_EX||',s.DATE_WRITE';
       end;
     end if;

      V_SQL_STRING:=V_SQL_STRING||') from WB_TEMP_XML_ID t join WB_SHED s '||
                                  'on t.id=s.id join WB_SHED_MVL_TYPE mvl '||
                                     'on mvl.id=s.MVL_TYPE join WB_REF_AIRPORTS p_1 '||
                                        'on p_1.id=s.ID_AP_1 join WB_REF_AIRPORTS p_2 '||
                                           'on p_2.id=s.ID_AP_2 join WB_REF_AIRCOMPANY_ADV_INFO ac '||
                                              'on ac.id_ac=s.ID_AC and '||
                                                 'ac.date_from=nvl((select max(acc.date_from) '||
                                                                   'from WB_REF_AIRCOMPANY_ADV_INFO acc '||
                                                                   'where acc.id_ac=ac.id_ac and '||
                                                                         'acc.date_from<=sysdate), (select min(acc.date_from) '||
                                                                                                   'from WB_REF_AIRCOMPANY_ADV_INFO acc '||
                                                                                                   'where acc.id_ac=ac.id_ac and '||
                                                                                                         'acc.date_from>sysdate)) join WB_REF_WS_TYPES ws '||
                                                 'on ws.id=s.id_ws ';

      execute IMMEDIATE V_SQL_STRING;


      delete from WB_TEMP_XML_ID where num=-1;

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
        from (select t.num id_sort,
                       to_char(s.id) id,
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
              from WB_TEMP_XML_ID t join WB_SHED s
              on t.id=s.id join WB_SHED_MVL_TYPE mvl
                 on mvl.id=s.MVL_TYPE join WB_REF_AIRPORTS p_1
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
                             on ws.id=s.id_ws
              order by t.num) q
      order by q.id_sort;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_SHED_DATA;
/
