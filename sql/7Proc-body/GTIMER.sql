create or replace PACKAGE BODY gtimer
AS

TYPE TClientStagePermits IS TABLE OF trip_ckin_client.pr_permit%TYPE INDEX BY BINARY_INTEGER;
TYPE TStagesInStageType IS TABLE OF BOOLEAN INDEX BY BINARY_INTEGER;
TYPE TStageStatuses IS TABLE OF TStagesInStageType INDEX BY BINARY_INTEGER;
stage_statuses_t TStageStatuses;

PROCEDURE puttrip_stages( vpoint_id IN points.point_id%TYPE )
IS
CURSOR cur_stages(vairp		stage_sets.airp%TYPE) IS
  SELECT graph_stages.stage_id,
         NVL(stage_names.name,graph_stages.name) name,
         NVL(stage_sets.pr_auto,graph_stages.pr_auto) pr_auto
   FROM graph_stages, stage_sets, stage_names
  WHERE graph_stages.stage_id > 0 AND graph_stages.stage_id < 99 AND
        stage_sets.airp(+) = vairp AND
        stage_names.airp(+) = vairp AND
        stage_sets.stage_id(+) = graph_stages.stage_id AND
        stage_names.stage_id(+) = graph_stages.stage_id
   ORDER BY stage_id;
vrow_stages 	cur_stages%ROWTYPE;
CURSOR cur_times(vstage_id	graph_times.stage_id%TYPE,
                 vairline	graph_times.airline%TYPE,
                 vairp		graph_times.airp%TYPE,
                 vcraft		graph_times.craft%TYPE,
                 vtrip_type	graph_times.trip_type%TYPE) IS
  SELECT time,
         DECODE( graph_times.airline, NULL, 0, 8 ) +
         DECODE( graph_times.airp, NULL, 0, 4 ) +
         DECODE( graph_times.craft, NULL, 0, 2 ) +
         DECODE( graph_times.trip_type, NULL, 0, 1 ) AS priority
  FROM graph_times
  WHERE stage_id = vstage_id AND
        ( graph_times.airline IS NULL OR graph_times.airline = vairline ) AND
        ( graph_times.airp IS NULL OR graph_times.airp = vairp ) AND
        ( graph_times.craft IS NULL OR graph_times.craft = vcraft ) AND
        ( graph_times.trip_type IS NULL OR graph_times.trip_type = vtrip_type )
  UNION
  SELECT time, -1 AS priority
  FROM graph_stages
  WHERE stage_id = vstage_id
  ORDER BY 2/*priority*/ DESC;
