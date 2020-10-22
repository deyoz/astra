create or replace procedure SP_WB_REF_WS_AIR_SL_CI_T_DATA
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



      select count(t.id) into V_R_COUNT
      from WB_REF_WS_AIR_SL_CI_T t join WB_REF_AIRCO_CLASS_CODES cc
      on t.adv_id=P_ADV_ID and
         (t.ID_AC<>-1 and t.ID_AC=V_ID_AC) or
         (t.ID_AC=-1);

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("TABLE_DATA",
                  xmlattributes(to_char(q.id) as "ID",
                                  q.class_code as "CLASS_CODE",
                                    q.FIRST_ROW as "FIRST_ROW",
                                      q.LAST_ROW as "LAST_ROW",
                                        q.NUM_OF_SEATS as "NUM_OF_SEATS",
                                          q.LA_FROM as "LA_FROM",
                                            q.LA_TO as "LA_TO",
                                              q.BA_CENTROID as "BA_CENTROID",
                                                q.BA_FWD as "BA_FWD",
                                                  q.BA_AFT as "BA_AFT",
                                                    q.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT",
                                                      q.U_NAME as "U_NAME",
                                                        q.U_IP as "U_IP",
                                                          q.U_HOST_NAME as "U_HOST_NAME",
                                                            q.DATE_WRITE as "DATE_WRITE"))).getClobVal() into cXML_data
          from (select distinct t.id,
                                  cc.class_code,
                                    to_char(t.FIRST_ROW) FIRST_ROW,
                                      to_char(t.LAST_ROW) LAST_ROW,
                                        to_char(t.NUM_OF_SEATS) NUM_OF_SEATS,
                                           to_char(t.LA_FROM) LA_FROM,
                                             to_char(t.LA_TO) LA_TO,
                                               nvl(to_char(t.BA_CENTROID), 'NULL') as "BA_CENTROID",
                                                 nvl(to_char(t.BA_FWD), 'NULL') as "BA_FWD",
	                                                 nvl(to_char(t.BA_AFT), 'NULL') as "BA_AFT",
	                                                   nvl(to_char(t.INDEX_PER_WT_UNIT), 'NULL') as "INDEX_PER_WT_UNIT",
                                                       t.U_NAME,
                                                         t.U_IP,
                                                           t.U_HOST_NAME,
                                                             to_char(t.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
                from WB_REF_WS_AIR_SL_CI_T t join WB_REF_AIRCO_CLASS_CODES cc
                on t.adv_id=P_ADV_ID and
                  ((t.ID_AC<>-1 and t.ID_AC=cc.id_ac) or
                   (t.ID_AC=-1)) and
                  cc.class_code=t.class_code
                order by cc.class_code,
                         t.FIRST_ROW,
                         t.last_row) q
          order by q.class_code,
                   q.FIRST_ROW,
                   q.last_row;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;
    end;
  end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SL_CI_T_DATA;
/
