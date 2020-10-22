create or replace PROCEDURE SP_WB_AIRCO_AHM_TREE(cXML_in IN VARCHAR2,
                                                   cXML_out OUT CLOB)
AS
cXML_data CLOB:='';
Lang_NAME varchar2(100);
AIRCO_ID number:=-1;
V_ID_WS number;
V_MAX_ID number;
V_WS_PAR_ID number;
V_WS_BORT_PAR_ID number;
V_WS_PAR_LEVEL number;
  BEGIN
     cXML_out:=to_clob('<?xml version="1.0" ?><root>');

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into LANG_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/AIRCO_ID[1]')) into AIRCO_ID from dual;

     ------------------------------------------------------------------------------------------------------------
     ------------------------------------áÄèéãçüÖå ÑÖêÖÇé--------------------------------------------------------
     insert into WB_REF_TMP_AIRCOMPANY_AHM_TREE (ID,
	                                                 PARENT_ID,
	                                                   TITLE_RUS,
                                                     	 TITLE_ENG,
                                                 	       SORT_PRIOR_RUS,
                                                 	         SORT_PRIOR_ENG,
                                                 	           LEVEL_TREE,
                                                 	             TAB_INDEX,
                                                                 SHOW_INFO,
                                                                   FUNCTIONS,
                                                                     DATA_BLOCK,
                                                                       FACT_DB_ID,
                                                                         WS_DB_ID)
     select ID,
	            PARENT_ID,
	              TITLE_RUS,
                  TITLE_ENG,
                    SORT_PRIOR_RUS,
                      SORT_PRIOR_ENG,
                        LEVEL_TREE,
                          TAB_INDEX,
                            SHOW_INFO,
                              FUNCTIONS,
                                'AIRLINE',
                                  0,
                                    0
     from WB_REF_AIRCOMPANY_AHM_TREE;


     if AIRCO_ID<>-1 then
       begin
         update WB_REF_TMP_AIRCOMPANY_AHM_TREE
         set TITLE_RUS=(select i.NAME_RUS_full
                        from WB_REF_AIRCOMPANY_ADV_INFO i
                        where i.id_ac=AIRCO_ID and
                              i.date_from=nvl((select max(ii.date_from)
                                               from WB_REF_AIRCOMPANY_ADV_INFO ii
                                               where ii.id_ac=i.id_ac and
                                                     ii.date_from<=sysdate()), (select min(ii.date_from)
                                                                                from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                                where ii.id_ac=i.id_ac and
                                                                                      ii.date_from>sysdate()))),
             TITLE_eng=(select NAME_ENG_full
                        from WB_REF_AIRCOMPANY_ADV_INFO i
                        where i.id_ac=AIRCO_ID and
                              i.date_from=nvl((select max(ii.date_from)
                                               from WB_REF_AIRCOMPANY_ADV_INFO ii
                                               where ii.id_ac=i.id_ac and
                                                     ii.date_from<=sysdate()), (select min(ii.date_from)
                                                                                from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                                where ii.id_ac=i.id_ac and
                                                                                      ii.date_from>sysdate())))
         where parent_id=-1;


         select id,
                level_tree into V_WS_PAR_ID, V_WS_PAR_LEVEL
         from WB_REF_AIRCOMPANY_AHM_TREE
         where "FUNCTIONS" like '%WS_ADD%';

         for i in (select a.id,
                          a.id_ws
                   from WB_REF_AIRCO_WS_TYPES a join WB_REF_WS_TYPES ws
                   on a.id_ac=AIRCO_ID and
                      a.id_ws=ws.id
                   order by case when LANG_NAME='RUS'
                                 then ws.NAME_RUS_FULL
                                 else ws.NAME_ENG_FULL
                            end)
           loop
             begin
               select max(id)+1 into V_MAX_ID
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE;

               insert into WB_REF_TMP_AIRCOMPANY_AHM_TREE (ID,
	                                                           PARENT_ID,
	                                                             TITLE_RUS,
                                                               	 TITLE_ENG,
                                                   	               SORT_PRIOR_RUS,
                                                   	                 SORT_PRIOR_ENG,
                                                   	                   LEVEL_TREE,
                                                   	                     TAB_INDEX,
                                                                           SHOW_INFO,
                                                                             FUNCTIONS,
                                                                               DATA_BLOCK,
                                                                                 FACT_DB_ID,
                                                                                   WS_DB_ID)
               select distinct ah.id+V_MAX_ID,

                               case when ah.parent_id=-1
                                    then V_WS_PAR_ID
                                    else ah.parent_id+V_MAX_ID
                               end parent_id,

                               case when ah.parent_id=-1
                                    then ws.NAME_RUS_FULL
                                    else ah.title_rus
                               end title_rus,
                               case when ah.parent_id=-1
                                    then ws.NAME_ENG_FULL
                                    else ah.title_eng
                               end title_eng,
                               ah.SORT_PRIOR_RUS,
                               ah.SORT_PRIOR_ENG,
                               ah.LEVEL_TREE+2,
                               ah.TAB_INDEX,
                               ah.SHOW_INFO,
                               ah.FUNCTIONS,
                               'AIRLINE_AIRCRAFT',
                               i.id,
                               ws.id
               from WB_REF_WS_TYPES ws join WB_REF_WS_AHM_TREE ah
               on ws.id=i.id_ws;

               -----------------------------------------------------------------
               -------------SEATING LAYOUT--------------------------------------
               select max(id)+1 into V_MAX_ID
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE;

               insert into WB_REF_TMP_AIRCOMPANY_AHM_TREE (ID,
	                                                           PARENT_ID,
	                                                             TITLE_RUS,
                                                               	 TITLE_ENG,
                                                   	               SORT_PRIOR_RUS,
                                                   	                 SORT_PRIOR_ENG,
                                                   	                   LEVEL_TREE,
                                                   	                     TAB_INDEX,
                                                                           SHOW_INFO,
                                                                             FUNCTIONS,
                                                                               DATA_BLOCK,
                                                                                 FACT_DB_ID,
                                                                                   WS_DB_ID)
               select distinct sl.id+V_MAX_ID,
                               ah.id,

                               (select q.TABLE_NAME
                                from (select a.TABLE_NAME
                                      from WB_REF_WS_AIR_SEAT_LAY_ADV a
                                      where a.idn=sl.id
                                      order by a.date_from desc) q
                                WHERE ROWNUM<2)/*||'id_ac='||to_char(sl.id_ac)||'id_ws'||to_char(sl.id_ws)||'id='||to_char(sl.id)*/,

                              (select q.TABLE_NAME
                                from (select a.TABLE_NAME
                                      from WB_REF_WS_AIR_SEAT_LAY_ADV a
                                      where a.idn=sl.id
                                      order by a.date_from desc) q
                                WHERE ROWNUM<2)/*||'id_ac='||to_char(sl.id_ac)||'id_ws'||to_char(sl.id_ws)||'id='||to_char(sl.id)*/,

                               1,
                               1,

                               ah.LEVEL_TREE+1,
                               20,
                               1, --SHOW_INFO,
                               'WS_SEATING_LAYOUT_CONFIGURATION_INS',
                               'AIRLINE_AIRCRAFT',
                               sl.id,
                               i.id_ws
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE ah join WB_REF_WS_AIR_SEAT_LAY_IDN sl
               on ah.data_block='AIRLINE_AIRCRAFT' and
                  ah.WS_DB_ID=i.id_ws and
                  ah.TAB_INDEX=19 and
                  sl.id_ac=AIRCO_ID and
                  sl.id_ws=ah.WS_DB_ID and
                  sl.id_bort=-1
               order by (select q.TABLE_NAME
                         from (select a.TABLE_NAME
                               from WB_REF_WS_AIR_SEAT_LAY_ADV a
                               where a.idn=sl.id
                               order by a.date_from desc) q
                         WHERE ROWNUM<2);
               -------------SEATING LAYOUT--------------------------------------
               -----------------------------------------------------------------

               select max(id)+1 into V_MAX_ID
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE;

               select min(t.id) into V_WS_BORT_PAR_ID
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE t
               where t.FACT_DB_ID=i.id and
                     t.level_tree=(select min(tt.level_tree)
                                   from WB_REF_TMP_AIRCOMPANY_AHM_TREE tt
                                   where tt.FACT_DB_ID=t.FACT_DB_ID);

               insert into WB_REF_TMP_AIRCOMPANY_AHM_TREE (ID,
	                                                           PARENT_ID,
	                                                             TITLE_RUS,
                                                               	 TITLE_ENG,
                                                   	               SORT_PRIOR_RUS,
                                                   	                 SORT_PRIOR_ENG,
                                                   	                   LEVEL_TREE,
                                                   	                     TAB_INDEX,
                                                                           SHOW_INFO,
                                                                             FUNCTIONS,
                                                                               DATA_BLOCK,
                                                                                 FACT_DB_ID,
                                                                                   WS_DB_ID)
               select distinct ah.id+V_MAX_ID,

                               case when ah.parent_id=-1
                                    then V_WS_BORT_PAR_ID
                                    else ah.parent_id+V_MAX_ID
                               end parent_id,
                               ah.title_rus,
                               ah.title_eng,
                               ah.SORT_PRIOR_RUS,
                               ah.SORT_PRIOR_ENG,
                               ah.LEVEL_TREE+3,
                               ah.TAB_INDEX,
                               ah.SHOW_INFO,
                               ah.FUNCTIONS,
                               'AIRLINE_AIRCRAFT',
                               i.id,
                               ws.id
               from WB_REF_WS_TYPES ws join WB_REF_WS_BORT_AHM_TREE ah
               on ws.id=i.id_ws;

           end;
         end loop;

       end;
     end if;
     ------------------------------------áÄèéãçüÖå ÑÖêÖÇé--------------------------------------------------------
     ------------------------------------------------------------------------------------------------------------

     SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.id "id",
                                                      e.PARENT_ID "PARENT_ID",
                                                        e.TITLE_RUS "TITLE_RUS",
                                                          e.TITLE_ENG "TITLE_ENG",
                                                            e.LEVEL_TREE "LEVEL",
                                                              e.TAB_INDEX "TAB_INDEX",
                                                                e.SHOW_INFO "SHOW_INFO",
                                                                  e.FUNCTIONS "FUNCTIONS",
                                                                    e.DATA_BLOCK "DATA_BLOCK",
                                                                      e.FACT_DB_ID "FACT_DB_ID",
                                                                        e.ws_db_id "ws_db_id"))).getClobVal() into cXML_data
     from (select ID,
	                  PARENT_ID,
	                    TITLE_RUS,
                        TITLE_ENG,
                          LEVEL_TREE,
                            TAB_INDEX,
                              SHOW_INFO,
                                FUNCTIONS,
                                  DATA_BLOCK,
                                    FACT_DB_ID,
                                      WS_DB_ID
           from  WB_REF_TMP_AIRCOMPANY_AHM_TREE
           order by parent_id,
                    case when LANG_NAME='ENG' then SORT_PRIOR_ENG else SORT_PRIOR_RUS end,
                    case when LANG_NAME='ENG' then TITLE_ENG else TITLE_RUS end) e;



    if cXML_data is not null then
      begin
        cXML_out:=cXML_out||cXML_data;

      end;
    end if;

    cXML_out:=cXML_out||'</root>';

    commit;
  END SP_WB_AIRCO_AHM_TREE;
/
