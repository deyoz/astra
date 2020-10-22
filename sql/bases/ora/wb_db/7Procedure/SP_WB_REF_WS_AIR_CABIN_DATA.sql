create or replace procedure SP_WB_REF_WS_AIR_CABIN_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
cXML_data clob;
R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id) into R_COUNT
    from WB_REF_WS_AIR_CABIN_ADV
    where idn=P_ID;

    if R_COUNT>0 then
      begin
         cXML_out:='<?xml version="1.0" ?><root>';

        select '<adv_data TABLE_NAME="'||a.TABLE_NAME||'" '||
                         'CD_BALANCE_ARM="'||to_char(i.CD_BALANCE_ARM)||'" '||
	                       'CD_INDEX_UNIT="'||to_char(i.CD_INDEX_UNIT)||'" '||
	                       'FDL_BALANCE_ARM="'||to_char(i.FDL_BALANCE_ARM)||'" '||
	                       'FDL_INDEX_UNIT="'||to_char(i.FDL_INDEX_UNIT)||'" '||
	                       'CCL_BALANCE_ARM="'||to_char(i.CCL_BALANCE_ARM)||'" '||
	                       'CCL_INDEX_UNIT="'||to_char(i.CCL_INDEX_UNIT)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
           FROM WB_REF_WS_AIR_CABIN_ADV i join WB_REF_WS_AIR_CABIN_IDN a
           on i.IDN=a.id and
              a.id=P_ID;

        cXML_out:=cXML_out||cXML_data;

        select count(id) into R_COUNT
        from WB_REF_WS_AIR_CABIN_CD
        where idn=P_ID;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("cd_data", xmlattributes(e.section "section",
                                                                e.deck_name "deck_name",
                                                                  e.deck_id as "deck_id",
                                                                    e.ROWS_FROM as "ROWS_FROM",
                                                                      e.ROWS_TO as "ROWS_TO",
                                                                        e.LA_FROM as "LA_FROM",
                                                                          e.LA_TO as "LA_TO",
                                                                            e.BA_CENTROID as "BA_CENTROID",
                                                                              e.BA_FWD as "BA_FWD",
                                                                                e.BA_AFT as "BA_AFT",
                                                                                  e.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT"))).getClobVal() into cXML_data
            from (select l.section,
                           d.name deck_name,
                             to_char(d.id) deck_id,
                               case when l.ROWS_FROM is null then 'NULL' else to_char(l.ROWS_FROM) end ROWS_FROM,
                                 case when l.ROWS_TO is null then 'NULL' else to_char(l.ROWS_TO) end ROWS_TO,
                                   case when l.LA_FROM is null then 'NULL' else to_char(l.LA_FROM) end LA_FROM,
                                     case when l.LA_TO is null then 'NULL' else to_char(l.LA_TO) end LA_TO,
                                       case when l.BA_CENTROID is null then 'NULL' else to_char(l.BA_CENTROID) end BA_CENTROID,
                                         case when l.BA_FWD is null then 'NULL' else to_char(l.BA_FWD) end BA_FWD,
                                           case when l.BA_AFT is null then 'NULL' else to_char(l.BA_AFT) end BA_AFT,
                                             case when l.INDEX_PER_WT_UNIT is null then 'NULL' else to_char(l.INDEX_PER_WT_UNIT) end INDEX_PER_WT_UNIT
                  from WB_REF_WS_AIR_CABIN_CD l join WB_REF_WS_DECK d
                  on l.idn=P_ID and
                     l.DECK_ID=d.id
                  order by l.section,
                           d.name) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into R_COUNT
        from WB_REF_WS_AIR_CABIN_FD
        where idn=P_ID;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("fd_data", xmlattributes(e.fcl_name "fcl_name",
                                                                e.fcl_id as "fcl_id",
                                                                  e.MAX_NUM_SEATS as "MAX_NUM_SEATS",
                                                                    e.LA_CENTROID as "LA_CENTROID",
                                                                      e.BA_CENTROID as "BA_CENTROID",
                                                                        e.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT"))).getClobVal() into cXML_data
            from (select d.name fcl_name,
                           to_char(d.id) fcl_id,
                             case when l.MAX_NUM_SEATS is null then 'NULL' else to_char(l.MAX_NUM_SEATS) end MAX_NUM_SEATS,
                               case when l.LA_CENTROID is null then 'NULL' else to_char(l.LA_CENTROID) end LA_CENTROID,
                                 case when l.BA_CENTROID is null then 'NULL' else to_char(l.BA_CENTROID) end BA_CENTROID,
                                   case when l.INDEX_PER_WT_UNIT is null then 'NULL' else to_char(l.INDEX_PER_WT_UNIT) end INDEX_PER_WT_UNIT
                  from WB_REF_WS_AIR_CABIN_FD l join WB_REF_WS_FL_CREW_LOCATION d
                  on l.idn=P_ID and
                     l.FCL_ID=d.id
                  order by d.name) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into R_COUNT
        from WB_REF_WS_AIR_CABIN_CCL
        where idn=P_ID;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("ccl_data", xmlattributes(e.deck_name "deck_name",
                                                                e.deck_id as "deck_id",
                                                                  e.location as "location",
                                                                    e.MAX_NUM_SEATS as "MAX_NUM_SEATS",
                                                                      e.LA_CENTROID as "LA_CENTROID",
                                                                        e.BA_CENTROID as "BA_CENTROID",
                                                                          e.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT"))).getClobVal() into cXML_data
            from (select d.name deck_name,
                           to_char(d.id) deck_id,
                             l.location,
                               case when l.MAX_NUM_SEATS is null then 'NULL' else to_char(l.MAX_NUM_SEATS) end MAX_NUM_SEATS,
                                 case when l.LA_CENTROID is null then 'NULL' else to_char(l.LA_CENTROID) end LA_CENTROID,
                                   case when l.BA_CENTROID is null then 'NULL' else to_char(l.BA_CENTROID) end BA_CENTROID,
                                     case when l.INDEX_PER_WT_UNIT is null then 'NULL' else to_char(l.INDEX_PER_WT_UNIT) end INDEX_PER_WT_UNIT
                  from WB_REF_WS_AIR_CABIN_CCL l join  WB_REF_WS_DECK d
                  on l.idn=P_ID and
                     l.DECK_ID=d.id
                  order by d.name,
                           l.location) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        cXML_out:=cXML_out||'</root>';
     end;
    end if;


end SP_WB_REF_WS_AIR_CABIN_DATA;
/
