create or replace PACKAGE BODY arch
AS

BULK_COLLECT_LIMIT CONSTANT INTEGER := 100;

FUNCTION get_main_pax_id2(vpart_key       IN arx_pax_grp.part_key%TYPE,
                          vgrp_id         IN arx_pax_grp.grp_id%TYPE,
                          include_refused IN NUMBER DEFAULT 1) RETURN arx_pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM arx_pax
    WHERE part_key=vpart_key AND grp_id=vgrp_id
    ORDER BY DECODE(bag_pool_num,NULL,1,0),
             DECODE(pers_type,'��',0,'��',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'��',0,'��',1,2),
             reg_no;
curRow	cur%ROWTYPE;
res	arx_pax.pax_id%TYPE;
BEGIN
  res:=NULL;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    res:=curRow.pax_id;
    IF include_refused=0 AND curRow.refuse IS NOT NULL THEN res:=NULL; END IF;
  END IF;
  CLOSE cur;
  RETURN res;
END get_main_pax_id2;

FUNCTION get_bag_pool_pax_id(vpart_key     IN arx_pax.part_key%TYPE,
                             vgrp_id       IN arx_pax.grp_id%TYPE,
                             vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                             include_refused IN NUMBER DEFAULT 1) RETURN arx_pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM arx_pax
    WHERE part_key=vpart_key AND grp_id=vgrp_id AND bag_pool_num=vbag_pool_num
    ORDER BY DECODE(pers_type,'��',0,'��',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'��',0,'��',1,2),
             reg_no;
curRow	cur%ROWTYPE;
res	arx_pax.pax_id%TYPE;
BEGIN
  IF vbag_pool_num IS NULL THEN RETURN NULL; END IF;
  res:=NULL;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    res:=curRow.pax_id;
    IF include_refused=0 AND curRow.refuse IS NOT NULL THEN res:=NULL; END IF;
  END IF;
  CLOSE cur;
  RETURN res;
END get_bag_pool_pax_id;

FUNCTION bag_pool_refused(vpart_key     IN arx_bag2.part_key%TYPE,
                          vgrp_id       IN arx_bag2.grp_id%TYPE,
                          vbag_pool_num IN arx_bag2.bag_pool_num%TYPE,
                          vclass        IN arx_pax_grp.class%TYPE,
                          vbag_refuse   IN arx_pax_grp.bag_refuse%TYPE) RETURN NUMBER
IS
n NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 1; END IF;
  IF vclass IS NULL THEN RETURN 0; END IF;
  SELECT SUM(DECODE(refuse,NULL,1,0)) INTO n FROM arx_pax
  WHERE part_key=vpart_key AND grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
  IF n IS NULL THEN
    RETURN 1;
--    raise_application_error(-20000,'bag_pool_refused: Data integrity is broken (part_key='||TO_CHAR(vpart_key,'DD.MM.YY HH24:MI:SS')||', grp_id='||vgrp_id||', bag_pool_num='||vbag_pool_num||')');
  END IF;
  IF n>0 THEN RETURN 0; ELSE RETURN 1; END IF;
END bag_pool_refused;

