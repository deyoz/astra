create or replace PACKAGE BODY sopp
AS

FUNCTION get_birks(vpoint_id 	IN points.point_id%TYPE,
                   vlang	IN lang_types.code%TYPE) RETURN VARCHAR2
IS
res	VARCHAR2(1000);
cur ckin.birks_cursor_ref;
BEGIN
  OPEN cur FOR
    /* Только бирки тех багажных пулов, все пассажиры которых не прошли посадку */
    SELECT tag_type,no_len,
           tag_colors.code AS color,
           DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
           TRUNC(no/1000) AS first,
           MOD(no,1000) AS last,
           no
    FROM pax_grp,bag2,bag_tags,tag_types,tag_colors
    WHERE pax_grp.grp_id=bag2.grp_id AND
          bag2.grp_id=bag_tags.grp_id AND
          bag2.num=bag_tags.bag_num AND
          bag_tags.tag_type=tag_types.code AND
          bag_tags.color=tag_colors.code(+) AND
          pax_grp.point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          ckin.bag_pool_refused(bag2.grp_id, bag2.bag_pool_num, pax_grp.class, pax_grp.bag_refuse)=0 AND
          ckin.bag_pool_boarded(bag2.grp_id, bag2.bag_pool_num, pax_grp.class, pax_grp.bag_refuse)=0
    UNION
    SELECT tag_type,no_len,
           tag_colors.code AS color,
           DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
           TRUNC(no/1000) AS first,
           MOD(no,1000) AS last,
           no
    FROM pax_grp,bag_tags,tag_types,tag_colors
    WHERE pax_grp.grp_id=bag_tags.grp_id AND
          bag_tags.bag_num IS NULL AND
          bag_tags.tag_type=tag_types.code AND
          bag_tags.color=tag_colors.code(+) AND
          pax_grp.point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          ckin.bag_pool_refused(bag_tags.grp_id, 1, pax_grp.class, pax_grp.bag_refuse)=0 AND
          ckin.bag_pool_boarded(bag_tags.grp_id, 1, pax_grp.class, pax_grp.bag_refuse)=0
    ORDER BY tag_type,color,no;
  res:=ckin.build_birks_str(cur);
  CLOSE cur;
  RETURN res;
END get_birks;

PROCEDURE set_flight_sets(vpoint_id IN points.point_id%TYPE,
                          use_seances      IN NUMBER,
                          vf IN trip_sets.f%TYPE DEFAULT 0,
                          vc IN trip_sets.f%TYPE DEFAULT 0,
                          vy IN trip_sets.f%TYPE DEFAULT 0 )
IS
CURSOR CKIN_SETSCur(vairline        points.airline%TYPE,
                   vflt_no 	   points.flt_no%TYPE,
                   vairp_dep       points.airp%TYPE) IS
  SELECT client_types.code AS client_type,
         ckin_client_sets.desk_grp_id,
         NVL(ckin_client_sets.pr_permit,0) AS pr_permit,
         NVL(ckin_client_sets.pr_waitlist,0) AS pr_waitlist,
         NVL(ckin_client_sets.pr_tckin,0) AS pr_tckin,
         NVL(ckin_client_sets.pr_upd_stage,0) AS pr_upd_stage,
         NVL(ckin_client_sets.priority,0) AS priority
  FROM client_types,
   (SELECT client_type,
           desk_grp_id,
           pr_permit,
           pr_waitlist,
           pr_tckin,
           pr_upd_stage,
           DECODE(airline,NULL,0,8)+
           DECODE(flt_no,NULL,0,2)+
           DECODE(airp_dep,NULL,0,4) AS priority
    FROM ckin_client_sets
    WHERE (airline IS NULL OR airline=vairline) AND
          (flt_no IS NULL OR flt_no=vflt_no) AND
          (airp_dep IS NULL OR airp_dep=vairp_dep)) ckin_client_sets
  WHERE client_types.code=ckin_client_sets.client_type(+) AND code<>'TERM'
  ORDER BY client_types.code,ckin_client_sets.desk_grp_id,priority DESC;
