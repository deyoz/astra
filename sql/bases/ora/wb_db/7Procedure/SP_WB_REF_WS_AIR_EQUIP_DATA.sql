create or replace procedure SP_WB_REF_WS_AIR_EQUIP_DATA
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
    from WB_REF_WS_AIR_EQUIP_ADV
    where idn=P_ID;

    if R_COUNT>0 then
      begin
         cXML_out:='<?xml version="1.0" ?><root>';

        select '<adv_data TABLE_NAME="'||a.TABLE_NAME||'" '||
	                       'PWL_BALANCE_ARM="'||to_char(i.PWL_BALANCE_ARM)||'" '||
	                       'PWL_INDEX_UNIT="'||to_char(i.PWL_INDEX_UNIT)||'" '||
	                       'GOL_BALANCE_ARM="'||to_char(i.GOL_BALANCE_ARM)||'" '||
	                       'GOL_INDEX_UNIT="'||to_char(i.GOL_INDEX_UNIT)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
           FROM WB_REF_WS_AIR_EQUIP_ADV i join WB_REF_WS_AIR_EQUIP_IDN a
           on i.IDN=a.id and
              a.id=P_ID;

        cXML_out:=cXML_out||cXML_data;

        select count(id) into R_COUNT
        from WB_REF_WS_AIR_EQUIP_PWL
        where idn=P_ID;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("pwl_data", xmlattributes(e.tank_name "tank_name",
                                                                 e.MAX_WEIGHT "MAX_WEIGHT",
                                                                   e.L_CENTROID as "L_CENTROID",
                                                                     e.BA_CENTROID as "BA_CENTROID",
                                                                       e.BA_FWD as "BA_FWD",
                                                                         e.BA_AFT as "BA_AFT",
                                                                           e.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT",
                                                                             e.SHOW_ON_PLAN as "SHOW_ON_PLAN"))).getClobVal() into cXML_data
            from (select l.tank_name,
                           case when l.MAX_WEIGHT is null then 'NULL' else to_char(l.MAX_WEIGHT) end MAX_WEIGHT,
                             case when l.L_CENTROID is null then 'NULL' else to_char(l.L_CENTROID) end L_CENTROID,
                               case when l.BA_CENTROID is null then 'NULL' else to_char(l.BA_CENTROID) end BA_CENTROID,
                                 case when l.BA_FWD is null then 'NULL' else to_char(l.BA_FWD) end BA_FWD,
                                   case when l.BA_AFT is null then 'NULL' else to_char(l.BA_AFT) end BA_AFT,
                                     case when l.INDEX_PER_WT_UNIT is null then 'NULL' else to_char(l.INDEX_PER_WT_UNIT) end INDEX_PER_WT_UNIT,
                                       case when l.SHOW_ON_PLAN is null then 'NULL' else to_char(l.SHOW_ON_PLAN) end SHOW_ON_PLAN
                  from WB_REF_WS_AIR_EQUIP_PWL l
                  where l.idn=P_ID
                  order by l.tank_name) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(id) into R_COUNT
        from WB_REF_WS_AIR_EQUIP_GOL
        where idn=P_ID;

        if R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("gol_data", xmlattributes(e.description "description",
                                                                 e.deck_name as "DECK_NAME",
                                                                   e.deck_id as "DECK_ID",
                                                                     e.TYPE_name as "TYPE_NAME",
                                                                       e.TYPE_id as "TYPE_ID",
                                                                         e.MAX_WEIGHT "MAX_WEIGHT",
                                                                           e.LA_CENTROID as "LA_CENTROID",
                                                                             e.LA_FROM as "LA_FROM",
                                                                               e.LA_TO as "LA_TO",
                                                                                 e.BA_CENTROID as "BA_CENTROID",
                                                                                   e.BA_FWD as "BA_FWD",
                                                                                     e.BA_AFT as "BA_AFT",
                                                                                       e.INDEX_PER_WT_UNIT as "INDEX_PER_WT_UNIT",
                                                                                         e.SHOW_ON_PLAN as "SHOW_ON_PLAN"))).getClobVal() into cXML_data
            from (select l.description,
                           d.name DECK_NAME,
                             to_char(l.deck_id) DECK_ID,
                               g.name TYPE_NAME,
                                 to_char(l.type_id) TYPE_ID,
                                   case when l.MAX_WEIGHT is null then 'NULL' else to_char(l.MAX_WEIGHT) end MAX_WEIGHT,
                                     case when l.LA_CENTROID is null then 'NULL' else to_char(l.LA_CENTROID) end LA_CENTROID,
                                       case when l.LA_FROM is null then 'NULL' else to_char(l.LA_FROM) end LA_FROM,
                                         case when l.LA_TO is null then 'NULL' else to_char(l.LA_TO) end LA_TO,
                                           case when l.BA_CENTROID is null then 'NULL' else to_char(l.BA_CENTROID) end BA_CENTROID,
                                             case when l.BA_FWD is null then 'NULL' else to_char(l.BA_FWD) end BA_FWD,
                                               case when l.BA_AFT is null then 'NULL' else to_char(l.BA_AFT) end BA_AFT,
                                                 case when l.INDEX_PER_WT_UNIT is null then 'NULL' else to_char(l.INDEX_PER_WT_UNIT) end INDEX_PER_WT_UNIT,
                                                   case when l.SHOW_ON_PLAN is null then 'NULL' else to_char(l.SHOW_ON_PLAN) end SHOW_ON_PLAN
                  from WB_REF_WS_AIR_EQUIP_GOL l join WB_REF_WS_DECK d
                  on l.idn=P_ID and
                     l.deck_id=d.id join WB_REF_WS_GOL_LOCATIONS g
                     on g.id=l.type_id
                  order by g.name,
                           d.name,
                           l.description) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        ------------------------------------------------------------------------
        ------------------------èêàÇüáäÄ ä ÅéêíÄå-------------------------------
        select count(b.id) into R_COUNT
        from WB_REF_WS_AIR_EQUIP_ADV a join WB_REF_AIRCO_WS_BORTS b
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
                                              from WB_REF_WS_AIR_EQUIP_BORT cb
                                              where cb.id_bort=b.id and
                                                    cb.idn=a.idn)
                                  then '1'
                                  else '0'
                              end is_checked
                  from WB_REF_WS_AIR_EQUIP_ADV a join WB_REF_AIRCO_WS_BORTS b
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


end SP_WB_REF_WS_AIR_EQUIP_DATA;
/
