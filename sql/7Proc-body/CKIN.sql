create or replace PACKAGE BODY ckin
AS

FUNCTION next_airp(vfirst_point 	IN points.first_point%TYPE,
                   vpoint_num        	IN points.point_num%TYPE) RETURN points.airp%TYPE
AS
CURSOR cur IS
  SELECT airp FROM points
  WHERE first_point=vfirst_point AND point_num>vpoint_num AND pr_del=0
  ORDER BY point_num;
vairp	points.airp%TYPE;
BEGIN
  vairp:=NULL;
  OPEN cur;
  FETCH cur INTO vairp;
  CLOSE cur;
  RETURN vairp;
END next_airp;

FUNCTION tranzitable(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE
IS
vmove_id	points.move_id%TYPE;
vpoint_num	points.point_num%TYPE;
n		BINARY_INTEGER;
BEGIN
  BEGIN
    SELECT move_id,point_num
    INTO vmove_id,vpoint_num
    FROM points
    WHERE point_id=vpoint_id AND pr_del=0;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN RETURN 0;
  END;

  SELECT COUNT(*) INTO n FROM points
  WHERE move_id=vmove_id AND point_num<vpoint_num AND pr_del=0 AND rownum<2;
  IF n=0 THEN RETURN 0; END IF;
  RETURN 1;
END tranzitable;

FUNCTION get_pr_tranzit(vpoint_id	IN points.point_id%TYPE) RETURN points.pr_tranzit%TYPE
IS
vfirst_point	points.first_point%TYPE;
vpoint_num	points.point_num%TYPE;
n		BINARY_INTEGER;
BEGIN
  BEGIN
    SELECT DECODE(pr_tranzit,0,points.point_id,first_point),point_num
    INTO vfirst_point,vpoint_num
    FROM points
    WHERE point_id=vpoint_id AND pr_del=0 AND pr_tranzit<>0;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN RETURN 0;
  END;

  SELECT COUNT(*) INTO n FROM points
  WHERE first_point=vfirst_point AND point_num>vpoint_num AND pr_del=0 AND rownum<2;
  IF n=0 THEN RETURN 0; END IF;
  SELECT COUNT(*) INTO n FROM points
  WHERE vfirst_point IN (point_id,first_point) AND point_num<vpoint_num AND pr_del=0; --rownum портит план разбора
  IF n=0 THEN RETURN 0; END IF;
  RETURN 1;
END get_pr_tranzit;

FUNCTION get_pr_tranz_reg(vpoint_id	IN points.point_id%TYPE) RETURN trip_sets.pr_tranz_reg%TYPE
IS
res             trip_sets.pr_tranz_reg%TYPE;
BEGIN
  IF ckin.get_pr_tranzit(vpoint_id)=0 THEN RETURN 0; END IF;
  SELECT NVL(pr_tranz_reg,0) INTO res FROM trip_sets WHERE point_id=vpoint_id;
  RETURN res;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN 0;
END get_pr_tranz_reg;

FUNCTION build_birks_str(cur birks_cursor_ref) RETURN VARCHAR2
IS
res	      VARCHAR2(4000);
noStr     VARCHAR2(15);
firstStr	VARCHAR2(17);
lastStr		VARCHAR2(3);
diff		  BINARY_INTEGER;

TYPE birks_cursor_row IS RECORD
(
tag_type	 bag_tags.tag_type%TYPE,
no_len		 tag_types.no_len%TYPE,
color 		 tag_colors.code%TYPE,
color_view tag_colors.code%TYPE,
first           bag_tags.no%TYPE,
last            bag_tags.no%TYPE,
no              bag_tags.no%TYPE
);

curRow		birks_cursor_row;
oldRow  	birks_cursor_row;
oldRow2		birks_cursor_row;
BEGIN
  res:=NULL;
  BEGIN
    FETCH cur INTO curRow;
    IF cur%FOUND THEN
      diff:=1;
      oldRow:=curRow;
      oldRow2.tag_type:=NULL;
      oldRow2.no_len:=NULL;
      oldRow2.color:=NULL;
      oldRow2.first:=NULL;
      oldRow2.last:=NULL;
      LOOP
        FETCH cur INTO curRow;
        IF cur%FOUND AND
           oldRow.tag_type=curRow.tag_type AND
           (oldRow.color IS NOT NULL AND curRow.color IS NOT NULL AND
            oldRow.color=curRow.color OR
            oldRow.color IS NULL AND curRow.color IS NULL) AND
           oldRow.first=curRow.first AND
           oldRow.last+diff=curRow.last THEN
          diff:=diff+1;
        ELSE
          IF oldRow2.tag_type=oldRow.tag_type AND
             (oldRow2.color IS NOT NULL AND oldRow.color IS NOT NULL AND
              oldRow2.color=oldRow.color OR
              oldRow2.color IS NULL AND oldRow.color IS NULL) AND
             oldRow2.first=oldRow.first THEN
            firstStr:=LPAD(TO_CHAR(oldRow.last),3,'0');
            IF res IS NOT NULL THEN res:=res||','; END IF;
          ELSE
            firstStr:=oldRow.color_view;
            noStr:=TO_CHAR(oldRow.first*1000+oldRow.last);
            IF LENGTH(noStr)<oldRow.no_len THEN
              firstStr:=firstStr||LPAD(noStr,oldRow.no_len,'0');
            ELSE
              firstStr:=firstStr||noStr;
            END IF;
            oldRow2:=oldRow;
            IF res IS NOT NULL THEN res:=res||', '; END IF;
          END IF;
          IF diff<>1 THEN
            lastStr:=LPAD(TO_CHAR(oldRow.last+diff-1),3,'0');
            res:=res||firstStr||'-'||lastStr;
          ELSE
            res:=res||firstStr;
          END IF;
          diff:=1;
          oldRow:=curRow;
        END IF;
        EXIT WHEN not(cur%FOUND);
      END LOOP;
    END IF;
  EXCEPTION
    WHEN VALUE_ERROR THEN NULL;
  END;
  RETURN res;
END build_birks_str;

FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    pr_lat        IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
BEGIN
  IF pr_lat<>0 THEN
    RETURN get_birks2(vgrp_id, vpax_id, vbag_pool_num, '');
  ELSE
    RETURN get_birks2(vgrp_id, vpax_id, vbag_pool_num, 'RU');
  END IF;
END get_birks2;

FUNCTION get_birks2(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id 	    IN pax.pax_id%TYPE,
                    vbag_pool_num IN pax.bag_pool_num%TYPE,
                    vlang	        IN lang_types.code%TYPE) RETURN VARCHAR2
IS
res	VARCHAR2(4000);
pool_pax_id    pax.pax_id%TYPE;
cur            ckin.birks_cursor_ref;
BEGIN
  res:=NULL;
  IF vpax_id IS NOT NULL THEN
    IF vbag_pool_num IS NULL THEN RETURN res; END IF;
    pool_pax_id:=get_bag_pool_pax_id(vgrp_id,vbag_pool_num);
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
        FROM bag_tags,tag_types,tag_colors
        WHERE bag_tags.tag_type=tag_types.code AND
              bag_tags.color=tag_colors.code(+) AND
              grp_id=vgrp_id
        ORDER BY tag_type,color,no;
    ELSE
      IF vbag_pool_num=1 THEN
        /*для тех групп которые регистрировались с терминала без обязательной привязки */
        OPEN cur FOR
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM bag2,bag_tags,tag_types,tag_colors
          WHERE bag2.grp_id=bag_tags.grp_id AND
                bag2.num=bag_tags.bag_num AND
                bag_tags.tag_type=tag_types.code AND
                bag_tags.color=tag_colors.code(+) AND
                bag2.grp_id=vgrp_id AND bag2.bag_pool_num=vbag_pool_num
          UNION
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM bag_tags,tag_types,tag_colors
          WHERE bag_tags.tag_type=tag_types.code AND
                bag_tags.color=tag_colors.code(+) AND
                bag_tags.grp_id=vgrp_id AND bag_tags.bag_num IS NULL
          ORDER BY tag_type,color,no;
      ELSE
        OPEN cur FOR
          SELECT tag_type,no_len,
                 tag_colors.code AS color,
                 DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
                 TRUNC(no/1000) AS first,
                 MOD(no,1000) AS last,
                 no
          FROM bag2,bag_tags,tag_types,tag_colors
          WHERE bag2.grp_id=bag_tags.grp_id AND
                bag2.num=bag_tags.bag_num AND
                bag_tags.tag_type=tag_types.code AND
                bag_tags.color=tag_colors.code(+) AND
                bag2.grp_id=vgrp_id AND bag2.bag_pool_num=vbag_pool_num
          ORDER BY tag_type,color,no;
      END IF;
    END IF;
    res:=ckin.build_birks_str(cur);
    CLOSE cur;
  END IF;
  RETURN res;
END get_birks2;

FUNCTION need_for_payment(vgrp_id        IN pax_grp.grp_id%TYPE,
                          vclass         IN pax_grp.class%TYPE,
                          vbag_refuse    IN pax_grp.bag_refuse%TYPE,
                          vpiece_concept IN pax_grp.piece_concept%TYPE,
                          vexcess_wt     IN pax_grp.excess_wt%TYPE,
                          vexcess_pc     IN pax_grp.excess_pc%TYPE,
                          vpax_id        IN pax.pax_id%TYPE) RETURN NUMBER
IS
res NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 0; END IF;

  IF vpiece_concept=0 THEN
    IF vexcess_wt>0 THEN RETURN 1; END IF;
  ELSE
    IF vpax_id IS NOT NULL THEN
      SELECT COUNT(*)
      INTO res
      FROM paid_rfisc
      WHERE pax_id=vpax_id AND paid>0 AND rownum<2;

      IF res>0 THEN RETURN 1; END IF;
    END IF;
  END IF;

  SELECT COUNT(*)
  INTO res
  FROM value_bag, bag2
  WHERE value_bag.grp_id=bag2.grp_id(+) AND
        value_bag.num=bag2.value_bag_num(+) AND
        (bag2.grp_id IS NULL OR
         ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,vclass,vbag_refuse)=0) AND
        value_bag.grp_id=vgrp_id AND value_bag.value>0 AND rownum<2;
  RETURN res;