vrow_times 	cur_times%ROWTYPE;
vc 		NUMBER;
vscd		points.scd_out%TYPE;
vact		points.act_out%TYPE;
vpr_del		points.pr_del%TYPE;
vairp		points.airp%TYPE;
vairline	points.airline%TYPE;
vcraft		points.craft%TYPE;
vtrip_type	points.trip_type%TYPE;
vtime           trip_stages.scd%TYPE;
vignore_auto	trip_stages.ignore_auto%TYPE;
BEGIN
 BEGIN
   SELECT scd_out,airline,airp,craft,trip_type,act_out,points.pr_del
   INTO vscd,vairline,vairp,vcraft,vtrip_type,vact,vpr_del
   FROM points, trip_types
   WHERE points.point_id = vpoint_id AND points.pr_del>=0 AND
         points.trip_type = trip_types.code AND
         trip_types.pr_reg = 1;
 EXCEPTION
   WHEN NO_DATA_FOUND THEN RETURN;
 END;
 FOR vrow_stages IN cur_stages( vairp ) LOOP
  SELECT COUNT(*) INTO vc FROM trip_stages
   WHERE point_id = vpoint_id AND
         stage_id = vrow_stages.stage_id;
  IF vc = 0 THEN
   OPEN cur_times(vrow_stages.stage_id,vairline,vairp,vcraft,vtrip_type);
   FETCH cur_times INTO vrow_times;
   IF cur_times%FOUND THEN
     vtime:=TRUNC(vscd-vrow_times.time/1440,'MI');
     IF vact IS NULL AND vpr_del = 0 THEN
       vignore_auto := 0;
     ELSE
       vignore_auto := 1;
     END IF;
     INSERT INTO trip_stages( point_id,stage_id,scd,est,act,pr_auto,pr_manual,ignore_auto )
     VALUES( vpoint_id,vrow_stages.stage_id,vtime,NULL,NULL,vrow_stages.pr_auto,0,vignore_auto );
     system.MsgToLog('Этап '''||vrow_stages.name||''': план. время='||
                     TO_CHAR(vtime,'HH24:MI DD.MM.YY')||' (UTC)',system.evtGraph,vpoint_id);
   END IF;
   CLOSE cur_times;
  END IF;
 END LOOP;
 sync_trip_final_stages( vpoint_id );
END puttrip_stages;

FUNCTION IsClientStage( vpoint_id IN points.point_id%TYPE,
                        vstage_id IN trip_stages.stage_id%TYPE,
                        vpr_permit OUT trip_ckin_client.pr_permit%TYPE ) RETURN NUMBER
IS
  CURSOR cur IS
   SELECT NVL(pr_permit,0) pr_permit
    FROM ckin_client_stages, trip_ckin_client
    WHERE point_id(+)=vpoint_id AND
          ckin_client_stages.client_type=trip_ckin_client.client_type(+) AND
          stage_id=vstage_id
    ORDER BY 1 DESC;
curRow		cur%ROWTYPE;
vres NUMBER;
BEGIN
 OPEN cur;
 FETCH cur INTO curRow;
 IF cur%FOUND THEN -- это не клиентский шаг
   vpr_permit := curRow.pr_permit;
   vres := 1;
 ELSE
   vpr_permit := 0;
   vres := 0;
 END IF;
 CLOSE cur;
 RETURN vres;
END IsClientStage;

FUNCTION getCurrStage( vpoint_id   IN trip_final_stages.point_id%TYPE,
                       vstage_type IN trip_final_stages.stage_type%TYPE,
                       client_stage_permits IN TClientStagePermits) RETURN trip_final_stages.stage_id%TYPE
IS
  CURSOR cur IS
   SELECT target_stage,level
    FROM
     (SELECT target_stage,cond_stage,act
      	FROM graph_rules,trip_stages
      WHERE trip_stages.stage_id=graph_rules.target_stage AND
            point_id=vpoint_id AND next=1)
    START WITH cond_stage=0 AND act IS NOT NULL
    CONNECT BY PRIOR target_stage = cond_stage AND act IS NOT NULL;

  CURSOR cur2 IS
    SELECT stage_id, stage_type FROM stage_statuses;
curRow          cur%ROWTYPE;
vlevel          BINARY_INTEGER;
vstage_id       trip_stages.stage_id%TYPE;
vpr_permit	trip_ckin_client.pr_permit%TYPE;
BEGIN
  IF stage_statuses_t.FIRST IS NULL THEN
    -- пустая таблица: надо наполнить
    FOR cur2Row IN cur2 LOOP
      stage_statuses_t(cur2Row.stage_type)(cur2Row.stage_id):=TRUE;
    END LOOP;
  END IF;


  vstage_id:=0;
  vlevel:=0;
  OPEN cur;
  FETCH cur INTO curRow;
  WHILE cur%FOUND LOOP
    BEGIN
      vpr_permit := client_stage_permits( curRow.target_stage );
    EXCEPTION
      WHEN NO_DATA_FOUND THEN vpr_permit := 1;
    END;
    IF vpr_permit<>0 THEN
      EXIT WHEN vlevel>=curRow.level;
      BEGIN
        IF stage_statuses_t(vstage_type)(curRow.target_stage) THEN
          vstage_id:=curRow.target_stage;
          vlevel:=curRow.level;
        END IF;
      EXCEPTION
         WHEN NO_DATA_FOUND THEN NULL;
      END;
    END IF;
    FETCH cur INTO curRow;
  END LOOP;
  CLOSE cur;
  RETURN vstage_id;
END getCurrstage;

PROCEDURE sync_trip_final_stages( vpoint_id IN trip_stages.point_id%TYPE )
IS
  CURSOR cur IS
    SELECT stage_types.id, trip_final_stages.stage_id
    FROM stage_types, trip_final_stages
    WHERE stage_types.id=trip_final_stages.stage_type(+) AND
          trip_final_stages.point_id(+)=vpoint_id
    FOR UPDATE;
  CURSOR cur2 IS
    SELECT stage_id FROM graph_stages;
vpr_permit	      trip_ckin_client.pr_permit%TYPE;
client_stage_permits  TClientStagePermits;
vstage_id             trip_final_stages.stage_id%TYPE;
BEGIN
  FOR cur2Row IN cur2 LOOP
    IF IsClientStage( vpoint_id, cur2Row.stage_id, vpr_permit ) <> 0 THEN
      client_stage_permits(cur2Row.stage_id):=vpr_permit;
    END IF;
  END LOOP;

  FOR curRow IN cur LOOP
    vstage_id:=getCurrstage( vpoint_id, curRow.id, client_stage_permits );
    IF curRow.stage_id IS NULL THEN
      INSERT INTO trip_final_stages(point_id,stage_type,stage_id)
      VALUES(vpoint_id, curRow.id, vstage_id);
    ELSE
      IF vstage_id<>curRow.stage_id THEN
        UPDATE trip_final_stages SET stage_id=vstage_id
        WHERE point_id=vpoint_id AND stage_type=curRow.id;
      END IF;
    END IF;
  END LOOP;

END sync_trip_final_stages;

FUNCTION ExecStage( vpoint_id IN points.point_id%TYPE,
                    vstage_id IN trip_stages.stage_id%TYPE,
                    vact OUT trip_stages.act%TYPE ) RETURN NUMBER
IS
CURSOR cur_cond( vpoint_id trip_stages.point_id%TYPE,
                 vstage_id trip_stages.stage_id%TYPE )
IS
SELECT cond_stage, num, DECODE(cond_stage,0,system.UTCSYSDATE,99,NULL,act) act
 FROM trip_stages,
 (SELECT cond_stage, num
   FROM trip_stages, graph_rules
  WHERE trip_stages.point_id = vpoint_id AND
        trip_stages.stage_id = vstage_id AND
        graph_rules.target_stage = vstage_id AND
        graph_rules.next = 1)
WHERE trip_stages.point_id(+) = vpoint_id AND
      trip_stages.stage_id(+) = cond_stage
ORDER BY num, act DESC;
vrow_cond cur_cond%ROWTYPE;
vpr_permit trip_ckin_client.pr_permit%TYPE;
vres NUMBER;
old_num graph_rules.num%TYPE := NULL;
BEGIN
 BEGIN
   SELECT pr_del INTO vres FROM points WHERE point_id=vpoint_id AND pr_del=0;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN RETURN 0;
 END;
 vres := 1;
 FOR vrow_cond IN cur_cond( vpoint_id, vstage_id ) LOOP
  IF old_num IS NULL OR old_num != vrow_cond.num THEN
   EXIT WHEN vres = 1 AND old_num IS NOT NULL;
   vres := 1;
   old_num := vrow_cond.num;
  END IF;
  IF vres = 1 AND vrow_cond.act IS NULL THEN -- нет факта выполнения, проверка на web-шаг
   IF IsClientStage( vpoint_id, vrow_cond.cond_stage, vpr_permit ) = 0 OR vpr_permit != 0 THEN
    vres := 0;
   END IF;
  END IF;
 END LOOP;
 IF vres!=0 THEN
  vact := TRUNC( system.UTCSYSDATE, 'MI' );
  UPDATE trip_stages SET act = vact
   WHERE point_id = vpoint_id AND stage_id = vstage_id;
  IF SQL%NOTFOUND THEN
   INSERT INTO trip_stages(point_id,stage_id,scd,est,act,pr_auto,pr_manual)
    VALUES(vpoint_id,vstage_id,vact,NULL,vact,0,1);
  END IF;
  sync_trip_final_stages(vpoint_id);
 END IF;
 RETURN vres;
END ExecStage;

FUNCTION get_stage_time( vpoint_id IN trip_stages.point_id%TYPE,
                         vstage_id IN trip_stages.stage_id%TYPE ) RETURN DATE
IS
res	DATE;
BEGIN
  SELECT NVL( act, NVL( est, scd ) ) INTO res FROM trip_stages
   WHERE point_id=vpoint_id AND stage_id=vstage_id;
  RETURN res;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END get_stage_time;

END gtimer;
/
