create or replace procedure SP_WB_REF_WS_AIR_HLD_DECK_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data clob;
R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select XMLAGG(XMLELEMENT("deck_list",
                    xmlattributes(to_char(q."ID") "id",
                                            q."NAME" "NAME"))).getClobVal()
    into cXML_data
    from (select id,
                 NAME
          from WB_REF_WS_DECK
          order by SORT_PRIOR) q;

     if cXML_data is not NULL then
       begin
         cXML_out:=cXML_out||cXML_data;
       end;
     end if;

    select count(h.id) into R_COUNT
    from WB_REF_WS_AIR_HLD_DECK h join WB_REF_WS_DECK d
    on h.id_ac=P_ID_AC and
       h.id_ws=P_ID_WS and
       h.id_bort=P_ID_BORT and
       h.deck_id=d.id;

    if R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("deck_data", xmlattributes(e.id as "id",
                                                              e.deck_name "deck_name",
                                                                e.MAX_WEIGHT as "MAX_WEIGHT",
                                                                  e.MAX_VOLUME as "MAX_VOLUME",
                                                                    e.LA_FROM as "LA_FROM",
                                                                      e.LA_TO as "LA_TO",
                                                                        e.BA_FWD as "BA_FWD",
                                                                          e.BA_AFT as "BA_AFT",
                                                                            e.U_NAME as "U_NAME",
                                                                              e.U_IP as "U_IP",
                                                                                e.U_HOST_NAME as "U_HOST_NAME",
                                                                                  e.date_write as "date_write"))).getClobVal() into cXML_data
        from (select to_char(h.id) as id,
                       d.name as deck_name,
                         case when h.MAX_WEIGHT is null then 'NULL' else to_char(h.MAX_WEIGHT) end as MAX_WEIGHT,
                           case when h.MAX_VOLUME is null then 'NULL' else to_char(h.MAX_VOLUME) end as MAX_VOLUME,
                             case when h.LA_FROM is null then 'NULL' else to_char(h.LA_FROM) end as LA_FROM,
                               case when h.LA_TO is null then 'NULL' else to_char(h.LA_TO) end as LA_TO,
                                 case when h.BA_FWD is null then 'NULL' else to_char(h.BA_FWD) end as BA_FWD,
                                   case when h.BA_AFT is null then 'NULL' else to_char(h.BA_AFT) end as BA_AFT,
                                     h.U_NAME,
                                       h.U_IP,
                                         h.U_HOST_NAME,
                                           to_char(h.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as date_write
               from WB_REF_WS_AIR_HLD_DECK h join WB_REF_WS_DECK d
               on h.id_ac=P_ID_AC and
                  h.id_ws=P_ID_WS and
                  h.id_bort=P_ID_BORT and
                  h.deck_id=d.id
               order by d.SORT_PRIOR) e;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_HLD_DECK_DATA;
/