END need_for_payment;

FUNCTION get_bagInfo2(vgrp_id       IN pax.grp_id%TYPE,
                      vpax_id 	    IN pax.pax_id%TYPE,
                      vbag_pool_num IN pax.bag_pool_num%TYPE) RETURN TBagInfo
IS
bagInfo		TBagInfo;
pool_pax_id    pax.pax_id%TYPE;
BEGIN
  bagInfo.bagAmount:=NULL;
  bagInfo.bagWeight:=NULL;
  bagInfo.rkAmount:=NULL;
  bagInfo.rkWeight:=NULL;
  IF vpax_id IS NOT NULL THEN
    IF vbag_pool_num IS NULL THEN RETURN bagInfo; END IF;
    pool_pax_id:=get_bag_pool_pax_id(vgrp_id,vbag_pool_num);
  END IF;
  IF vpax_id IS NULL OR
     pool_pax_id IS NOT NULL AND pool_pax_id=vpax_id THEN
    IF vpax_id IS NULL THEN
      SELECT SUM(DECODE(pr_cabin,0,amount,NULL)) AS bagAmount,
             SUM(DECODE(pr_cabin,0,weight,NULL)) AS bagWeight,
             SUM(DECODE(pr_cabin,0,NULL,amount)) AS rkAmount,
             SUM(DECODE(pr_cabin,0,NULL,weight)) AS rkWeight
      INTO bagInfo.bagAmount,bagInfo.bagWeight,bagInfo.rkAmount,bagInfo.rkWeight
      FROM bag2
      WHERE grp_id=vgrp_id;
    ELSE
      SELECT SUM(DECODE(pr_cabin,0,amount,NULL)) AS bagAmount,
             SUM(DECODE(pr_cabin,0,weight,NULL)) AS bagWeight,
             SUM(DECODE(pr_cabin,0,NULL,amount)) AS rkAmount,
             SUM(DECODE(pr_cabin,0,NULL,weight)) AS rkWeight
      INTO bagInfo.bagAmount,bagInfo.bagWeight,bagInfo.rkAmount,bagInfo.rkWeight
      FROM bag2
      WHERE grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
    END IF;
  END IF;
  bagInfo.grp_id:=vgrp_id;
  bagInfo.pax_id:=vpax_id;
  RETURN bagInfo;
