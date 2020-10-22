create or replace procedure SP_WB_REF_WS_AIR_SL_CAI_T_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ADV_ID number:=-1;
P_SL_ADV_ID number:=-1;
V_ID_AC number:=0;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SL_ADV_ID[1]')) into P_SL_ADV_ID from dual;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_C_ADV
  where id=P_ADV_ID;

  if V_R_COUNT>0 then
    begin
      select id_ac into V_ID_AC
      from WB_REF_WS_AIR_S_L_C_ADV
      where id=P_ADV_ID;

      select count(id) into V_R_COUNT
      from WB_REF_AIRCO_CLASS_CODES
      where (V_ID_AC<>-1 and ID_AC=V_ID_AC) or
            (V_ID_AC=-1);

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("CLASS_CODE_LIST",
                      xmlattributes(q.CLASS_CODE as "CLASS_CODE"))).getClobVal() into cXML_data
          from (select distinct CLASS_CODE
                from WB_REF_AIRCO_CLASS_CODES
                where (V_ID_AC<>-1 and ID_AC=V_ID_AC) or
                      (V_ID_AC=-1)
                order by CLASS_CODE) q
          order by q.CLASS_CODE;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;

      select count(cd.id) into V_R_COUNT
      from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
      on a.id=P_SL_ADV_ID and
         a.cabin_id=cd.idn;

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("CABIN_SECTION_LIST",
                      xmlattributes(q.section as "section"))).getClobVal() into cXML_data
          from (select distinct cd.section
                from WB_REF_WS_AIR_SEAT_LAY_ADV a join WB_REF_WS_AIR_CABIN_CD cd
                on a.id=P_SL_ADV_ID and
                   a.cabin_id=cd.idn
            order by cd.section) q
          order by q.section;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;

      select count(t.id) into V_R_COUNT
      from WB_REF_WS_AIR_SL_CAI_T t join WB_REF_AIRCO_CLASS_CODES cc
      on t.adv_id=P_ADV_ID and
         (t.ID_AC<>-1 and t.ID_AC=V_ID_AC) or
         (t.ID_AC=-1);

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("TABLE_DATA",
                  xmlattributes(to_char(q.t_id) as "t_ID",
                                  q.CABIN_SECTION as "CABIN_SECTION",
                                    q.class_code as "CLASS_CODE",
                                      q.total_per_cabin as "TOTAL_PER_CABIN",
                                        q.NUM_OF_SEATS as "NUM_OF_SEATS",
                                          q.BA_CENTROID as "BA_CENTROID",
                                            q.BA_FWD as "BA_FWD",
                                              q.BA_AFT as "BA_AFT",
                                                q.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT",
                                                  q.U_NAME as "U_NAME",
                                                    q.U_IP as "U_IP",
                                                      q.U_HOST_NAME as "U_HOST_NAME",
                                                        q.DATE_WRITE as "DATE_WRITE"))).getClobVal() into cXML_data
          from (select distinct t.id t_id,
                         t.cabin_section,
                           cc.class_code,
                             nvl((select to_char(sum(ttt.NUM_OF_SEATS))
                                  from WB_REF_WS_AIR_SL_CAI_TT ttt
                                  where ttt.t_id=t.id), 'NULL') total_per_cabin,
                               nvl(to_char(tt.NUM_OF_SEATS), 'NULL') as "NUM_OF_SEATS",
                                 nvl(to_char(t.BA_CENTROID), 'NULL') as "BA_CENTROID",
                                   nvl(to_char(t.BA_FWD), 'NULL') as "BA_FWD",
	                                   nvl(to_char(t.BA_AFT), 'NULL') as "BA_AFT",
	                                     nvl(to_char(t.INDEX_PER_WT_UNIT), 'NULL') as "INDEX_PER_WT_UNIT",
                                         t.U_NAME,
                                           t.U_IP,
                                             t.U_HOST_NAME,
                                               to_char(t.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
                from WB_REF_WS_AIR_SL_CAI_T t join WB_REF_AIRCO_CLASS_CODES cc
                on t.adv_id=P_ADV_ID and
                  (t.ID_AC<>-1 and t.ID_AC=V_ID_AC) or
                  (t.ID_AC=-1) left outer join WB_REF_WS_AIR_SL_CAI_TT tt
                  on tt.t_id=t.id and
                     tt.CLASS_CODE=cc.CLASS_CODE
                order by t.CABIN_SECTION,
                           cc.class_code) q
          order by q.CABIN_SECTION,
                   q.class_code;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SL_CAI_T_DATA;
/
