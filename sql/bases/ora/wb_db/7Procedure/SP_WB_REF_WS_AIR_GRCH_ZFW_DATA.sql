create or replace procedure SP_WB_REF_WS_AIR_GRCH_ZFW_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data clob;
R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id),
             id_ac,
               id_ws,
                 id_bort
           into R_COUNT,
                  P_ID_AC,
                    P_ID_WS,
                      P_ID_BORT
    from WB_REF_WS_AIR_GR_CH_ADV
    where idn=P_ID
    group by id_ac,
               id_ws,
                 id_bort;

    if R_COUNT>0 then
      begin
         cXML_out:='<?xml version="1.0" ?><root>';

        select '<adv_data TABLE_NAME="'||i.TABLE_NAME||'" '||
                         'CONDITION="'||WB_CLEAR_XML(i.CONDITION)||'" '||
                         'TYPE="'||WB_CLEAR_XML(i.TYPE)||'" '||
                         'TYPE_FROM="'||nvl(to_char(i.TYPE_FROM), 'NULL')||'" '||
                         'TYPE_TO="'||nvl(to_char(i.TYPE_TO), 'NULL')||'" '||
                         'CH_INDEX="'||to_char(i.CH_INDEX)||'" '||
                         'CH_PROC_MAC="'||to_char(i.CH_PROC_MAC)||'" '||
                         'CH_CERTIFIED="'||to_char(i.CH_CERTIFIED)||'" '||
                         'CH_CURTAILED="'||to_char(i.CH_CURTAILED)||'" '||
                         'REMARK_1="'||WB_CLEAR_XML(i.REMARK_1)||'" '||
                         'REMARK_2="'||WB_CLEAR_XML(i.REMARK_2)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
           FROM WB_REF_WS_AIR_GR_CH_ADV i
           WHERE i.IDN=P_ID;

        cXML_out:=cXML_out||cXML_data;

        select count(al.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_A_L al
        on a.idn=P_ID and
           al.id_ch=a.id;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("a_l_data", xmlattributes(to_char(e.id) "id",
                                                                 to_char(e.weight) "WEIGHT",
                                                                   to_char(e.proc_mac) as "proc_mac",
                                                                     to_char(e.indx) as "indx"))).getClobVal() into cXML_data
            from (select al.id,
                           al.weight,
                             al.proc_mac,
                               al.indx
            from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_A_L al
            on a.idn=P_ID and
               al.id_ch=a.id
            order by al.weight) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(al.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_F_L al
        on a.idn=P_ID and
           al.id_ch=a.id;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("f_l_data", xmlattributes(to_char(e.id) "id",
                                                                 to_char(e.weight) "WEIGHT",
                                                                   to_char(e.proc_mac) as "proc_mac",
                                                                     to_char(e.indx) as "indx"))).getClobVal() into cXML_data
            from (select al.id,
                           al.weight,
                             al.proc_mac,
                               al.indx
            from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_F_L al
            on a.idn=P_ID and
               al.id_ch=a.id
            order by al.weight) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(al.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_ITL_L al
        on a.idn=P_ID and
           al.id_ch=a.id;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("itl_l_data", xmlattributes(to_char(e.id) "id",
                                                                   to_char(e.weight) "WEIGHT",
                                                                     to_char(e.proc_mac) as "proc_mac",
                                                                       to_char(e.indx) as "indx"))).getClobVal() into cXML_data
            from (select al.id,
                           al.weight,
                             al.proc_mac,
                               al.indx
            from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_ITL_L al
            on a.idn=P_ID and
               al.id_ch=a.id
            order by al.weight) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(al.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_L_L al
        on a.idn=P_ID and
           al.id_ch=a.id;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("l_l_data", xmlattributes(to_char(e.id) "id",
                                                                   to_char(e.weight) "WEIGHT",
                                                                     to_char(e.proc_mac) as "proc_mac",
                                                                       to_char(e.indx) as "indx"))).getClobVal() into cXML_data
            from (select al.id,
                           al.weight,
                             al.proc_mac,
                               al.indx
            from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_L_L al
            on a.idn=P_ID and
               al.id_ch=a.id
            order by al.weight) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(u.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_USE u
        on a.idn=P_ID and
           u.id_ch=a.id;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("use_data", xmlattributes(q.ITEM_NAME as "ITEM_NAME",
                                                                 to_char(q.USE_ITEM_ID) as "use_item_id",
                                                                   q.value_1 as "value_1",
                                                                     q.value_2 as "value_2"))).getClobVal() into cXML_data
            from (select ui.ITEM_NAME,
                           case when u.value_1 is NULL
                                then 'NULL'
                                else to_char(u.value_1)
                           end value_1,
                             case when u.value_2 is NULL
                                  then 'NULL'
                                  else to_char(u.value_2)
                             end value_2,
                               u.USE_ITEM_ID
                  from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_WS_AIR_GR_CH_USE u
                  on a.idn=P_ID and
                     u.id_ch=a.id join WB_REF_WS_AIR_GR_CH_USE_ITEMS ui
                     on ui.id=u.USE_ITEM_ID
                  order by ui.ITEM_NAME,
                           nvl(u.value_1, -999999999999999999),
                           nvl(u.value_2, 99999999999999999)) q;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(a.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_IDN a
        where a.id<>P_ID and
              a.id_ac=P_ID_AC and
              a.id_ws=P_ID_WS and
              a.id_bort=P_ID_BORT and
              a.ch_type in ('ZFW', 'TAW', 'TOW', 'LDW', 'Inflight', 'Other');

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("ch_rel", xmlattributes(q.idn as "idn",
                                                               q.ch_type as "ch_type",
                                                                 q.table_name as "table_name"))).getClobVal() into cXML_data
            from (select to_char(i.id) as idn,
                           i.ch_type,
                             a.table_name
                  from WB_REF_WS_AIR_GR_CH_IDN i join WB_REF_WS_AIR_GR_CH_ADV a
                  on i.id<>P_ID and
                     i.id_ac=P_ID_AC and
                     i.id_ws=P_ID_WS and
                     i.id_bort=P_ID_BORT and
                     i.ch_type in ('ZFW', 'TAW', 'TOW', 'LDW', 'Inflight', 'Other') and
                     i.id=a.idn
                  order by i.ch_type,
                           a.table_name,
                           i.id) q;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(a.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_REL a
        where a.IDN=P_ID and
              a.idn_rel>-1;

        if R_COUNT>0 then
          begin

            SELECT XMLAGG(XMLELEMENT("idn_from_rel", xmlattributes(to_char(idn_rel) as "idn_rel"))).getClobVal() into cXML_data
            from WB_REF_WS_AIR_GR_CH_REL a
            where a.IDN=P_ID;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        ------------------------------------------------------------------------
        ------------------------èêàÇüáäÄ ä ÅéêíÄå-------------------------------
        select count(b.id) into R_COUNT
        from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_AIRCO_WS_BORTS b
        on a.idn=P_ID and
           a.ID_AC=b.ID_AC and
           a.ID_WS=b.ID_WS;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("bort_list_data", xmlattributes(e.id as "id",
                                                                       e.BORT_NUM as "BORT_NUM",
                                                                         e.is_checked as "IS_CHECKED"))).getClobVal() into cXML_data
            from (select to_char(b.id) as id,
                           b.BORT_NUM,
                             case when exists(select cb.id
                                              from WB_REF_WS_AIR_GR_CH_BORT cb
                                              where cb.id_bort=b.id and
                                                    cb.id_CH=a.id)
                                  then '1'
                                  else '0'
                              end is_checked
                  from WB_REF_WS_AIR_GR_CH_ADV a join WB_REF_AIRCO_WS_BORTS b
                  on a.idn=P_ID and
                     a.ID_AC=b.ID_AC and
                     a.ID_WS=b.ID_WS
                  order by b.BORT_NUM) e
            order by e.BORT_NUM;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;
        ------------------------èêàÇüáäÄ ä ÅéêíÄå-------------------------------
        ------------------------------------------------------------------------


        cXML_out:=cXML_out||'</root>';
     end;
    end if;
end SP_WB_REF_WS_AIR_GRCH_ZFW_DATA;
/