END get_bagInfo2;

FUNCTION get_bagAmount2(vgrp_id       IN pax.grp_id%TYPE,
                        vpax_id 	    IN pax.pax_id%TYPE,
                        vbag_pool_num IN pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.bagAmount;
END get_bagAmount2;

FUNCTION get_bagWeight2(vgrp_id       IN pax.grp_id%TYPE,
                        vpax_id 	    IN pax.pax_id%TYPE,
                        vbag_pool_num IN pax.bag_pool_num%TYPE,
                        row	          IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.bagWeight;
END get_bagWeight2;

FUNCTION get_rkAmount2(vgrp_id       IN pax.grp_id%TYPE,
                       vpax_id 	     IN pax.pax_id%TYPE,
                       vbag_pool_num IN pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.rkAmount;
END get_rkAmount2;

FUNCTION get_rkWeight2(vgrp_id       IN pax.grp_id%TYPE,
                       vpax_id  	   IN pax.pax_id%TYPE,
                       vbag_pool_num IN pax.bag_pool_num%TYPE,
                       row	         IN NUMBER DEFAULT 1) RETURN NUMBER
IS
BEGIN
  IF row<>1 AND
     vgrp_id=bagInfo.grp_id AND
     (vpax_id IS NULL AND bagInfo.pax_id IS NULL OR vpax_id=bagInfo.pax_id) THEN
    NULL;
  ELSE
    bagInfo:=get_bagInfo2(vgrp_id,vpax_id,vbag_pool_num);
  END IF;
  RETURN bagInfo.rkWeight;
END get_rkWeight2;

FUNCTION get_excess_wt(vgrp_id         IN pax.grp_id%TYPE,
                       vpax_id         IN pax.pax_id%TYPE,
                       vexcess_wt      IN pax_grp.excess_wt%TYPE DEFAULT NULL,
                       vbag_refuse     IN pax_grp.bag_refuse%TYPE DEFAULT NULL) RETURN NUMBER

IS
vexcess         pax_grp.excess_wt%TYPE;
main_pax_id     pax.pax_id%TYPE;
BEGIN
  vexcess:=0;

  IF vexcess_wt IS NULL OR vbag_refuse IS NULL THEN
    BEGIN
      SELECT DECODE(bag_refuse, 0, excess_wt, 0)
      INTO vexcess
      FROM pax_grp WHERE grp_id=vgrp_id;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN RETURN NULL;
    END;
  ELSE
    SELECT DECODE(vbag_refuse, 0, vexcess_wt, 0)
    INTO vexcess
    FROM dual;
  END IF;

  IF vpax_id IS NOT NULL THEN
    main_pax_id:=get_main_pax_id2(vgrp_id);
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

FUNCTION get_excess_pc(vgrp_id         IN pax.grp_id%TYPE,
                       vpax_id         IN pax.pax_id%TYPE,
                       include_all_svc IN NUMBER DEFAULT 0) RETURN NUMBER
IS
vexcess         pax_grp.excess_pc%TYPE;
BEGIN
  vexcess:=0;

  IF vpax_id IS NOT NULL THEN
    IF include_all_svc=0 THEN
      SELECT NVL(SUM(paid_rfisc.paid), 0)
      INTO vexcess
      FROM paid_rfisc, rfisc_list_items
      WHERE paid_rfisc.list_id=rfisc_list_items.list_id AND
            paid_rfisc.rfisc=rfisc_list_items.rfisc AND
            paid_rfisc.service_type=rfisc_list_items.service_type AND
            paid_rfisc.airline=rfisc_list_items.airline AND
            paid_rfisc.pax_id=vpax_id AND
            paid_rfisc.transfer_num=0 AND
            paid_rfisc.paid>0 AND
            rfisc_list_items.category IN (1/*baggage*/, 2/*carry_on*/);
    ELSE
      SELECT NVL(SUM(paid), 0)
      INTO vexcess
      FROM paid_rfisc
      WHERE pax_id=vpax_id AND paid>0 AND transfer_num=0;
    END IF;
  END IF;

  IF vexcess=0 THEN vexcess:=NULL; END IF;
  RETURN vexcess;
END get_excess_pc;

FUNCTION get_crs_priority(vcrs           IN crs_set.crs%TYPE,
                          vairline       IN crs_set.airline%TYPE,
                          vflt_no        IN crs_set.flt_no%TYPE,
                          vairp_dep      IN crs_set.airp_dep%TYPE) RETURN NUMBER
IS
CURSOR cur IS
  SELECT priority FROM crs_set
  WHERE crs=vcrs AND airline=vairline AND
        (flt_no=vflt_no OR flt_no IS NULL) AND
        (airp_dep=vairp_dep OR airp_dep IS NULL)
  ORDER BY flt_no,airp_dep;
vpriority       crs_set.priority%TYPE;
BEGIN
  vpriority:=NULL;
  OPEN cur;
  FETCH cur INTO vpriority;
  CLOSE cur;
  RETURN vpriority;
END get_crs_priority;

FUNCTION get_crs_ok(vpoint_id	IN points.point_id%TYPE) RETURN NUMBER
IS
vcrs_ok  NUMBER;
BEGIN
  SELECT SUM(NVL(a.resa,b.crs_ok))
  INTO vcrs_ok
  FROM
   (SELECT resa, airp_arv, class
    FROM trip_data
    WHERE point_id=vpoint_id) a
   FULL OUTER JOIN
   (SELECT crs_ok, airp_arv, class
    FROM crs_counters
    WHERE crs_counters.point_dep=vpoint_id) b
  ON a.airp_arv=b.airp_arv AND
     a.class=b.class;

  RETURN NVL(vcrs_ok,0);
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END get_crs_ok;

FUNCTION delete_grp_trfer(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER
IS
  CURSOR cur IS
    SELECT transfer_num,point_id_trfer FROM transfer WHERE grp_id=vgrp_id FOR UPDATE;
res	BINARY_INTEGER;
BEGIN
  res:=0;
  FOR curRow IN cur LOOP
    DELETE FROM transfer WHERE grp_id=vgrp_id AND transfer_num=curRow.transfer_num;
    BEGIN
      DELETE FROM trfer_trips WHERE point_id=curRow.point_id_trfer;
    EXCEPTION
      WHEN OTHERS THEN NULL;
    END;
    res:=res+1;
  END LOOP;
  RETURN res;
END delete_grp_trfer;

FUNCTION delete_grp_tckin_segs(vgrp_id     pax_grp.grp_id%TYPE) RETURN NUMBER
IS
  CURSOR cur IS
    SELECT seg_no,point_id_trfer FROM tckin_segments WHERE grp_id=vgrp_id FOR UPDATE;
res	BINARY_INTEGER;
BEGIN
  res:=0;
  FOR curRow IN cur LOOP
    DELETE FROM tckin_segments WHERE grp_id=vgrp_id AND seg_no=curRow.seg_no;
    BEGIN
      DELETE FROM trfer_trips WHERE point_id=curRow.point_id_trfer;
    EXCEPTION
      WHEN OTHERS THEN NULL;
    END;
    res:=res+1;
  END LOOP;
  RETURN res;
END delete_grp_tckin_segs;

PROCEDURE check_grp(vgrp_id     pax_grp.grp_id%TYPE)
IS
CURSOR cur(vgrp_id       IN pax_grp.grp_id%TYPE) IS
  SELECT pax_id,reg_no,refuse,bag_pool_num FROM pax WHERE grp_id=vgrp_id;
CURSOR bagPoolCur(vgrp_id       IN bag2.grp_id%TYPE,
                  vbag_pool_num IN bag2.bag_pool_num%TYPE) IS
  SELECT num,value_bag_num FROM bag2 WHERE grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
CURSOR valueBagCur(vgrp_id       IN pax_grp.grp_id%TYPE) IS
  SELECT num FROM value_bag WHERE grp_id=vgrp_id ORDER BY num;
CURSOR bagCur(vgrp_id       IN pax_grp.grp_id%TYPE) IS
  SELECT num,bag_type FROM bag2 WHERE grp_id=vgrp_id ORDER BY num;
CURSOR tagCur(vgrp_id       IN pax_grp.grp_id%TYPE) IS
  SELECT num FROM bag_tags WHERE grp_id=vgrp_id ORDER BY num;
CURSOR langCur IS
  SELECT code AS lang FROM lang_types;

vclass        pax_grp.class%TYPE;
vstatus       pax_grp.status%TYPE;
vbag_refuse   pax_grp.bag_refuse%TYPE;
vpoint_dep    pax_grp.point_dep%TYPE;
is_unaccomp   BOOLEAN;
checked       BINARY_INTEGER;
refused	      BINARY_INTEGER;
deleted	      BINARY_INTEGER;
i             BINARY_INTEGER;
j             BINARY_INTEGER;
TYPE TBagPoolNums IS TABLE OF BOOLEAN INDEX BY BINARY_INTEGER;
del_pools TBagPoolNums;
BEGIN
  SELECT class,status,bag_refuse,point_dep
  INTO vclass,vstatus,vbag_refuse,vpoint_dep
  FROM pax_grp WHERE grp_id=vgrp_id;
  is_unaccomp:=vclass IS NULL AND vstatus<>'E';
  IF NOT(is_unaccomp) THEN
    /* это не багаж без сопровождения */
    checked:=0;
    refused:=0;
    deleted:=0;
    FOR curRow IN cur(vgrp_id) LOOP
      IF curRow.bag_pool_num IS NOT NULL AND
         NOT del_pools.EXISTS(curRow.bag_pool_num) THEN
        del_pools(curRow.bag_pool_num):=TRUE;
      END IF;
      IF curRow.refuse='А' THEN
        deleted:=deleted+1;
        DELETE FROM pax_events WHERE pax_id=curRow.pax_id;
        DELETE FROM stat_ad WHERE pax_id=curRow.pax_id;
        DELETE FROM confirm_print WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doco WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doca WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_fqt WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_asvc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_emd WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_norms WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_brands WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_rem WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_rem_origin WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_seats WHERE pax_id=curRow.pax_id;
        DELETE FROM rozysk WHERE pax_id=curRow.pax_id;
        DELETE FROM transfer_subcls WHERE pax_id=curRow.pax_id;
        DELETE FROM trip_comp_layers WHERE pax_id=curRow.pax_id;
        UPDATE service_payment SET pax_id=NULL WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_alarms WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_custom_alarms WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_service_lists WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_services WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_services_auto WHERE pax_id=curRow.pax_id;
        DELETE FROM paid_rfisc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_norms_text WHERE pax_id=curRow.pax_id;
        DELETE FROM trfer_pax_stat WHERE pax_id=curRow.pax_id;
        DELETE FROM bi_stat WHERE pax_id=curRow.pax_id;
        DELETE FROM pax WHERE pax_id=curRow.pax_id;
        FOR langCurRow IN langCur LOOP
          UPDATE events_bilingual SET id2=NULL
          WHERE lang=langCurRow.lang AND type IN (system.evtPax, system.evtPay) AND id1=vpoint_dep AND id2=curRow.reg_no AND id3=vgrp_id AND
                NOT EXISTS(SELECT reg_no FROM pax WHERE grp_id=vgrp_id AND reg_no=curRow.reg_no);  --из-за возможного дублирования reg_no
        END LOOP;
      ELSE
        IF curRow.bag_pool_num IS NOT NULL THEN
          del_pools(curRow.bag_pool_num):=FALSE;
        END IF;
        IF curRow.refuse IS NOT NULL THEN
          refused:=refused+1;
        ELSE
          checked:=checked+1;
        END IF;
      END IF;
    END LOOP;
    IF deleted>0 AND checked+refused>0 THEN
      /* надо удалить багаж, который привязан к удаленным пассажирам */
      j:=1;
      i:=del_pools.FIRST;
      WHILE i IS NOT NULL LOOP
        IF del_pools(i) THEN
          /*пул удален*/
          FOR bagPoolCurRow IN bagPoolCur(vgrp_id, i) LOOP
            DELETE FROM bag_tags WHERE grp_id=vgrp_id AND bag_num=bagPoolCurRow.num;
            DELETE FROM value_bag WHERE grp_id=vgrp_id AND num=bagPoolCurRow.value_bag_num;
            DELETE FROM unaccomp_bag_info WHERE grp_id=vgrp_id AND num=bagPoolCurRow.value_bag_num;
            DELETE FROM bag2 WHERE grp_id=vgrp_id AND num=bagPoolCurRow.num;
            /*value_bag, paid_bag?*/
          END LOOP;
        ELSE
          /*пул не удален*/
          IF i<>j THEN
            UPDATE pax SET bag_pool_num=j, tid=cycle_tid__seq.currval WHERE grp_id=vgrp_id AND bag_pool_num=i;
            UPDATE bag2 SET bag_pool_num=j WHERE grp_id=vgrp_id AND bag_pool_num=i;
          END IF;
          j:=j+1;
        END IF;
        i:=del_pools.NEXT(i);
      END LOOP;
      j:=1;
      FOR valueBagCurRow IN valueBagCur(vgrp_id) LOOP
        IF valueBagCurRow.num<>j THEN
          UPDATE value_bag SET num=j WHERE grp_id=vgrp_id AND num=valueBagCurRow.num;
          UPDATE bag2 SET value_bag_num=j WHERE grp_id=vgrp_id AND value_bag_num=valueBagCurRow.num;
        END IF;
        j:=j+1;
      END LOOP;
      j:=1;
      FOR bagCurRow IN bagCur(vgrp_id) LOOP
        IF bagCurRow.num<>j THEN
          UPDATE bag2 SET num=j WHERE grp_id=vgrp_id AND num=bagCurRow.num;
          UPDATE bag_tags SET bag_num=j WHERE grp_id=vgrp_id AND bag_num=bagCurRow.num;
        END IF;
        j:=j+1;
      END LOOP;
      j:=1;
      FOR tagCurRow IN tagCur(vgrp_id) LOOP
        IF tagCurRow.num<>j THEN
          UPDATE bag_tags SET num=j WHERE grp_id=vgrp_id AND num=tagCurRow.num;
        END IF;
        j:=j+1;
      END LOOP;
      /*удалить из pax_norms, paid_bag???*/
    END IF;
    IF checked=0 AND refused>0 THEN
      UPDATE pax_grp SET bag_refuse=1 WHERE grp_id=vgrp_id;
    END IF;
  ELSE
    checked:=0;
    refused:=0;
  END IF;
  IF NOT(is_unaccomp) AND checked+refused=0 OR
     is_unaccomp AND vbag_refuse<>0 THEN
    DELETE FROM bag_prepay WHERE grp_id=vgrp_id;
    UPDATE bag_receipts SET grp_id=NULL WHERE grp_id=vgrp_id;
    DELETE FROM bag_tags WHERE grp_id=vgrp_id;
    DELETE FROM bag_tags_generated WHERE grp_id=vgrp_id;
    DELETE FROM unaccomp_bag_info WHERE grp_id=vgrp_id;
    DELETE FROM bag2 WHERE grp_id=vgrp_id;
    DELETE FROM grp_norms WHERE grp_id=vgrp_id;
    DELETE FROM paid_bag WHERE grp_id=vgrp_id;
    DELETE FROM paid_bag_emd_props WHERE grp_id=vgrp_id;
    DELETE FROM service_payment WHERE grp_id=vgrp_id;
    DELETE FROM tckin_pax_grp WHERE grp_id=vgrp_id;
    i:=delete_grp_trfer(vgrp_id);
    i:=delete_grp_tckin_segs(vgrp_id);
    DELETE FROM value_bag WHERE grp_id=vgrp_id;
    DELETE FROM pnr_addrs_pc WHERE grp_id=vgrp_id;
    DELETE FROM grp_service_lists WHERE grp_id=vgrp_id;
    DELETE FROM pax_grp WHERE grp_id=vgrp_id;
    --не чистим mark_trips потому что будет слишком долгая проверка pax_grp.point_id_mark
    FOR langCurRow IN langCur LOOP
        UPDATE events_bilingual SET id2=NULL
        WHERE lang=langCurRow.lang AND type=system.evtPax AND id1=vpoint_dep AND id3=vgrp_id;
    END LOOP;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END check_grp;

FUNCTION get_main_pax_id2(vgrp_id IN pax_grp.grp_id%TYPE,
                          include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM pax
    WHERE grp_id=vgrp_id
    ORDER BY DECODE(bag_pool_num,NULL,1,0),
             DECODE(pers_type,'ВЗ',0,'РБ',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'ВЗ',0,'РБ',1,2),
             reg_no;
curRow	cur%ROWTYPE;
res	pax.pax_id%TYPE;
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

FUNCTION get_bag_pool_pax_id(vgrp_id       IN pax.grp_id%TYPE,
                             vbag_pool_num IN pax.bag_pool_num%TYPE,
                             include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM pax
    WHERE grp_id=vgrp_id AND bag_pool_num=vbag_pool_num
    ORDER BY DECODE(pers_type,'ВЗ',0,'РБ',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'ВЗ',0,'РБ',1,2),
             reg_no;
curRow	cur%ROWTYPE;
res	pax.pax_id%TYPE;
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

FUNCTION bag_pool_refused(vgrp_id       IN bag2.grp_id%TYPE,
                          vbag_pool_num IN bag2.bag_pool_num%TYPE,
                          vclass        IN pax_grp.class%TYPE,
                          vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER
IS
n NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 1; END IF;
  IF vclass IS NULL THEN RETURN 0; END IF;
  SELECT SUM(DECODE(refuse,NULL,1,0)) INTO n FROM pax
  WHERE grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
  IF n IS NULL THEN
    RETURN 1;
--    raise_application_error(-20000,'bag_pool_refused: Data integrity is broken (grp_id='||vgrp_id||', bag_pool_num='||vbag_pool_num||')');
  END IF;
  IF n>0 THEN RETURN 0; ELSE RETURN 1; END IF;
END bag_pool_refused;

FUNCTION bag_pool_boarded(vgrp_id       IN bag2.grp_id%TYPE,
                          vbag_pool_num IN bag2.bag_pool_num%TYPE,
                          vclass        IN pax_grp.class%TYPE,
                          vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER
IS
n NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 0; END IF;
  IF vclass IS NULL THEN RETURN 1; END IF;
  SELECT SUM(DECODE(pr_brd,NULL,0,0,0,1)) INTO n FROM pax
  WHERE grp_id=vgrp_id AND bag_pool_num=vbag_pool_num;
  IF n IS NULL THEN
    RETURN 0;
--    raise_application_error(-20000,'bag_pool_boarded: Data integrity is broken (grp_id='||vgrp_id||', bag_pool_num='||vbag_pool_num||')');
  END IF;
  IF n>0 THEN RETURN 1; ELSE RETURN 0; END IF;
END bag_pool_boarded;

FUNCTION excess_boarded(vgrp_id       IN pax_grp.grp_id%TYPE,
                        vclass        IN pax_grp.class%TYPE,
                        vbag_refuse   IN pax_grp.bag_refuse%TYPE) RETURN NUMBER
IS
n NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 0; END IF;
  IF vclass IS NULL THEN RETURN 1; END IF;
  SELECT SUM(DECODE(pr_brd,NULL,0,0,0,1)) INTO n FROM pax
  WHERE grp_id=vgrp_id;
  IF n IS NULL THEN
    RETURN 0;
--    raise_application_error(-20000,'excess_boarded: Data integrity is broken (grp_id='||vgrp_id||')');
  END IF;
  IF n>0 THEN RETURN 1; ELSE RETURN 0; END IF;
END excess_boarded;

PROCEDURE delete_typeb_data(vpoint_id  tlg_trips.point_id%TYPE,
                            vsystem    typeb_sender_systems.system%TYPE,
                            vsender    typeb_sender_systems.sender%TYPE,
                            delete_trip_comp_layers BOOLEAN)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
pnrids  TIdsTable;
paxids  TIdsTable;
i       BINARY_INTEGER;
BEGIN
  SELECT pnr_id BULK COLLECT INTO pnrids
  FROM crs_pnr
  WHERE point_id=vpoint_id AND
        (vsystem IS NULL OR system=vsystem) AND
        (vsender IS NULL OR sender=vsender);
  SELECT pax_id BULK COLLECT INTO paxids
  FROM crs_pnr,crs_pax
  WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND
        crs_pnr.point_id=vpoint_id AND
        (vsystem IS NULL OR system=vsystem) AND
        (vsender IS NULL OR sender=vsender);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_seats_blocking WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_inf WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_inf_deleted WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_rem WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_doc WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_doco WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_doca WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_tkn WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_fqt WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_chkd WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_asvc WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_refuse WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM dcs_bag WHERE pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM dcs_tags WHERE pax_id=paxids(i);

  IF delete_trip_comp_layers THEN
    FORALL i IN 1..paxids.COUNT
      DELETE FROM trip_comp_layers WHERE crs_pax_id=paxids(i);
  END IF;
  FORALL i IN 1..paxids.COUNT
    DELETE FROM tlg_comp_layers WHERE crs_pax_id=paxids(i);
  FORALL i IN 1..paxids.COUNT
    DELETE FROM crs_pax_alarms WHERE pax_id=paxids(i);

  FORALL i IN 1..pnrids.COUNT
    DELETE FROM pnr_addrs WHERE pnr_id=pnrids(i);
  FORALL i IN 1..pnrids.COUNT
    DELETE FROM crs_transfer WHERE pnr_id=pnrids(i);
  FORALL i IN 1..pnrids.COUNT
    DELETE FROM crs_pax WHERE pnr_id=pnrids(i);
  FORALL i IN 1..pnrids.COUNT
    DELETE FROM pnr_market_flt WHERE pnr_id=pnrids(i);

  DELETE FROM crs_pnr
  WHERE point_id=vpoint_id AND
        (vsystem IS NULL OR system=vsystem) AND
        (vsender IS NULL OR sender=vsender);
  DELETE FROM crs_data
  WHERE point_id=vpoint_id AND
        (vsystem IS NULL OR system=vsystem) AND
        (vsender IS NULL OR sender=vsender);
  DELETE FROM crs_rbd
  WHERE point_id=vpoint_id AND
        (vsystem IS NULL OR system=vsystem) AND
        (vsender IS NULL OR sender=vsender);
END delete_typeb_data;

PROCEDURE set_additional_list_id(vgrp_id IN pax.grp_id%TYPE)
IS
CURSOR cur IS
  SELECT pax_service_lists.pax_id,
         pax_service_lists.category,
         pax_service_lists.transfer_num,
         pax_service_lists.list_id,
         pax_norms_text.airline
  FROM pax, pax_service_lists, pax_norms_text, service_lists
  WHERE pax.pax_id=pax_service_lists.pax_id AND
        pax_service_lists.pax_id=pax_norms_text.pax_id AND
        pax_service_lists.transfer_num=pax_norms_text.transfer_num AND
        pax_service_lists.category=DECODE(pax_norms_text.carry_on,0,1,2) AND
        pax_norms_text.lang='RU' AND pax_norms_text.page_no=1 AND
        pax_service_lists.list_id=service_lists.id AND service_lists.rfisc_used<>0 AND
        pax.grp_id=vgrp_id
  ORDER BY pax_id, category, transfer_num;
zeroRow cur%ROWTYPE;
BEGIN
  FOR curRow IN cur LOOP
    IF curRow.transfer_num=0 THEN
      zeroRow:=curRow;
    ELSE
      IF zeroRow.pax_id=curRow.pax_id AND
         zeroRow.category=curRow.category AND
         zeroRow.list_id<>curRow.list_id AND
         zeroRow.airline<>curRow.airline THEN
        UPDATE pax_service_lists
        SET additional_list_id=zeroRow.list_id
        WHERE pax_id=curRow.pax_id AND transfer_num=curRow.transfer_num AND category=curRow.category;
        UPDATE service_lists_group SET last_access=SYSTEM.UTCSYSDATE
        WHERE list_id=curRow.list_id AND additional_list_id=zeroRow.list_id;
        IF SQL%NOTFOUND THEN
          BEGIN
            INSERT INTO service_lists_group(term_list_id, list_id, additional_list_id, last_access)
            VALUES(service_lists_group__seq.nextval, curRow.list_id, zeroRow.list_id, SYSTEM.UTCSYSDATE);
          EXCEPTION
            WHEN DUP_VAL_ON_INDEX THEN NULL;
          END;
        END IF;
      END IF;
    END IF;
  END LOOP;
END set_additional_list_id;

END ckin;
/