FUNCTION get_birks2(vpart_key     IN arx_pax.part_key%TYPE,
                    vgrp_id       IN arx_pax.grp_id%TYPE,
                    vpax_id 	    IN arx_pax.pax_id%TYPE,
                    vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                    pr_lat        IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
BEGIN
  IF pr_lat<>0 THEN
    RETURN get_birks2(vpart_key, vgrp_id, vpax_id, vbag_pool_num, '');
  ELSE
    RETURN get_birks2(vpart_key, vgrp_id, vpax_id, vbag_pool_num, 'RU');
  END IF;
END get_birks2;

FUNCTION get_birks2(vpart_key     IN arx_pax.part_key%TYPE,
                    vgrp_id       IN arx_pax.grp_id%TYPE,
                    vpax_id 	    IN arx_pax.pax_id%TYPE,
                    vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                    vlang	        IN lang_types.code%TYPE) RETURN VARCHAR2
IS
res	VARCHAR2(4000);
pool_pax_id    arx_pax.pax_id%TYPE;
cur            ckin.birks_cursor_ref;
BEGIN
  res:=NULL;
  IF vpax_id IS NOT NULL THEN
    IF vbag_pool_num IS NULL THEN RETURN res; END IF;
    pool_pax_id:=get_bag_pool_pax_id(vpart_key,vgrp_id,vbag_pool_num);
  END IF;
  IF vpax_id IS NULL OR
     pool_pax_id IS NOT NULL AND pool_pax_id=vpax_id THEN
    IF vpax_id IS NULL THEN
      OPEN cur FOR
        SELECT tag_type,no_len,
               tag_colors.code AS color,
               DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
               TRUNC(no/1000) AS first,
      	       MOD(no,1000) AS last,
               no
        FROM arx_bag_tags,tag_types,tag_colors
        WHERE arx_bag_tags.tag_type=tag_types.code AND
              arx_bag_tags.color=tag_colors.code(+) AND
              part_key=vpart_key AND grp_id=vgrp_id
        ORDER BY tag_type,color,no;
    ELSE
      IF vbag_pool_num=1 THEN
        /*��� �� ��㯯 ����� ॣ����஢����� � �ନ���� ��� ��易⥫쭮� �ਢ離� */
        OPEN cur FOR
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM arx_bag2,arx_bag_tags,tag_types,tag_colors
          WHERE arx_bag2.part_key=arx_bag_tags.part_key AND
                arx_bag2.grp_id=arx_bag_tags.grp_id AND
                arx_bag2.num=arx_bag_tags.bag_num AND
                arx_bag_tags.tag_type=tag_types.code AND
                arx_bag_tags.color=tag_colors.code(+) AND
                arx_bag2.part_key=vpart_key AND
                arx_bag2.grp_id=vgrp_id AND
                arx_bag2.bag_pool_num=vbag_pool_num
          UNION
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM arx_bag_tags,tag_types,tag_colors
          WHERE arx_bag_tags.tag_type=tag_types.code AND
                arx_bag_tags.color=tag_colors.code(+) AND
                arx_bag_tags.part_key=vpart_key AND
                arx_bag_tags.grp_id=vgrp_id AND
                arx_bag_tags.bag_num IS NULL
          ORDER BY tag_type,color,no;
      ELSE
        OPEN cur FOR
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM arx_bag2,arx_bag_tags,tag_types,tag_colors
          WHERE arx_bag2.part_key=arx_bag_tags.part_key AND
                arx_bag2.grp_id=arx_bag_tags.grp_id AND
                arx_bag2.num=arx_bag_tags.bag_num AND
                arx_bag_tags.tag_type=tag_types.code AND
                arx_bag_tags.color=tag_colors.code(+) AND
                arx_bag2.part_key=vpart_key AND
                arx_bag2.grp_id=vgrp_id AND
                arx_bag2.bag_pool_num=vbag_pool_num
          ORDER BY tag_type,color,no;
      END IF;
    END IF;
    res:=ckin.build_birks_str(cur);
    CLOSE cur;
  END IF;
  RETURN res;
END get_birks2;

FUNCTION get_bagInfo2(vpart_key     IN arx_pax.part_key%TYPE,
                      vgrp_id       IN arx_pax.grp_id%TYPE,
                      vpax_id 	    IN arx_pax.pax_id%TYPE,
                      vbag_pool_num IN arx_pax.bag_pool_num%TYPE) RETURN TBagInfo
IS
bagInfo		TBagInfo;
pool_pax_id    arx_pax.pax_id%TYPE;
BEGIN
  bagInfo.bagAmount:=NULL;
  bagInfo.bagWeight:=NULL;
  bagInfo.rkAmount:=NULL;
  bagInfo.rkWeight:=NULL;
  IF vpax_id IS NOT NULL THEN
    IF vbag_pool_num IS NULL THEN RETURN bagInfo; END IF;
    pool_pax_id:=get_bag_pool_pax_id(vpart_key,vgrp_id,vbag_pool_num);
  END IF;
  IF vpax_id IS NULL OR
     pool_pax_id IS NOT NULL AND pool_pax_id=vpax_id THEN
    IF vpax_id IS NULL THEN
      SELECT SUM(DECODE(pr_cabin,0,amount,NULL)) AS bagAmount,
             SUM(DECODE(pr_cabin,0,weight,NULL)) AS bagWeight,
             SUM(DECODE(pr_cabin,0,NULL,amount)) AS rkAmount,
             SUM(DECODE(pr_cabin,0,NULL,weight)) AS rkWeight
      INTO bagInfo.bagAmount,bagInfo.bagWeight,bagInfo.rkAmount,bagInfo.rkWeight
      FROM arx_bag2
      WHERE part_key=vpart_key AND grp_id=vgrp_id;
    ELSE
      SELECT SUM(DECODE(pr_cabin,0,amount,NULL)) AS bagAmount,
             SUM(DECODE(pr_cabin,0,weight,NULL)) AS bagWeight,
             SUM(DECODE(pr_cabin,0,NULL,amount)) AS rkAmount,
             SUM(DECODE(pr_cabin,0,NULL,weight)) AS rkWeight
      INTO bagInfo.bagAmount,bagInfo.bagWeight,bagInfo.rkAmount,bagInfo.rkWeight
      FROM arx_bag2
      WHERE part_key=vpart_key AND grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
    END IF;
  END IF;
  bagInfo.grp_id:=vgrp_id;
  bagInfo.pax_id:=vpax_id;
  RETURN bagInfo;
END get_bagInfo2;

FUNCTION get_bagAmount2(vpart_key     IN arx_pax.part_key%TYPE,
                        vgrp_id       IN arx_pax.grp_id%TYPE,
                        vpax_id 	    IN arx_pax.pax_id%TYPE,
                        vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vpart_key,vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.bagAmount;
END get_bagAmount2;

FUNCTION get_bagWeight2(vpart_key     IN arx_pax.part_key%TYPE,
                        vgrp_id       IN arx_pax.grp_id%TYPE,
                        vpax_id 	    IN arx_pax.pax_id%TYPE,
                        vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vpart_key,vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.bagWeight;
END get_bagWeight2;

FUNCTION get_rkAmount2(vpart_key     IN arx_pax.part_key%TYPE,
                       vgrp_id       IN arx_pax.grp_id%TYPE,
                       vpax_id 	     IN arx_pax.pax_id%TYPE,
                       vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vpart_key,vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.rkAmount;
END get_rkAmount2;

FUNCTION get_rkWeight2(vpart_key     IN arx_pax.part_key%TYPE,
                       vgrp_id       IN arx_pax.grp_id%TYPE,
                       vpax_id  	   IN arx_pax.pax_id%TYPE,
                       vbag_pool_num IN arx_pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vpart_key,vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.rkWeight;
END get_rkWeight2;

FUNCTION get_excess_wt(vpart_key       IN arx_pax.part_key%TYPE,
                       vgrp_id         IN arx_pax.grp_id%TYPE,
                       vpax_id         IN arx_pax.pax_id%TYPE,
                       vexcess_wt      IN arx_pax_grp.excess_wt%TYPE DEFAULT NULL,
                       vexcess_nvl     IN arx_pax_grp.excess%TYPE DEFAULT NULL,
                       vbag_refuse     IN arx_pax_grp.bag_refuse%TYPE DEFAULT NULL) RETURN NUMBER

IS
vexcess         arx_pax_grp.excess_wt%TYPE;
main_pax_id     arx_pax.pax_id%TYPE;
BEGIN
  vexcess:=0;

  IF NVL(vexcess_wt, vexcess_nvl) IS NULL OR vbag_refuse IS NULL THEN
    BEGIN
      SELECT DECODE(bag_refuse, 0, NVL(excess_wt, excess), 0)
      INTO vexcess
      FROM arx_pax_grp
      WHERE part_key=vpart_key AND grp_id=vgrp_id;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN RETURN NULL;
    END;
  ELSE
    SELECT DECODE(vbag_refuse, 0, NVL(vexcess_wt, vexcess_nvl), 0)
    INTO vexcess
    FROM dual;
  END IF;

  IF vpax_id IS NOT NULL THEN
    main_pax_id:=get_main_pax_id2(vpart_key, vgrp_id);
  END IF;
  IF vpax_id IS NULL OR
     main_pax_id IS NOT NULL AND main_pax_id=vpax_id THEN
    NULL;
  ELSE
    vexcess:=NULL;
  END IF;

  IF vexcess=0 THEN vexcess:=NULL; END IF;
  RETURN vexcess;
END get_excess_wt;

FUNCTION next_airp(vpart_key     IN arx_points.part_key%TYPE,
                   vfirst_point  IN arx_points.first_point%TYPE,
                   vpoint_num    IN arx_points.point_num%TYPE) RETURN arx_points.airp%TYPE
IS
CURSOR cur IS
  SELECT airp FROM arx_points
  WHERE part_key=vpart_key AND
        first_point=vfirst_point AND point_num>vpoint_num AND pr_del=0
  ORDER BY point_num;
vairp	arx_points.airp%TYPE;
BEGIN
  vairp:=NULL;
  OPEN cur;
  FETCH cur INTO vairp;
  CLOSE cur;
  RETURN vairp;
END next_airp;

PROCEDURE move(vmove_id  points.move_id%TYPE,
               vpart_key DATE,
               vdate_range move_arx_ext.date_range%TYPE)
IS
  CURSOR cur IS
    SELECT point_id,pr_del,pr_reg FROM points WHERE move_id=vmove_id FOR UPDATE;

  CURSOR curAODBPax(vpoint_id       points.point_id%TYPE) IS
    SELECT pax_id,point_addr FROM aodb_pax WHERE point_id=vpoint_id;

  CURSOR langCur IS
    SELECT code AS lang FROM lang_types;

n       INTEGER;
pax_count INTEGER;

i       BINARY_INTEGER;
j       BINARY_INTEGER;
k       BINARY_INTEGER;
rowids     TRowidsTable;
grprowids  TRowidsTable;
paxrowids  TRowidsTable;
miscrowids TRowidsTable;

grpids   TIdsTable;
paxids   TIdsTable;
miscids  TIdsTable;
miscids2 TIdsTable;

use_insert BOOLEAN;
use_move_insert BOOLEAN;
BEGIN
  /*��稬*/
  UPDATE points SET point_id=point_id WHERE move_id=vmove_id;

  INSERT INTO arx_points
    (point_id,move_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,
     craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,
     park_in,park_out,remark,pr_reg,pr_del,
     airp_fmt,airline_fmt,craft_fmt,suffix_fmt,
     tid,part_key)
  SELECT
     point_id,move_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,
     craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,
     park_in,park_out,remark,pr_reg,pr_del,
     airp_fmt,airline_fmt,craft_fmt,suffix_fmt,
     tid,vpart_key
  FROM points
  WHERE move_id=vmove_id AND pr_del<>-1;

  use_move_insert:=SQL%FOUND;

  IF use_move_insert THEN
    IF vdate_range IS NOT NULL THEN
      INSERT INTO move_arx_ext(part_key,move_id,date_range)
      VALUES(vpart_key,vmove_id,vdate_range);
    END IF;

    INSERT INTO arx_move_ref
      (move_id,reference,part_key)
    SELECT
       move_id,reference,vpart_key
    FROM move_ref
    WHERE move_id=vmove_id;
  END IF;

  FOR langCurRow IN langCur LOOP
    SELECT rowid BULK COLLECT INTO rowids
    FROM events_bilingual
    WHERE lang=langCurRow.lang AND type IN (system.evtDisp) AND id1=vmove_id FOR UPDATE;

    IF use_move_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_events
          (type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,part_key,part_num,lang)
        SELECT
           type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,vpart_key,part_num,lang
        FROM events_bilingual
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM events_bilingual WHERE rowid=rowids(i);
  END LOOP;

  FOR curRow IN cur LOOP
    pax_count:=0;
    use_insert:=curRow.pr_del<>-1;

    FOR langCurRow IN langCur LOOP
      SELECT rowid BULK COLLECT INTO rowids
      FROM events_bilingual
      WHERE lang=langCurRow.lang AND type IN (system.evtFlt,
                                              system.evtGraph,
                                              system.evtFltTask,
                                              system.evtPax,
                                              system.evtPay,
                                              system.evtTlg,
                                              system.evtPrn) AND id1=curRow.point_id FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_events
            (type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,part_key,part_num,lang)
          SELECT
             type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,vpart_key,part_num,lang
          FROM events_bilingual
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM events_bilingual WHERE rowid=rowids(i);
    END LOOP;

    SELECT rowid,grp_id BULK COLLECT INTO grprowids,grpids
    FROM pax_grp
    WHERE point_dep=curRow.point_id FOR UPDATE;
    SELECT DISTINCT point_id_mark BULK COLLECT INTO miscids
    FROM pax_grp
    WHERE point_dep=curRow.point_id;
    IF use_insert THEN
      FOR i IN 1..miscids.COUNT LOOP
        BEGIN
          INSERT INTO arx_mark_trips
            (point_id,airline,flt_no,suffix,scd,airp_dep,part_key)
          SELECT
             point_id,airline,flt_no,suffix,scd,airp_dep,vpart_key
          FROM mark_trips
          WHERE point_id=miscids(i);
        EXCEPTION
          WHEN DUP_VAL_ON_INDEX THEN NULL;
        END;
      END LOOP;

      FORALL i IN 1..grprowids.COUNT
        INSERT INTO arx_pax_grp
          (grp_id,point_dep,point_arv,airp_dep,airp_arv,class,class_grp,
           status,excess_wt,excess_pc,hall,bag_refuse,user_id,client_type,point_id_mark,pr_mark_norms,
           piece_concept,desk,time_create,tid,part_key)
        SELECT
           grp_id,point_dep,point_arv,airp_dep,airp_arv,class,class_grp,
           status,excess_wt,excess_pc,hall,bag_refuse,user_id,client_type,point_id_mark,pr_mark_norms,
           piece_concept,desk,time_create,tid,vpart_key
        FROM pax_grp
        WHERE rowid=grprowids(i);
    END IF;

    SELECT rowid BULK COLLECT INTO rowids
    FROM self_ckin_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_self_ckin_stat
          (point_id, client_type, desk, desk_airp, descr, adult, child, baby, tckin,
           term_bp, term_bag, term_ckin_service, part_key)
        SELECT
           point_id, client_type, desk, desk_airp, descr, adult, child, baby, tckin,
           term_bp, term_bag, term_ckin_service, vpart_key
        FROM self_ckin_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM self_ckin_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM rfisc_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_rfisc_stat
           (point_id, pr_trfer, trfer_airline, trfer_flt_no, trfer_suffix, trfer_airp_arv, trfer_scd,
            point_num, airp_arv, reg_no, ticket_no, coupon_no, rfisc, excess, paid, tag_no, fqt_no, travel_time, user_login, user_descr, desk, time_create, part_key)
        SELECT
           point_id, pr_trfer, trfer_airline, trfer_flt_no, trfer_suffix, trfer_airp_arv, trfer_scd,
            point_num, airp_arv, reg_no, ticket_no, coupon_no, rfisc, excess, paid, tag_no, fqt_no, travel_time, user_login, user_descr, desk, time_create, vpart_key
        FROM rfisc_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM rfisc_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_services
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_services
        (point_id, scd_out, pax_id, airp_dep, airp_arv, rfic, rfisc, receipt_no, part_key)
        SELECT
         point_id, scd_out, pax_id, airp_dep, airp_arv, rfic, rfisc, receipt_no, vpart_key
        FROM stat_services
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_services WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_rem
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_rem
        (point_id, travel_time, rem_code, ticket_no, airp_last, user_id, desk, rfisc, rate, rate_cur, part_key)
        SELECT
         point_id, travel_time, rem_code, ticket_no, airp_last, user_id, desk, rfisc, rate, rate_cur, vpart_key
        FROM stat_rem
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_rem WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM limited_capability_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_limited_capability_stat
        (point_id, airp_arv, rem_code, pax_amount, part_key)
        SELECT
         point_id, airp_arv, rem_code, pax_amount, vpart_key
        FROM limited_capability_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM limited_capability_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM pfs_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_pfs_stat
        (point_id, pax_id, status, airp_arv, seats, subcls, pnr, surname, name, gender, birth_date, part_key)
        SELECT
         point_id, pax_id, status, airp_arv, seats, subcls, pnr, surname, name, gender, birth_date, vpart_key
        FROM pfs_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM pfs_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_ad
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_ad
        (scd_out, point_id, pax_id, pnr, class, client_type, bag_amount, bag_weight, seat_no, seat_no_lat, desk, station, part_key)
        SELECT
        scd_out, point_id, pax_id, pnr, class, client_type, bag_amount, bag_weight, seat_no, seat_no_lat, desk, station, vpart_key
        FROM stat_ad
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_ad WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_ha
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_ha
        (point_id, hotel_id, room_type, scd_out, adt, chd, inf, part_key)
        SELECT
         point_id, hotel_id, room_type, scd_out, adt, chd, inf, vpart_key
        FROM stat_ha
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_ha WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_vo
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_vo
        (point_id, voucher, scd_out, amount, part_key)
        SELECT
         point_id, voucher, scd_out, amount, vpart_key
        FROM stat_vo
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_vo WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat_reprint
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat_reprint
        (point_id, scd_out, desk, ckin_type, amount, part_key)
        SELECT
         point_id, scd_out, desk, ckin_type, amount, vpart_key
        FROM stat_reprint
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat_reprint WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM trfer_pax_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_trfer_pax_stat
        (point_id, scd_out, pax_id, rk_weight, bag_weight, bag_amount, segments, part_key)
        SELECT
         point_id, scd_out, pax_id, rk_weight, bag_weight, bag_amount, segments, vpart_key
        FROM trfer_pax_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM trfer_pax_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM bi_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_bi_stat
        (point_id, scd_out, pax_id, print_type, terminal, hall, op_type, pr_print, time_print, desk, part_key)
        SELECT
         point_id, scd_out, pax_id, print_type, terminal, hall, op_type, pr_print, time_print, desk, vpart_key
        FROM bi_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM bi_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM agent_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_agent_stat
          (point_id, point_part_key, user_id, desk, ondate, pax_time, pax_amount,
           dpax_amount, dtckin_amount, dbag_amount, dbag_weight, drk_amount, drk_weight, part_key)
        SELECT
           point_id, vpart_key, user_id, desk, ondate, pax_time, pax_amount,
           dpax_amount, dtckin_amount, dbag_amount, dbag_weight, drk_amount, drk_weight, ondate
        FROM agent_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM agent_stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_stat
          (point_id,airp_arv,hall,status,client_type,f,c,y,adult,child,baby,child_wop,baby_wop,
           pcs,weight,unchecked,excess_wt,excess_pc,term_bp,term_bag,term_ckin_service,part_key)
        SELECT
           point_id,airp_arv,hall,status,client_type,f,c,y,adult,child,baby,child_wop,baby_wop,
           pcs,weight,unchecked,excess_wt,excess_pc,term_bp,term_bag,term_ckin_service,vpart_key
        FROM stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM stat WHERE rowid=rowids(i);

    SELECT rowid BULK COLLECT INTO rowids
    FROM trfer_stat
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_trfer_stat
          (point_id,trfer_route,client_type,f,c,y,adult,child,baby,child_wop,baby_wop,
           pcs,weight,unchecked,excess_wt,excess_pc,part_key)
        SELECT
           point_id,trfer_route,client_type,f,c,y,adult,child,baby,child_wop,baby_wop,
           pcs,weight,unchecked,excess_wt,excess_pc,vpart_key
        FROM trfer_stat
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM trfer_stat WHERE rowid=rowids(i);

    SELECT rowid,id BULK COLLECT INTO rowids,miscids
    FROM tlg_out
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_tlg_out
          (id,num,type,point_id,addr,origin,heading,body,ending,pr_lat,completed,has_errors,
           time_create,time_send_scd,time_send_act,originator_id,airline_mark,manual_creation,
           part_key)
        SELECT
           id,num,type,point_id,addr,origin,heading,body,ending,pr_lat,completed,has_errors,
           time_create,time_send_scd,time_send_act,originator_id,airline_mark,manual_creation,
           NVL(time_send_act,NVL(time_send_scd,time_create))
        FROM tlg_out
        WHERE rowid=rowids(i) AND type<>'LCI';
    END IF;
    FORALL i IN 1..miscids.COUNT
      DELETE FROM typeb_out_extra WHERE tlg_id=miscids(i);
    FORALL i IN 1..miscids.COUNT
      DELETE FROM typeb_out_errors WHERE tlg_id=miscids(i);
    FORALL i IN 1..rowids.COUNT
      DELETE FROM tlg_out WHERE rowid=rowids(i);

    IF use_insert THEN
      INSERT INTO arx_trip_classes
        (point_id,class,cfg,block,prot,part_key)
      SELECT
         point_id,class,cfg,block,prot,vpart_key
      FROM trip_classes
      WHERE point_id=curRow.point_id;

      INSERT INTO arx_trip_delays
        (point_id,delay_num,delay_code,time,part_key)
      SELECT
         point_id,delay_num,delay_code,time,vpart_key
      FROM trip_delays
      WHERE point_id=curRow.point_id;

      INSERT INTO arx_trip_load
        (point_dep,point_arv,airp_dep,airp_arv,cargo,mail,part_key)
      SELECT
         point_dep,point_arv,airp_dep,airp_arv,cargo,mail,vpart_key
      FROM trip_load
      WHERE point_dep=curRow.point_id;

      INSERT INTO arx_trip_sets
        (point_id,max_commerce,comp_id,pr_etstatus,pr_stat,pr_tranz_reg,f,c,y,part_key)
      SELECT
         point_id,max_commerce,comp_id,pr_etstatus,pr_stat,pr_tranz_reg,f,c,y,vpart_key
      FROM trip_sets
      WHERE point_id=curRow.point_id;

      INSERT INTO arx_crs_displace2
        (point_id_spp,airp_arv_spp,class_spp,airline,flt_no,suffix,scd,airp_dep,
         point_id_tlg,airp_arv_tlg,class_tlg,status,part_key)
      SELECT
         point_id_spp,airp_arv_spp,class_spp,airline,flt_no,suffix,scd,airp_dep,
         NULL,        airp_arv_tlg,class_tlg,status,vpart_key
      FROM crs_displace2
      WHERE point_id_spp=curRow.point_id;
    END IF;

    SELECT rowid BULK COLLECT INTO rowids
    FROM trip_stages
    WHERE point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_trip_stages
          (point_id,stage_id,scd,est,act,pr_auto,pr_manual,part_key)
        SELECT
           point_id,stage_id,scd,est,act,pr_auto,pr_manual,vpart_key
        FROM trip_stages
        WHERE rowid=rowids(i);
    END IF;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM trip_stages WHERE rowid=rowids(i);

    SELECT bag_receipts.rowid,bag_receipts.receipt_id,bag_rcpt_kits.kit_id
    BULK COLLECT INTO rowids,miscids,miscids2
    FROM bag_receipts, bag_rcpt_kits
    WHERE bag_receipts.kit_id=bag_rcpt_kits.kit_id(+) AND
          bag_receipts.kit_num=bag_rcpt_kits.kit_num(+) AND
          bag_receipts.point_id=curRow.point_id FOR UPDATE;
    IF use_insert THEN
      FORALL i IN 1..rowids.COUNT
        INSERT INTO arx_bag_receipts
            (receipt_id,point_id,grp_id,status,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name,
             tickets,prev_no,airline,aircode,flt_no,suffix,scd_local_date,airp_dep,airp_arv,ex_amount,ex_weight,value_tax,
             rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,remarks,
             issue_date,issue_place,issue_user_id,issue_desk,annul_date,annul_user_id,annul_desk,is_inter,desk_lang,
             kit_id,kit_num,part_key)
        SELECT
             receipt_id,point_id,grp_id,status,form_type,no,pax_name,pax_doc,service_type,bag_type,bag_name,
             tickets,prev_no,airline,aircode,flt_no,suffix,scd_local_date,airp_dep,airp_arv,ex_amount,ex_weight,value_tax,
             rate,rate_cur,exch_rate,exch_pay_rate,pay_rate_cur,remarks,
             issue_date,issue_place,issue_user_id,issue_desk,annul_date,annul_user_id,annul_desk,is_inter,desk_lang,
             kit_id,kit_num,vpart_key
        FROM bag_receipts
        WHERE rowid=rowids(i);
    END IF;
    FOR j IN 1..miscids.COUNT LOOP
      IF use_insert THEN
        INSERT INTO arx_bag_pay_types
            (receipt_id,num,pay_type,pay_rate_sum,extra,part_key)
        SELECT
             receipt_id,num,pay_type,pay_rate_sum,extra,vpart_key
        FROM bag_pay_types
        WHERE receipt_id=miscids(j);
      END IF;
      DELETE FROM bag_pay_types WHERE receipt_id=miscids(j);
    END LOOP;
    FORALL i IN 1..rowids.COUNT
      DELETE FROM bag_receipts WHERE rowid=rowids(i);

    FOR i IN 1..miscids2.COUNT LOOP
      IF miscids2(i) IS NOT NULL THEN
        DELETE FROM bag_rcpt_kits WHERE kit_id=miscids2(i);
      END IF;
    END LOOP;

    FOR j IN 1..grpids.COUNT LOOP
      /* ======================横� �� ��㯯�� ���ᠦ�஢========================= */

      SELECT id BULK COLLECT INTO miscids
      FROM annul_bag
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..miscids.COUNT
          INSERT INTO arx_annul_bag
          (id, grp_id, pax_id, bag_type, rfisc, time_create, time_annul, amount,
          weight, user_id, part_key)
          SELECT
          id, grp_id, pax_id, bag_type, rfisc, time_create, time_annul, amount,
          weight, user_id, vpart_key
          FROM annul_bag
          WHERE id=miscids(i);
        forall i in 1..miscids.count
          insert into arx_annul_tags (id, no, part_key)
          select id, no, vpart_key from annul_tags where id = miscids(i);
      END IF;
      forall i in 1..miscids.count
        delete from annul_tags where id = miscids(i);
      FORALL i IN 1..miscids.count
        DELETE FROM annul_bag WHERE id=miscids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM unaccomp_bag_info
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_unaccomp_bag_info
            (original_tag_no, surname, name, airline, flt_no, suffix, scd, grp_id, num, part_key)
          select
            original_tag_no, surname, name, airline, flt_no, suffix, scd, grp_id, num, vpart_key
          FROM unaccomp_bag_info
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM unaccomp_bag_info WHERE rowid=rowids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM bag2
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_bag2
            (grp_id,num,id,bag_type,rfisc,pr_cabin,amount,weight,value_bag_num,pr_liab_limit,to_ramp,
             using_scales,bag_pool_num,hall,user_id,is_trfer,handmade,desk,time_create,
             list_id, bag_type_str, service_type, airline, part_key)
          SELECT
             grp_id,num,id,bag_type,rfisc,pr_cabin,amount,weight,value_bag_num,pr_liab_limit,to_ramp,
             using_scales,bag_pool_num,hall,user_id,is_trfer,handmade,desk,time_create,
             list_id, bag_type_str, service_type, airline, vpart_key
          FROM bag2
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM bag2 WHERE rowid=rowids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM bag_prepay
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_bag_prepay
            (receipt_id,grp_id,no,aircode,ex_weight,bag_type,value,value_cur,part_key)
          SELECT
             receipt_id,grp_id,no,aircode,ex_weight,bag_type,value,value_cur,vpart_key
          FROM bag_prepay
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM bag_prepay WHERE rowid=rowids(i);


      SELECT rowid BULK COLLECT INTO rowids
      FROM bag_tags
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_bag_tags
            (grp_id,num,tag_type,no,color,bag_num,pr_print,part_key)
          SELECT
             grp_id,num,tag_type,no,color,bag_num,pr_print,vpart_key
          FROM bag_tags
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM bag_tags WHERE rowid=rowids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM paid_bag
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_paid_bag
            (grp_id,bag_type,weight,rate_id,rate_trfer,handmade,
             list_id, bag_type_str, airline, part_key)
          SELECT
             grp_id,bag_type,weight,rate_id,rate_trfer,handmade,
             list_id, bag_type_str, airline, vpart_key
          FROM paid_bag
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM paid_bag WHERE rowid=rowids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM pay_services
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_pay_services
            (grp_id,pax_id,transfer_num,rfisc,service_type,airline,
             name_view,name_view_lat,list_id,svc_id,doc_id,pass_id,seg_id,
             price,currency,ticknum,ticket_cpn,time_paid,order_id, part_key)
          SELECT
             grp_id,pax_id,transfer_num,rfisc,service_type,airline,
             name_view,name_view_lat,list_id,svc_id,doc_id,pass_id,seg_id,
             price,currency,ticknum,ticket_cpn,time_paid,order_id, vpart_key
          FROM pay_services
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM pay_services WHERE rowid=rowids(i);

      SELECT rowid,pax_id BULK COLLECT INTO paxrowids,paxids
      FROM pax
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..paxrowids.COUNT
          INSERT INTO arx_pax
            (pax_id,grp_id,surname,name,pers_type,is_female,seat_no,seat_type,seats,is_jmp,
             pr_brd,pr_exam,wl_type,refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm,doco_confirm,
             subclass,cabin_subclass,cabin_class,cabin_class_grp,bag_pool_num,
             excess_pc,
             tid,part_key)
          SELECT
             pax_id,grp_id,surname,name,pers_type,is_female,
             salons.get_seat_no(pax_id,seats,is_jmp,NULL,curRow.point_id,'one',rownum) AS seat_no, /*�� ��⨬��쭮. ���� ��।����� grp_status */
             seat_type,seats,is_jmp,
             pr_brd,pr_exam,wl_type,refuse,reg_no,ticket_no,coupon_no,ticket_rem,ticket_confirm,doco_confirm,
             subclass,cabin_subclass,cabin_class,cabin_class_grp,bag_pool_num,
             ckin.get_excess_pc(grp_id, pax_id),
             tid,vpart_key
          FROM pax
          WHERE rowid=paxrowids(i);
      END IF;

      SELECT transfer.rowid,trfer_trips.rowid BULK COLLECT INTO rowids,miscrowids
      FROM transfer,trfer_trips
      WHERE transfer.point_id_trfer=trfer_trips.point_id AND
            grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_transfer
            (grp_id,transfer_num,airline,airline_fmt,flt_no,suffix,suffix_fmt,scd,
             airp_dep,airp_dep_fmt,airp_arv,airp_arv_fmt,pr_final,piece_concept,part_key)
          SELECT
             grp_id,transfer_num,airline,airline_fmt,flt_no,suffix,suffix_fmt,scd,
             airp_dep,airp_dep_fmt,airp_arv,airp_arv_fmt,pr_final,piece_concept,vpart_key
          FROM transfer,trfer_trips
          WHERE transfer.rowid=rowids(i) AND transfer_num>0 AND
                trfer_trips.rowid=miscrowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM transfer WHERE rowid=rowids(i);
      FOR i IN 1..miscrowids.COUNT LOOP
        BEGIN
          DELETE FROM trfer_trips WHERE rowid=miscrowids(i);
        EXCEPTION
          WHEN OTHERS THEN
            IF SQLCODE=-02292 THEN NULL; ELSE RAISE; END IF;
        END;
      END LOOP;

      SELECT tckin_segments.rowid,trfer_trips.rowid BULK COLLECT INTO rowids,miscrowids
      FROM tckin_segments,trfer_trips
      WHERE tckin_segments.point_id_trfer=trfer_trips.point_id AND
            grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_tckin_segments
            (grp_id,seg_no,airline,flt_no,suffix,scd,
             airp_dep,airp_arv,pr_final,part_key)
          SELECT
             grp_id,seg_no,airline,flt_no,suffix,scd,
             airp_dep,airp_arv,pr_final,vpart_key
          FROM tckin_segments,trfer_trips
          WHERE tckin_segments.rowid=rowids(i) AND seg_no>0 AND
                trfer_trips.rowid=miscrowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM tckin_segments WHERE rowid=rowids(i);
      FOR i IN 1..miscrowids.COUNT LOOP
        BEGIN
          DELETE FROM trfer_trips WHERE rowid=miscrowids(i);
        EXCEPTION
          WHEN OTHERS THEN
            IF SQLCODE=-02292 THEN NULL; ELSE RAISE; END IF;
        END;
      END LOOP;

      SELECT rowid BULK COLLECT INTO rowids
      FROM value_bag
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_value_bag
            (grp_id,num,value,value_cur,tax_id,tax,tax_trfer,part_key)
          SELECT
             grp_id,num,value,value_cur,tax_id,tax,tax_trfer,vpart_key
          FROM value_bag
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM value_bag WHERE rowid=rowids(i);

      SELECT rowid BULK COLLECT INTO rowids
      FROM grp_norms
      WHERE grp_id=grpids(j) FOR UPDATE;
      IF use_insert THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_grp_norms
            (grp_id, bag_type, norm_id, norm_trfer, handmade,
             list_id, bag_type_str, airline, part_key)
          SELECT
             grp_id, bag_type, norm_id, norm_trfer, handmade,
             list_id, bag_type_str, airline, vpart_key
          FROM grp_norms
          WHERE rowid=rowids(i);
      END IF;
      FORALL i IN 1..rowids.COUNT
        DELETE FROM grp_norms WHERE rowid=rowids(i);

      FOR k IN 1..paxids.COUNT LOOP
        /* ======================横� �� ���ᠦ�ࠬ ��㯯�========================= */
        SELECT rowid BULK COLLECT INTO rowids
        FROM pax_norms
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_pax_norms
              (pax_id, bag_type, norm_id, norm_trfer, handmade,
               list_id, bag_type_str, airline, part_key)
            SELECT
               pax_id, bag_type, norm_id, norm_trfer, handmade,
               list_id, bag_type_str, airline, vpart_key
            FROM pax_norms
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM pax_norms WHERE rowid=rowids(i);

        SELECT rowid BULK COLLECT INTO rowids
        FROM pax_rem
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_pax_rem
              (pax_id,rem,rem_code,part_key)
            SELECT
               pax_id,rem,rem_code,vpart_key
            FROM pax_rem
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM pax_rem WHERE rowid=rowids(i);

        SELECT rowid BULK COLLECT INTO rowids
        FROM transfer_subcls
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_transfer_subcls
              (pax_id,transfer_num,subclass,subclass_fmt,part_key)
            SELECT
               pax_id,transfer_num,subclass,subclass_fmt,vpart_key
            FROM transfer_subcls
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM transfer_subcls WHERE rowid=rowids(i);

        SELECT rowid BULK COLLECT INTO rowids
        FROM pax_doc
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_pax_doc
              (pax_id,type,subtype,issue_country,no,nationality,birth_date,gender,expiry_date,
               surname,first_name,second_name,pr_multi,type_rcpt,scanned_attrs,part_key)
            SELECT
               pax_id,type,subtype,issue_country,no,nationality,birth_date,gender,expiry_date,
               surname,first_name,second_name,pr_multi,type_rcpt,scanned_attrs,vpart_key
            FROM pax_doc
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM pax_doc WHERE rowid=rowids(i);

        SELECT rowid BULK COLLECT INTO rowids
        FROM pax_doco
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_pax_doco
              (pax_id,birth_place,type,subtype,no,issue_place,issue_date,expiry_date,applic_country,scanned_attrs,part_key)
            SELECT
               pax_id,birth_place,type,subtype,no,issue_place,issue_date,expiry_date,applic_country,scanned_attrs,vpart_key
            FROM pax_doco
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM pax_doco WHERE rowid=rowids(i);

        SELECT rowid BULK COLLECT INTO rowids
        FROM pax_doca
        WHERE pax_id=paxids(k) FOR UPDATE;
        IF use_insert THEN
          FORALL i IN 1..rowids.COUNT
            INSERT INTO arx_pax_doca
              (pax_id,type,country,address,city,region,postal_code,part_key)
            SELECT
               pax_id,type,country,address,city,region,postal_code,vpart_key
            FROM pax_doca
            WHERE rowid=rowids(i);
        END IF;
        FORALL i IN 1..rowids.COUNT
          DELETE FROM pax_doca WHERE rowid=rowids(i);

        DELETE FROM pax_events WHERE pax_id=paxids(k);
        DELETE FROM stat_ad WHERE pax_id=paxids(k);
        DELETE FROM confirm_print WHERE pax_id=paxids(k);
        DELETE FROM pax_fqt WHERE pax_id=paxids(k);
        DELETE FROM pax_asvc WHERE pax_id=paxids(k);
        DELETE FROM pax_emd WHERE pax_id=paxids(k);
        DELETE FROM pax_brands WHERE pax_id=paxids(k);
        DELETE FROM pax_rem_origin WHERE pax_id=paxids(k);
        DELETE FROM pax_seats WHERE pax_id=paxids(k);
        DELETE FROM rozysk WHERE pax_id=paxids(k);
        DELETE FROM trip_comp_layers WHERE pax_id=paxids(k);
        UPDATE service_payment SET pax_id=NULL WHERE pax_id=paxids(k);
        DELETE FROM pax_alarms WHERE pax_id=paxids(k);
        DELETE FROM pax_custom_alarms WHERE pax_id=paxids(k);
        DELETE FROM pax_service_lists WHERE pax_id=paxids(k);
        DELETE FROM pax_services WHERE pax_id=paxids(k);
        DELETE FROM pax_services_auto WHERE pax_id=paxids(k);
        DELETE FROM paid_rfisc WHERE pax_id=paxids(k);
        DELETE FROM pax_norms_text WHERE pax_id=paxids(k);
        DELETE FROM sbdo_tags_generated WHERE pax_id=paxids(k);
        DELETE FROM pax_calc_data WHERE pax_calc_data_id=paxids(k);
        DELETE FROM pax_confirmations WHERE pax_id=paxids(k);
        pax_count:=pax_count+1;
      END LOOP;
      FORALL i IN 1..paxrowids.COUNT
        DELETE FROM pax WHERE rowid=paxrowids(i);
      DELETE FROM paid_bag_emd_props WHERE grp_id=grpids(j);
      DELETE FROM service_payment WHERE grp_id=grpids(j);
      DELETE FROM tckin_pax_grp WHERE grp_id=grpids(j);
      DELETE FROM pnr_addrs_pc WHERE grp_id=grpids(j);
      DELETE FROM grp_service_lists WHERE grp_id=grpids(j);
      DELETE FROM bag_tags_generated WHERE grp_id=grpids(j);
      DELETE FROM mps_exchange WHERE grp_id=grpids(j);
      DELETE FROM svc_prices WHERE grp_id=grpids(j);
    END LOOP;
    FORALL i IN 1..grprowids.COUNT
      DELETE FROM pax_grp WHERE rowid=grprowids(i);

    FOR curAODBPaxRow IN curAODBPax(curRow.point_id) LOOP
      DELETE FROM aodb_bag
      WHERE pax_id=curAODBPaxRow.pax_id AND point_addr=curAODBPaxRow.point_addr;
    END LOOP;
    DELETE FROM aodb_pax_change WHERE point_id=curRow.point_id;
    DELETE FROM aodb_unaccomp WHERE point_id=curRow.point_id;
    DELETE FROM aodb_pax WHERE point_id=curRow.point_id;
    DELETE FROM aodb_points WHERE point_id=curRow.point_id;
    DELETE FROM exch_flights WHERE point_id=curRow.point_id;
    DELETE FROM counters2 WHERE point_dep=curRow.point_id;
    DELETE FROM crs_counters WHERE point_dep=curRow.point_id;
    DELETE FROM crs_displace2 WHERE point_id_spp=curRow.point_id;
    DELETE FROM snapshot_points WHERE point_id=curRow.point_id;
    UPDATE tag_ranges2 SET point_id=NULL WHERE point_id=curRow.point_id;
    DELETE FROM tlg_binding WHERE point_id_spp=curRow.point_id;
    DELETE FROM trip_bp WHERE point_id=curRow.point_id;
    DELETE FROM trip_hall WHERE point_id=curRow.point_id;
    DELETE FROM trip_bt WHERE point_id=curRow.point_id;
    DELETE FROM trip_ckin_client WHERE point_id=curRow.point_id;
    DELETE FROM trip_classes WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_rem WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_rates WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_rfisc WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_baselayers WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_elems WHERE point_id=curRow.point_id;
    DELETE FROM trip_comp_layers WHERE point_id=curRow.point_id;
    DELETE FROM trip_crew WHERE point_id=curRow.point_id;
    DELETE FROM trip_data WHERE point_id=curRow.point_id;
    DELETE FROM trip_delays WHERE point_id=curRow.point_id;
    DELETE FROM trip_load WHERE point_dep=curRow.point_id;
    DELETE FROM trip_sets WHERE point_id=curRow.point_id;
    DELETE FROM trip_final_stages WHERE point_id=curRow.point_id;
    DELETE FROM trip_stations WHERE point_id=curRow.point_id;
    DELETE FROM trip_paid_ckin WHERE point_id=curRow.point_id;
    DELETE FROM trip_calc_data WHERE point_id=curRow.point_id;
    DELETE FROM trip_pers_weights WHERE point_id=curRow.point_id;
    DELETE FROM trip_auto_weighing WHERE point_id=curRow.point_id;
    UPDATE trfer_trips SET point_id_spp=NULL WHERE point_id_spp=curRow.point_id;
    DELETE FROM pax_seats WHERE point_id=curRow.point_id;
    DELETE FROM trip_tasks WHERE point_id=curRow.point_id;
    DELETE FROM counters_by_subcls WHERE point_id=curRow.point_id;
    DELETE FROM apps_messages WHERE msg_id in (SELECT cirq_msg_id FROM apps_pax_data where point_id=curRow.point_id);
    DELETE FROM apps_messages WHERE msg_id in (SELECT cicx_msg_id FROM apps_pax_data where point_id=curRow.point_id);
    DELETE FROM apps_messages WHERE msg_id in (SELECT msg_id FROM apps_manifest_data where point_id=curRow.point_id);
    DELETE FROM apps_pax_data WHERE point_id=curRow.point_id;
    DELETE FROM apps_manifest_data WHERE point_id=curRow.point_id;
--    DELETE FROM iapi_pax_data WHERE point_id=curRow.point_id;
    DELETE FROM wb_msg_text where id in(SELECT id FROM wb_msg WHERE point_id = curRow.point_id);
    DELETE FROM wb_msg where point_id = curRow.point_id;
    DELETE FROM trip_vouchers WHERE point_id=curRow.point_id;
    DELETE FROM confirm_print_vo_unreg WHERE point_id = curRow.point_id;
    DELETE FROM del_vo WHERE point_id = curRow.point_id;
    DELETE FROM hotel_acmd_pax WHERE point_id = curRow.point_id;
    DELETE FROM hotel_acmd_free_pax WHERE point_id = curRow.point_id;
    DELETE FROM hotel_acmd_dates WHERE point_id = curRow.point_id;
  END LOOP;
  DELETE FROM points WHERE move_id=vmove_id;
  DELETE FROM move_ref WHERE move_id=vmove_id;
END;

PROCEDURE move(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER)
IS
i               BINARY_INTEGER;
rowids          TRowidsTable;
ids             TIdsTable;
remain_rows     INTEGER;
time_finish     DATE;
cur             TRowidCursor;
BEGIN
  remain_rows:=max_rows;
  time_finish:=SYSDATE+time_duration/86400;

  WHILE step>=1 AND step<=4 LOOP
    IF SYSDATE>time_finish THEN RETURN; END IF;
    IF step=1 THEN
      OPEN cur FOR
        SELECT rowid, id
        FROM tlg_out
        WHERE point_id IS NULL AND time_create<arx_date AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=2 THEN
      OPEN cur FOR
        SELECT rowid
        FROM events_bilingual
        WHERE type LIKE '%'||system.evtTlg AND time>=arx_date-30 AND time<arx_date AND   --LIKE ᯥ樠�쭮 ��� ��襣� ����� ࠧ���
              id1 IS NULL AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=3 THEN
      OPEN cur FOR
        SELECT rowid
        FROM events_bilingual
        WHERE type NOT IN (system.evtSeason,
                           system.evtDisp,
                           system.evtFlt,
                           system.evtGraph,
                           system.evtFltTask,
                           system.evtPax,
                           system.evtPay,
                           system.evtTlg,
                           system.evtPrn) AND time>=arx_date-30 AND time<arx_date AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=4 THEN
      OPEN cur FOR
        SELECT rowid
        FROM stat_zamar
        WHERE time<arx_date AND
              rownum<=remain_rows FOR UPDATE;
    END IF;

    LOOP
      IF step=1 THEN
        FETCH cur BULK COLLECT INTO rowids, ids LIMIT BULK_COLLECT_LIMIT;
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_tlg_out
            (id,num,type,point_id,addr,origin,heading,body,ending,pr_lat,completed,has_errors,
             time_create,time_send_scd,time_send_act,originator_id,airline_mark,manual_creation,
             part_key)
          SELECT
             id,num,type,point_id,addr,origin,heading,body,ending,pr_lat,completed,has_errors,
             time_create,time_send_scd,time_send_act,originator_id,airline_mark,manual_creation,
             NVL(time_send_act,NVL(time_send_scd,time_create))
          FROM tlg_out
          WHERE rowid=rowids(i);
        FORALL i IN 1..ids.COUNT
          DELETE FROM typeb_out_extra WHERE tlg_id=ids(i);
        FORALL i IN 1..ids.COUNT
          DELETE FROM typeb_out_errors WHERE tlg_id=ids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM tlg_out WHERE rowid=rowids(i);
      END IF;
      IF step=2 THEN
        FETCH cur BULK COLLECT INTO rowids LIMIT BULK_COLLECT_LIMIT;
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_events
            (type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,part_key,part_num,lang)
          SELECT
             type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,time,part_num,lang
          FROM events_bilingual
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM events_bilingual WHERE rowid=rowids(i);
      END IF;
      IF step=3 THEN
        FETCH cur BULK COLLECT INTO rowids LIMIT BULK_COLLECT_LIMIT;
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_events
            (type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,part_key,part_num,lang)
          SELECT
             type,sub_type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3,time,part_num,lang
          FROM events_bilingual
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM events_bilingual WHERE rowid=rowids(i);
      END IF;
      IF step=4 THEN
        FETCH cur BULK COLLECT INTO rowids LIMIT BULK_COLLECT_LIMIT;
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_stat_zamar
            (time, airline, airp, amount_ok, amount_fault, part_key, sbdo_type)
          SELECT
             time, airline, airp, amount_ok, amount_fault, time, sbdo_type
          FROM stat_zamar
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM stat_zamar WHERE rowid=rowids(i);
      END IF;
      remain_rows:=remain_rows-rowids.COUNT;
      EXIT WHEN rowids.COUNT<BULK_COLLECT_LIMIT;
      IF SYSDATE>time_finish THEN CLOSE cur; RETURN; END IF;
    END LOOP;
    CLOSE cur;

    IF remain_rows<=0 THEN RETURN; END IF;
    step:=step+1;
  END LOOP;
  step:=0;
END;

PROCEDURE tlg_trip(vpoint_id  tlg_trips.point_id%TYPE)
IS
n       INTEGER;
pnrids  TIdsTable;
paxids  TIdsTable;
i       BINARY_INTEGER;
BEGIN
  ckin.delete_typeb_data(vpoint_id, NULL, NULL, FALSE);
  DELETE FROM typeb_data_stat WHERE point_id=vpoint_id;
  DELETE FROM crs_data_stat WHERE point_id=vpoint_id;
  DELETE FROM tlg_comp_layers WHERE point_id=vpoint_id;
  UPDATE crs_displace2 SET point_id_tlg=NULL WHERE point_id_tlg=vpoint_id;

  SELECT trfer_id BULK COLLECT INTO pnrids
  FROM tlg_transfer
  WHERE point_id_out=vpoint_id;
  SELECT trfer_grp.grp_id BULK COLLECT INTO paxids
  FROM tlg_transfer,trfer_grp
  WHERE tlg_transfer.trfer_id=trfer_grp.trfer_id AND
        tlg_transfer.point_id_out=vpoint_id;

  FORALL i IN 1..paxids.COUNT
    DELETE FROM trfer_pax WHERE grp_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM trfer_tags WHERE grp_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM tlg_trfer_onwards WHERE grp_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM tlg_trfer_excepts WHERE grp_id=paxids(i);
  FORALL i IN 1..pnrids.COUNT
    DELETE FROM trfer_grp WHERE trfer_id=pnrids(i);

  DELETE FROM tlg_transfer WHERE point_id_out=vpoint_id;

  SELECT COUNT(*) INTO n FROM tlg_transfer
  WHERE point_id_in=vpoint_id AND point_id_in<>point_id_out;
  IF n=0 THEN
    DELETE FROM tlg_source WHERE point_id_tlg=vpoint_id;
    DELETE FROM tlg_trips WHERE point_id=vpoint_id;
  ELSE
    /* 㤠�塞 ⮫쪮 � ��뫪� �� ⥫��ࠬ�� ������ ��� � tlg_transfer */
    DELETE FROM tlg_source
    WHERE point_id_tlg=vpoint_id AND
          NOT EXISTS (SELECT * FROM tlg_transfer
                      WHERE tlg_transfer.point_id_in=tlg_source.point_id_tlg AND
                            tlg_transfer.tlg_id=tlg_source.tlg_id AND rownum<2);
  END IF;
END tlg_trip;

PROCEDURE norms_rates_etc(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER)
IS
i               BINARY_INTEGER;
rowids          TRowidsTable;
remain_rows     INTEGER;
time_finish     DATE;
cur             TRowidCursor;
BEGIN
  remain_rows:=max_rows;
  time_finish:=SYSDATE+time_duration/86400;

  WHILE step>=1 AND step<=5 LOOP
    IF SYSDATE>time_finish THEN RETURN; END IF;
    IF step=1 THEN
      OPEN cur FOR
        SELECT rowid
        FROM bag_norms
        WHERE last_date<arx_date AND
              NOT EXISTS (SELECT * FROM pax_norms WHERE pax_norms.norm_id=bag_norms.id AND rownum<2) AND
              NOT EXISTS (SELECT * FROM grp_norms WHERE grp_norms.norm_id=bag_norms.id AND rownum<2) AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=2 THEN
      OPEN cur FOR
        SELECT rowid
        FROM bag_rates
        WHERE last_date<arx_date AND
              NOT EXISTS (SELECT * FROM paid_bag WHERE paid_bag.rate_id=bag_rates.id AND rownum<2) AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=3 THEN
      OPEN cur FOR
        SELECT rowid
        FROM value_bag_taxes
        WHERE last_date<arx_date AND
              NOT EXISTS (SELECT * FROM value_bag WHERE value_bag.tax_id=value_bag_taxes.id AND rownum<2) AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=4 THEN
      OPEN cur FOR
        SELECT rowid
        FROM exchange_rates
        WHERE last_date<arx_date AND
              rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=5 THEN
      OPEN cur FOR
        SELECT rowid
        FROM mark_trips
        WHERE scd<arx_date AND
              rownum<=remain_rows FOR UPDATE;
    END IF;

    LOOP
      FETCH cur BULK COLLECT INTO rowids LIMIT BULK_COLLECT_LIMIT;
      IF step=1 THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_bag_norms
            (id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
             first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,tid,part_key)
          SELECT
             id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
             first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,tid,last_date
          FROM bag_norms
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM bag_norms WHERE rowid=rowids(i);
      END IF;
      IF step=2 THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_bag_rates
            (id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
             first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid,part_key)
          SELECT
             id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
             first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid,last_date
          FROM bag_rates
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM bag_rates WHERE rowid=rowids(i);
      END IF;
      IF step=3 THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_value_bag_taxes
            (id,airline,pr_trfer,city_dep,city_arv,
             first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid,part_key)
          SELECT
             id,airline,pr_trfer,city_dep,city_arv,
             first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid,last_date
          FROM value_bag_taxes
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM value_bag_taxes WHERE rowid=rowids(i);
      END IF;
      IF step=4 THEN
        FORALL i IN 1..rowids.COUNT
          INSERT INTO arx_exchange_rates
            (id,airline,rate1,cur1,rate2,cur2,first_date,last_date,extra,pr_del,tid,part_key)
          SELECT
             id,airline,rate1,cur1,rate2,cur2,first_date,last_date,extra,pr_del,tid,last_date
          FROM exchange_rates
          WHERE rowid=rowids(i);
        FORALL i IN 1..rowids.COUNT
          DELETE FROM exchange_rates WHERE rowid=rowids(i);
      END IF;
      IF step=5 THEN
        FOR i IN 1..rowids.COUNT LOOP
          BEGIN
            DELETE FROM mark_trips WHERE rowid=rowids(i);
          EXCEPTION
            WHEN OTHERS THEN
              IF SQLCODE=-02292 THEN NULL; ELSE RAISE; END IF;
          END;
        END LOOP;
      END IF;
      remain_rows:=remain_rows-rowids.COUNT;
      EXIT WHEN rowids.COUNT<BULK_COLLECT_LIMIT;
      IF SYSDATE>time_finish THEN CLOSE cur; RETURN; END IF;
    END LOOP;
    CLOSE cur;

    IF remain_rows<=0 THEN RETURN; END IF;
    step:=step+1;
  END LOOP;
  step:=0;
END norms_rates_etc;

PROCEDURE tlgs_files_etc(arx_date DATE, max_rows INTEGER, time_duration INTEGER, step IN OUT INTEGER)
IS
i               BINARY_INTEGER;
rowids          TRowidsTable;
ids             TIdsTable;
remain_rows     INTEGER;
time_finish     DATE;
cur             TRowidCursor;

TYPE aodb_spp_files_key IS RECORD
(
filename        aodb_spp_files.filename%TYPE,
point_addr      aodb_spp_files.point_addr%TYPE,
airline         aodb_spp_files.airline%TYPE
);
curRow	        aodb_spp_files_key;
BEGIN
  remain_rows:=max_rows;
  time_finish:=SYSDATE+time_duration/86400;

  WHILE step>=1 AND step<=8 LOOP
    IF SYSDATE>time_finish THEN RETURN; END IF;
    IF step=1 THEN
      OPEN cur FOR
        SELECT rowid,id
        FROM tlgs
        WHERE time<arx_date AND rownum<=remain_rows FOR UPDATE;
    END IF;
    IF step=2 THEN
      OPEN cur FOR
        SELECT rowid,id
        FROM files
        WHERE time<arx_date AND rownum<=remain_rows FOR UPDATE;
    END IF;

    IF step=8 THEN
      OPEN cur FOR
        SELECT rowid,id
        FROM kiosk_events
        WHERE time<arx_date AND rownum<=remain_rows FOR UPDATE;
    END IF;

    IF step=1 OR step=2 OR step=8 THEN
      LOOP
        FETCH cur BULK COLLECT INTO rowids,ids LIMIT BULK_COLLECT_LIMIT;
        IF step=1 THEN
          FORALL i IN 1..ids.COUNT
            INSERT INTO arx_tlg_stat
              (queue_tlg_id, typeb_tlg_id, typeb_tlg_num,
               sender_sita_addr, sender_canon_name, sender_descr, sender_country,
               receiver_sita_addr, receiver_canon_name, receiver_descr, receiver_country,
               time_create, time_send, time_receive, tlg_type, tlg_len,
               airline, flt_no, suffix, scd_local_date, airp_dep, airline_mark, extra, part_key)
            SELECT
               queue_tlg_id, typeb_tlg_id, typeb_tlg_num,
               sender_sita_addr, sender_canon_name, sender_descr, sender_country,
               receiver_sita_addr, receiver_canon_name, receiver_descr, receiver_country,
               time_create, time_send, time_receive, tlg_type, tlg_len,
               airline, flt_no, suffix, scd_local_date, airp_dep, airline_mark, extra, time_send
            FROM tlg_stat
            WHERE queue_tlg_id=ids(i) AND time_send IS NOT NULL;
          FORALL i IN 1..ids.COUNT
            DELETE FROM tlg_stat WHERE queue_tlg_id=ids(i);

          FORALL i IN 1..ids.COUNT
            DELETE FROM tlg_error WHERE id=ids(i);
          FORALL i IN 1..ids.COUNT
            DELETE FROM tlg_queue WHERE id=ids(i);
          FORALL i IN 1..ids.COUNT
            DELETE FROM tlgs_text WHERE id=ids(i);

          FORALL i IN 1..rowids.COUNT
            DELETE FROM tlgs WHERE rowid=rowids(i);
        END IF;
        IF step=2 THEN
          FORALL i IN 1..ids.COUNT
            DELETE FROM file_queue WHERE id=ids(i);
          FORALL i IN 1..ids.COUNT
            DELETE FROM file_params WHERE id=ids(i);
          FORALL i IN 1..ids.COUNT
            DELETE FROM file_error WHERE id=ids(i);

          FORALL i IN 1..rowids.COUNT
            DELETE FROM files WHERE rowid=rowids(i);
        END IF;
        IF step=8 THEN
          FORALL i IN 1..ids.COUNT
            DELETE FROM kiosk_event_params WHERE event_id=ids(i);

          FORALL i IN 1..rowids.COUNT
            DELETE FROM kiosk_events WHERE rowid=rowids(i);
        END IF;

        remain_rows:=remain_rows-rowids.COUNT;
        EXIT WHEN rowids.COUNT<BULK_COLLECT_LIMIT;
        IF SYSDATE>time_finish THEN CLOSE cur; RETURN; END IF;
      END LOOP;
      CLOSE cur;
    END IF;
    IF step=3 THEN
      DELETE FROM rozysk WHERE time<arx_date AND rownum<=remain_rows;
      remain_rows:=remain_rows-SQL%ROWCOUNT;
    END IF;
    IF step=4 THEN
      OPEN cur FOR
        SELECT filename, point_addr, airline
        FROM aodb_spp_files
        WHERE filename<'SPP'||TO_CHAR(arx_date, 'YYMMDD')||'.txt' AND rownum<=remain_rows FOR UPDATE;
      LOOP
        FETCH cur INTO curRow;
        EXIT WHEN cur%NOTFOUND;
        DELETE FROM aodb_events
        WHERE filename=curRow.filename AND point_addr=curRow.point_addr AND airline=curRow.airline AND
              rownum<=remain_rows;
        remain_rows:=remain_rows-SQL%ROWCOUNT;
        EXIT WHEN remain_rows<=0;
        DELETE FROM aodb_spp_files
        WHERE filename=curRow.filename AND point_addr=curRow.point_addr AND airline=curRow.airline;
        remain_rows:=remain_rows-SQL%ROWCOUNT;
        EXIT WHEN remain_rows<=0;
        IF SYSDATE>time_finish THEN CLOSE cur; RETURN; END IF;
      END LOOP;
      CLOSE cur;
    END IF;
    IF remain_rows<=0 THEN RETURN; END IF;
    step:=step+1;
  END LOOP;
  step:=0;
END;

PROCEDURE move_typeb_in(vid tlgs_in.id%TYPE)
IS
BEGIN
  DELETE FROM typeb_in_body WHERE id=vid;
  DELETE FROM typeb_in_errors WHERE tlg_id=vid;
  DELETE FROM typeb_in_history WHERE prev_tlg_id=vid;
  DELETE FROM typeb_in_history WHERE tlg_id=vid;
  DELETE FROM tlgs_in WHERE id=vid;
  DELETE FROM typeb_in WHERE id=vid;
END;

END arch;
/