CKIN_SETSCurRow CKIN_SETSCur%ROWTYPE;
vairline points.airline%TYPE;
vairp_dep points.airp%TYPE;
vflt_no points.flt_no%TYPE;
prev_client_type client_types.code%TYPE;
prev_grp_id ckin_client_sets.desk_grp_id%TYPE;
msg events.msg%TYPE;
BEGIN
  BEGIN
    SELECT airline,flt_no,airp
     INTO vairline,vflt_no,vairp_dep
    FROM points WHERE point_id=vpoint_id AND pr_del>=0 FOR UPDATE;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN RETURN;
  END;

 INSERT INTO trip_sets(point_id,f,c,y,max_commerce,pr_etstatus,pr_stat,
                       pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam,pr_check_pay,
                       pr_exam_check_pay,pr_reg_with_tkn,pr_reg_with_doc,crc_comp,
		       pr_basel_stat,auto_weighing,pr_free_seating)
  VALUES(vpoint_id,vf,vc,vy, NULL, 0, 0, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);

 prev_client_type:=NULL;
 prev_grp_id:=NULL;
 FOR CKIN_SETSCurRow IN CKIN_SETSCur(vairline,vflt_no,vairp_dep) LOOP
   IF prev_client_type=CKIN_SETSCurRow.client_type AND
      (prev_grp_id=CKIN_SETSCurRow.desk_grp_id OR prev_grp_id IS NULL AND CKIN_SETSCurRow.desk_grp_id IS NULL) OR
      CKIN_SETSCurRow.client_type='KIOSK' AND CKIN_SETSCurRow.desk_grp_id IS NULL OR
      CKIN_SETSCurRow.client_type='WEB' AND CKIN_SETSCurRow.desk_grp_id IS NOT NULL THEN
     NULL;
   ELSE
       INSERT INTO trip_ckin_client(point_id,client_type,pr_permit,
                                    pr_waitlist,pr_tckin,pr_upd_stage,desk_grp_id)
       VALUES(vpoint_id,CKIN_SETSCurRow.client_type,CKIN_SETSCurRow.pr_permit,
              CKIN_SETSCurRow.pr_waitlist,CKIN_SETSCurRow.pr_tckin,CKIN_SETSCurRow.pr_upd_stage,CKIN_SETSCurRow.desk_grp_id);

       SELECT 'На рейсе'||DECODE(CKIN_SETSCurRow.pr_permit,0,' запрещена',' разрешена')||
              DECODE(CKIN_SETSCurRow.client_type,'WEB',' web-регистрация',' регистрация')
       INTO msg FROM dual;
       IF CKIN_SETSCurRow.desk_grp_id IS NOT NULL THEN
         SELECT msg||DECODE(CKIN_SETSCurRow.client_type,'WEB',' для группы пультов ''',' для группы киосков ''')||
                descr||''''
         INTO msg FROM desk_grp WHERE grp_id=CKIN_SETSCurRow.desk_grp_id;
       END IF;

       IF CKIN_SETSCurRow.pr_permit<>0 THEN
         SELECT msg||' с параметрами: '||
--                'лист ожидания: '||DECODE(CKIN_SETSCurRow.pr_waitlist,0,'нет','да')||', '||
                'сквоз. рег.: '||DECODE(CKIN_SETSCurRow.pr_tckin,0,'нет','да')||', '||
                'перерасч. времен: '||DECODE(CKIN_SETSCurRow.pr_upd_stage,0,'нет','да')
         INTO msg FROM dual;
       END IF;
       system.MsgToLog(msg,system.evtFlt,vpoint_id);
     prev_client_type:=CKIN_SETSCurRow.client_type;
     prev_grp_id:=CKIN_SETSCurRow.desk_grp_id;
   END IF;
 END LOOP;
 ckin.set_trip_sets(vpoint_id,use_seances);
 gtimer.puttrip_stages(vpoint_id);
END set_flight_sets;

END sopp;
/
