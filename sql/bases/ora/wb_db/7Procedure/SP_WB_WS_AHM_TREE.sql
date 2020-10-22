create or replace PROCEDURE SP_WB_WS_AHM_TREE(cXML_in IN VARCHAR2,
                                                cXML_out OUT CLOB)
AS
cXML_data CLOB:='';
Lang_NAME varchar2(100);
WS_ID number:=-1;
V_MAX_ID number;
  BEGIN
     cXML_out:=to_clob('<?xml version="1.0" ?><root>');

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into LANG_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/WS_ID[1]')) into WS_ID from dual;

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
                                                                       FACT_DB_ID)
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
                                'AIRCRAFT',
                                  -1
     from WB_REF_WS_AHM_TREE;


     if WS_ID<>-1 then
       begin
         update WB_REF_TMP_AIRCOMPANY_AHM_TREE
         set TITLE_RUS=(select i.NAME_RUS_full
                        from WB_REF_WS_TYPES i
                        where i.id=WS_ID),
             TITLE_eng=(select NAME_ENG_full
                        from WB_REF_WS_TYPES i
                        where i.id=WS_ID)
         where parent_id=-1;

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
                                                                            FACT_DB_ID)
         select distinct sl.id+V_MAX_ID,
                               ah.id,

                               (select q.TABLE_NAME
                                from (select a.TABLE_NAME
                                      from WB_REF_WS_AIR_SEAT_LAY_ADV a
                                      where a.idn=sl.id
                                      order by a.date_from desc) q
                                WHERE ROWNUM<2),

                              (select q.TABLE_NAME
                                from (select a.TABLE_NAME
                                      from WB_REF_WS_AIR_SEAT_LAY_ADV a
                                      where a.idn=sl.id
                                      order by a.date_from desc) q
                                WHERE ROWNUM<2),

                               1,
                               1,

                               ah.LEVEL_TREE+1,
                               20,
                               1, --SHOW_INFO,
                               'WS_SEATING_LAYOUT_CONFIGURATION_INS',
                               'AIRCRAFT',
                               sl.id
               from WB_REF_TMP_AIRCOMPANY_AHM_TREE ah join WB_REF_WS_AIR_SEAT_LAY_IDN sl
               on ah.data_block='AIRCRAFT' and
                  ah.TAB_INDEX=19 and
                  sl.id_ac=-1 and
                  sl.id_ws=WS_ID and
                  sl.id_bort=-1
               order by (select q.TABLE_NAME
                         from (select a.TABLE_NAME
                               from WB_REF_WS_AIR_SEAT_LAY_ADV a
                               where a.idn=sl.id
                               order by a.date_from desc) q
                         WHERE ROWNUM<2);
         -------------SEATING LAYOUT--------------------------------------
         -----------------------------------------------------------------

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
                                                                      e.FACT_DB_ID "FACT_DB_ID"))).getClobVal() into cXML_data
     from (select ID,
	                  PARENT_ID,
	                    TITLE_RUS,
                        TITLE_ENG,
                          LEVEL_TREE,
                            TAB_INDEX,
                              SHOW_INFO,
                                FUNCTIONS,
                                  DATA_BLOCK,
                                    FACT_DB_ID
           from  WB_REF_TMP_AIRCOMPANY_AHM_TREE
           order by parent_id,
                    case when LANG_NAME='ENG' then SORT_PRIOR_ENG else SORT_PRIOR_RUS end,
                    case when LANG_NAME='ENG' then TITLE_ENG else TITLE_RUS end ) e;


    if cXML_data is not null then
      begin
        cXML_out:=cXML_out||cXML_data;

      end;
    end if;

    cXML_out:=cXML_out||'</root>';

    commit;
  END SP_WB_WS_AHM_TREE;
/
