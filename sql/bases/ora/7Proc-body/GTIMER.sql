create or replace PACKAGE BODY gtimer
AS

TYPE TClientStagePermits IS TABLE OF trip_ckin_client.pr_permit%TYPE INDEX BY BINARY_INTEGER;
TYPE TStagesInStageType IS TABLE OF BOOLEAN INDEX BY BINARY_INTEGER;
TYPE TStageStatuses IS TABLE OF TStagesInStageType INDEX BY BINARY_INTEGER;
stage_statuses_t TStageStatuses;

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
          trip_final_stages.point_id(+)=vpoint_id;
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
