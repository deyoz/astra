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
  WHERE vfirst_point IN (point_id,first_point) AND point_num<vpoint_num AND pr_del=0; --rownum ����� ���� ࠧ���
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
        /*��� �� ��㯯 ����� ॣ����஢����� � �ନ���� ��� ��易⥫쭮� �ਢ離� */
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
                          vexcess        IN pax_grp.excess%TYPE,
                          vexcess_wt     IN pax_grp.excess_wt%TYPE,
                          vexcess_pc     IN pax_grp.excess_pc%TYPE,
                          vpax_id        IN pax.pax_id%TYPE) RETURN NUMBER
IS
res NUMBER;
BEGIN
  IF vbag_refuse<>0 THEN RETURN 0; END IF;

  IF vpiece_concept=0 THEN
    IF vexcess>0 THEN RETURN 1; END IF;
  ELSE
    IF vpax_id IS NOT NULL THEN
      IF vexcess_wt IS NULL AND vexcess_pc IS NULL THEN
        SELECT COUNT(*)
        INTO res
        FROM paid_bag_pc
        WHERE pax_id=vpax_id AND
              status IN ('unknown', 'paid', 'need') AND
              rownum<2;
      ELSE
        SELECT COUNT(*)
        INTO res
        FROM paid_rfisc
        WHERE pax_id=vpax_id AND paid>0 AND rownum<2;
      END IF;

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

FUNCTION get_excess(vgrp_id       IN pax.grp_id%TYPE,
                    vpax_id       IN pax.pax_id%TYPE) RETURN NUMBER
IS
vexcess         pax_grp.excess%TYPE;
vexcess_wt      pax_grp.excess_wt%TYPE;
vexcess_pc      pax_grp.excess_pc%TYPE;
vpiece_concept  pax_grp.piece_concept%TYPE;
main_pax_id     pax.pax_id%TYPE;
BEGIN
  vexcess:=0;
  BEGIN
    SELECT NVL(piece_concept,0), DECODE(bag_refuse,0,excess,0), excess_wt, excess_pc
    INTO vpiece_concept, vexcess, vexcess_wt, vexcess_pc
    FROM pax_grp WHERE grp_id=vgrp_id;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN RETURN NULL;
  END;

  IF vpiece_concept=0 THEN
    IF vpax_id IS NOT NULL THEN
      main_pax_id:=get_main_pax_id2(vgrp_id);
    END IF;
    IF vpax_id IS NULL OR
       main_pax_id IS NOT NULL AND main_pax_id=vpax_id THEN
      NULL;
    ELSE
      vexcess:=NULL;
    END IF;
  ELSE
    IF vpax_id IS NOT NULL THEN
      IF vexcess_wt IS NULL AND vexcess_pc IS NULL THEN
        SELECT COUNT(*)
        INTO vexcess
        FROM paid_bag_pc
        WHERE pax_id=vpax_id AND
              transfer_num=0 AND
              status IN ('unknown', 'paid', 'need');
      ELSE
        SELECT NVL(SUM(paid), 0)
        INTO vexcess
        FROM paid_rfisc
        WHERE pax_id=vpax_id AND paid>0 AND transfer_num=0;
      END IF;
    END IF;
  END IF;
  IF vexcess=0 THEN vexcess:=NULL; END IF;
  RETURN vexcess;
END get_excess;

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

FUNCTION get_max_crs_priority(vairline       IN crs_set.airline%TYPE,
                              vflt_no        IN crs_set.flt_no%TYPE,
                              vairp_dep      IN crs_set.airp_dep%TYPE) RETURN NUMBER
IS
CURSOR cur IS
  SELECT priority,crs FROM crs_set
  WHERE airline=vairline AND
        (flt_no=vflt_no OR flt_no IS NULL) AND
        (airp_dep=vairp_dep OR airp_dep IS NULL)
  ORDER BY crs,flt_no,airp_dep;
curRow		cur%ROWTYPE;
oldRow		cur%ROWTYPE;
vmax		crs_set.priority%TYPE;
BEGIN
  vmax:=NULL;
  FOR curRow IN cur LOOP
    IF oldRow.crs IS NULL OR
       oldRow.crs IS NOT NULL AND oldRow.crs<>curRow.crs THEN
      IF vmax IS NULL OR
         vmax IS NOT NULL AND vmax<curRow.priority THEN
        vmax:=curRow.priority;
      END IF;
    END IF;
    oldRow:=curRow;
  END LOOP;
  RETURN vmax;
END get_max_crs_priority;

PROCEDURE crs_recount(vpoint_id	IN points.point_id%TYPE)
IS
vairline       crs_set.airline%TYPE;
vflt_no        crs_set.flt_no%TYPE;
vairp_dep      crs_set.airp_dep%TYPE;
vpriority      crs_set.priority%TYPE;
BEGIN
  DELETE FROM crs_counters WHERE point_dep=vpoint_id;

  SELECT airline,flt_no,airp INTO vairline,vflt_no,vairp_dep
  FROM points WHERE point_id=vpoint_id AND pr_del>=0;

    -- ��室�� ���ᨬ���� �ਮ���
  vpriority:=get_max_crs_priority(vairline,vflt_no,vairp_dep);

  INSERT INTO crs_counters(point_dep,airp_arv,class,crs_tranzit,crs_ok)
  SELECT vpoint_id,airp_arv,class,NVL(SUM(tranzit),0),NVL(SUM(resa),0)
  FROM
    (SELECT sender,airp_arv,class,SUM(tranzit) AS tranzit,SUM(resa) AS resa
     FROM crs_data,tlg_binding
     WHERE crs_data.point_id=tlg_binding.point_id_tlg AND
           point_id_spp=vpoint_id AND crs_data.system='CRS'
     GROUP BY sender,airp_arv,class)
  WHERE NVL(get_crs_priority(sender,vairline,vflt_no,vairp_dep),0)=NVL(vpriority,0)
  GROUP BY airp_arv,class
  HAVING SUM(tranzit) IS NOT NULL OR SUM(resa) IS NOT NULL;

  recount(vpoint_id);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END crs_recount;

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

PROCEDURE recount(vpoint_id	IN points.point_id%TYPE)
IS

vairline         points.airline%TYPE;
vflt_no          points.flt_no%TYPE;
vsuffix          points.suffix%TYPE;
vairp            points.airp%TYPE;
vpriority        crs_set.priority%TYPE;
vpr_tranzit      points.pr_tranzit%TYPE;
vpoint_num       points.point_num%TYPE;
vfirst_point     points.first_point%TYPE;
vpr_tranz_reg    trip_sets.pr_tranz_reg%TYPE;
vpr_free_seating trip_sets.pr_free_seating%TYPE;
vuse_jmp         trip_sets.use_jmp%TYPE;
vjmp_cfg         trip_sets.jmp_cfg%TYPE;
cfg_exists       BINARY_INTEGER;

CURSOR cur1 IS
  SELECT counters2.class,
         crs_tranzit AS crs_tr,crs_ok,
         tranzit AS tr, ok, goshow,
         jmp_tranzit AS jmp_tr, jmp_ok, jmp_goshow,
         NVL(cfg,0) AS cfg, NVL(block,0) AS block, NVL(prot,0) AS prot
  FROM counters2,points,trip_classes
  WHERE counters2.point_dep=trip_classes.point_id(+) AND
        counters2.class=trip_classes.class(+) AND
        counters2.point_arv=points.point_id AND
        counters2.point_dep=vpoint_id
  ORDER BY counters2.class,points.point_num DESC;
CURSOR cur2 IS
  SELECT airp_arv,class,
         0 AS priority,
         crs_ok AS resa,
         crs_tranzit AS tranzit
  FROM crs_counters
  WHERE point_dep=vpoint_id
  UNION
  SELECT airp_arv,class,1,resa,tranzit
  FROM trip_data
  WHERE point_id=vpoint_id
  ORDER BY airp_arv,class,priority DESC;
cur2Row     cur2%ROWTYPE;
vairp_arv   crs_data.airp_arv%TYPE;
vclass      crs_data.class%TYPE;

CURSOR cur4(vfirst_point        IN points.first_point%TYPE,
            vpoint_num          IN points.point_num%TYPE,
            vairp               IN points.airp%TYPE) IS
  SELECT point_id FROM points
  WHERE first_point=vfirst_point AND point_num>vpoint_num AND pr_del=0 AND airp=vairp
  ORDER BY point_num;
ctrsRow cur1%ROWTYPE;
ctrsOld cur1%ROWTYPE;
double_tr counters2.crs_tranzit%TYPE;
double_ok counters2.crs_ok%TYPE;

TYPE TSum IS RECORD
  (crs_tr       counters2.crs_tranzit%TYPE,
   crs_ok       counters2.crs_ok%TYPE,
   double_tr    counters2.crs_tranzit%TYPE,
   double_ok    counters2.crs_ok%TYPE,
   tr           counters2.tranzit%TYPE,
   ok           counters2.ok%TYPE,
   goshow       counters2.goshow%TYPE,
   jmp_tr       counters2.jmp_tranzit%TYPE,
   jmp_ok       counters2.jmp_ok%TYPE,
   jmp_goshow   counters2.jmp_goshow%TYPE
  );
vsum TSum;

sumRow counters2%ROWTYPE;

BEGIN
  DELETE FROM counters2 WHERE point_dep=vpoint_id;

  SELECT point_num,airp,pr_tranzit,DECODE(pr_tranzit,0,point_id,first_point),
         airline,flt_no,suffix,get_pr_tranz_reg(point_id)
  INTO vpoint_num,vairp,vpr_tranzit,vfirst_point,
       vairline,vflt_no,vsuffix,vpr_tranz_reg
  FROM points WHERE point_id=vpoint_id AND pr_del>=0;

  SELECT COUNT(*) INTO cfg_exists FROM trip_classes WHERE point_id=vpoint_id AND cfg>0 AND rownum<2;
  vpr_free_seating:=0;
  vuse_jmp:=0;
  vjmp_cfg:=0;
  BEGIN
    SELECT pr_free_seating, NVL(use_jmp, 0), NVL(jmp_cfg, 0)
    INTO vpr_free_seating, vuse_jmp, vjmp_cfg
    FROM trip_sets
    WHERE point_id=vpoint_id;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN NULL;
  END;


  INSERT INTO counters2(point_dep,point_arv,class,crs_tranzit,crs_ok,avail,
                        tranzit,ok,goshow,
                        jmp_tranzit,jmp_ok,jmp_goshow,
                        free_ok,free_goshow,nooccupy,jmp_nooccupy)
  SELECT vpoint_id,main.point_arv,main.class,0,0,0,
         NVL(pax.trok,0),NVL(pax.ok,0),NVL(pax.goshow,0),
         NVL(pax.jmp_trok,0),NVL(pax.jmp_ok,0),NVL(pax.jmp_goshow,0),
         0,0,0,0
  FROM
       (SELECT points.point_id AS point_arv,point_num,classes.code AS class
        FROM points,classes,trip_classes
        WHERE classes.code=trip_classes.class(+) AND trip_classes.point_id(+)=vpoint_id AND
              points.first_point=vfirst_point AND points.point_num>vpoint_num AND points.pr_del=0 AND
              (trip_classes.point_id IS NOT NULL OR cfg_exists=0 AND vpr_free_seating<>0)) main,
       (SELECT pax_grp.point_arv,pax_grp.class,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, DECODE(pax_grp.status,'T',pax.seats,0), 0)) AS trok,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, DECODE(pax_grp.status,'K',pax.seats,'C',pax.seats,0), 0)) AS ok,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, DECODE(pax_grp.status,'P',pax.seats,0), 0)) AS goshow,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, 0, DECODE(pax_grp.status,'T',pax.seats,0))) AS jmp_trok,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, 0, DECODE(pax_grp.status,'K',pax.seats,'C',pax.seats,0))) AS jmp_ok,
               SUM(DECODE(NVL(pax.is_jmp, 0), 0, 0, DECODE(pax_grp.status,'P',pax.seats,0))) AS jmp_goshow
        FROM pax_grp,pax
        WHERE pax_grp.grp_id=pax.grp_id AND
              pax_grp.point_dep=vpoint_id AND
              pax_grp.status NOT IN ('E') AND
              pax.pr_brd IS NOT NULL
        GROUP BY point_arv,class) pax
  WHERE main.point_arv=pax.point_arv(+) AND main.class=pax.class(+);

  vairp_arv:=NULL;
  vclass:=NULL;
  FOR cur2Row IN cur2 LOOP
    IF vairp_arv IS NULL OR vclass IS NULL OR
       vairp_arv<>cur2Row.airp_arv OR vclass<>cur2Row.class THEN
      vairp_arv:=cur2Row.airp_arv;
      vclass:=cur2Row.class;
      FOR cur4Row IN cur4(vfirst_point,vpoint_num,vairp_arv) LOOP
        UPDATE counters2
        SET crs_ok=crs_ok+cur2Row.resa,crs_tranzit=crs_tranzit+cur2Row.tranzit
        WHERE point_dep=vpoint_id AND point_arv=cur4Row.point_id AND class=vclass;
        EXIT;
      END LOOP;
    END IF;
  END LOOP;

  UPDATE counters2
  SET crs_tranzit=DECODE(vpr_tranzit,0,0,crs_tranzit),
      tranzit=    DECODE(vpr_tranzit,0,0,
                         DECODE(vpr_tranz_reg,0,
                                DECODE(SIGN(tranzit-crs_tranzit),-1,crs_tranzit,tranzit),tranzit)),
      jmp_tranzit=DECODE(vpr_tranzit,0,0,
                         DECODE(vpr_tranz_reg,0,0,jmp_tranzit))

  WHERE point_dep=vpoint_id;

  ctrsOld.class:=NULL;
  vsum.jmp_tr:=0;
  vsum.jmp_ok:=0;
  vsum.jmp_goshow:=0;
  OPEN cur1;
  FETCH cur1 INTO ctrsRow;
  IF (cur1%FOUND) THEN
    WHILE cur1%FOUND OR cur1%NOTFOUND LOOP
      IF ctrsOld.class IS NOT NULL AND ctrsOld.class<>ctrsRow.class OR cur1%NOTFOUND THEN
        UPDATE counters2
        SET avail=ctrsOld.cfg-ctrsOld.block-vsum.crs_tr-vsum.crs_ok,
            free_ok=ctrsOld.cfg-ctrsOld.block-vsum.crs_tr-vsum.double_tr-vsum.ok-vsum.goshow,
            free_goshow=ctrsOld.cfg-ctrsOld.block-vsum.crs_tr-vsum.crs_ok-
                        vsum.double_tr-vsum.double_ok-vsum.goshow,
            nooccupy=ctrsOld.cfg-ctrsOld.block-vsum.tr-vsum.ok-vsum.goshow
        WHERE point_dep=vpoint_id AND class=ctrsOld.class;
      END IF;
      EXIT WHEN cur1%NOTFOUND;
      IF cur1%FOUND THEN
        double_tr:=ctrsRow.tr-ctrsRow.crs_tr;
        IF double_tr<0 THEN double_tr:=0; END IF;
        double_ok:=ctrsRow.ok-ctrsRow.crs_ok;
        IF double_ok<0 THEN double_ok:=0; END IF;
        IF ctrsOld.class IS NULL OR ctrsOld.class<>ctrsRow.class THEN
          vsum.crs_tr:=0;
          vsum.crs_ok:=0;
          vsum.double_tr:=0;
          vsum.double_ok:=0;
          vsum.tr:=0;
          vsum.ok:=0;
          vsum.goshow:=0;
        END IF;
        vsum.crs_tr:=vsum.crs_tr+ctrsRow.crs_tr;
        vsum.crs_ok:=vsum.crs_ok+ctrsRow.crs_ok;
        vsum.double_tr:=vsum.double_tr+double_tr;
        vsum.double_ok:=vsum.double_ok+double_ok;
        vsum.tr:=vsum.tr+ctrsRow.tr;
        vsum.ok:=vsum.ok+ctrsRow.ok;
        vsum.goshow:=vsum.goshow+ctrsRow.goshow;
        vsum.jmp_tr:=vsum.jmp_tr+ctrsRow.jmp_tr;
        vsum.jmp_ok:=vsum.jmp_ok+ctrsRow.jmp_ok;
        vsum.jmp_goshow:=vsum.jmp_goshow+ctrsRow.jmp_goshow;
      END IF;
      ctrsOld:=ctrsRow;
      FETCH cur1 INTO ctrsRow;
    END LOOP;
  END IF;
  CLOSE cur1;

  UPDATE counters2
  SET jmp_nooccupy=DECODE(vuse_jmp, 0, 0, vjmp_cfg)-vsum.jmp_tr-vsum.jmp_ok-vsum.jmp_goshow
  WHERE point_dep=vpoint_id;

EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END recount;

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
    /* �� �� ����� ��� ᮯ஢������� */
    checked:=0;
    refused:=0;
    deleted:=0;
    FOR curRow IN cur(vgrp_id) LOOP
      IF curRow.bag_pool_num IS NOT NULL AND
         NOT del_pools.EXISTS(curRow.bag_pool_num) THEN
        del_pools(curRow.bag_pool_num):=TRUE;
      END IF;
      IF curRow.refuse='�' THEN
        deleted:=deleted+1;
        DELETE FROM confirm_print WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doco WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_doca WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_fqt WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_asvc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_emd WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_norms WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_norms_pc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_brands WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_rem WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_rem_origin WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_seats WHERE pax_id=curRow.pax_id;
        DELETE FROM rozysk WHERE pax_id=curRow.pax_id;
        DELETE FROM transfer_subcls WHERE pax_id=curRow.pax_id;
        DELETE FROM trip_comp_layers WHERE pax_id=curRow.pax_id;
        DELETE FROM paid_bag_pc WHERE pax_id=curRow.pax_id;
        UPDATE paid_bag_emd SET pax_id=NULL WHERE pax_id=curRow.pax_id;
        UPDATE service_payment SET pax_id=NULL WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_alarms WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_service_lists WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_services WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_services_auto WHERE pax_id=curRow.pax_id;
        DELETE FROM paid_rfisc WHERE pax_id=curRow.pax_id;
        DELETE FROM pax_norms_text WHERE pax_id=curRow.pax_id;
        DELETE FROM trfer_pax_stat WHERE pax_id=curRow.pax_id;
        DELETE FROM pax WHERE pax_id=curRow.pax_id;
        FOR langCurRow IN langCur LOOP
          UPDATE events_bilingual SET id2=NULL
          WHERE lang=langCurRow.lang AND type IN (system.evtPax, system.evtPay) AND id1=vpoint_dep AND id2=curRow.reg_no AND id3=vgrp_id AND
                NOT EXISTS(SELECT reg_no FROM pax WHERE grp_id=vgrp_id AND reg_no=curRow.reg_no);  --��-�� ���������� �㡫�஢���� reg_no
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
      /* ���� 㤠���� �����, ����� �ਢ易� � 㤠����� ���ᠦ�ࠬ */
      j:=1;
      i:=del_pools.FIRST;
      WHILE i IS NOT NULL LOOP
        IF del_pools(i) THEN
          /*�� 㤠���*/
          FOR bagPoolCurRow IN bagPoolCur(vgrp_id, i) LOOP
            DELETE FROM bag_tags WHERE grp_id=vgrp_id AND bag_num=bagPoolCurRow.num;
            DELETE FROM value_bag WHERE grp_id=vgrp_id AND num=bagPoolCurRow.value_bag_num;
            DELETE FROM unaccomp_bag_info WHERE grp_id=vgrp_id AND num=bagPoolCurRow.value_bag_num;
            DELETE FROM bag2 WHERE grp_id=vgrp_id AND num=bagPoolCurRow.num;
            /*value_bag, paid_bag?*/
          END LOOP;
        ELSE
          /*�� �� 㤠���*/
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
      /*㤠���� �� pax_norms, paid_bag???*/
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
    DELETE FROM unaccomp_bag_info WHERE grp_id=vgrp_id;
    DELETE FROM bag2 WHERE grp_id=vgrp_id;
    DELETE FROM grp_norms WHERE grp_id=vgrp_id;
    DELETE FROM paid_bag WHERE grp_id=vgrp_id;
    DELETE FROM paid_bag_emd WHERE grp_id=vgrp_id;
    DELETE FROM paid_bag_emd_props WHERE grp_id=vgrp_id;
    DELETE FROM service_payment WHERE grp_id=vgrp_id;
    DELETE FROM tckin_pax_grp WHERE grp_id=vgrp_id;
    i:=delete_grp_trfer(vgrp_id);
    i:=delete_grp_tckin_segs(vgrp_id);
    DELETE FROM value_bag WHERE grp_id=vgrp_id;
    DELETE FROM pnr_addrs_pc WHERE grp_id=vgrp_id;
    DELETE FROM grp_service_lists WHERE grp_id=vgrp_id;
    DELETE FROM pax_grp WHERE grp_id=vgrp_id;
    --�� ��⨬ mark_trips ��⮬� �� �㤥� ᫨誮� ������ �஢�ઠ pax_grp.point_id_mark
    FOR langCurRow IN langCur LOOP
        UPDATE events_bilingual SET id2=NULL
        WHERE lang=langCurRow.lang AND type=system.evtPax AND id1=vpoint_dep AND id3=vgrp_id;
    END LOOP;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END check_grp;

FUNCTION get_main_pax_id(vgrp_id IN pax_grp.grp_id%TYPE,
                         include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM pax
    WHERE grp_id=vgrp_id
    ORDER BY DECODE(refuse,NULL,0,1),
             DECODE(seats,0,1,0),
             DECODE(pers_type,'��',0,'��',1,2),
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
END get_main_pax_id;

FUNCTION get_main_pax_id2(vgrp_id IN pax_grp.grp_id%TYPE,
                          include_refused IN NUMBER DEFAULT 1) RETURN pax.pax_id%TYPE
IS
  CURSOR cur IS
    SELECT pax_id,refuse FROM pax
    WHERE grp_id=vgrp_id
    ORDER BY DECODE(bag_pool_num,NULL,1,0),
             DECODE(pers_type,'��',0,'��',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'��',0,'��',1,2),
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
    ORDER BY DECODE(pers_type,'��',0,'��',0,1),
             DECODE(seats,0,1,0),
             DECODE(refuse,NULL,0,1),
             DECODE(pers_type,'��',0,'��',1,2),
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
    DELETE FROM crs_inf WHERE pax_id=paxids(i);
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

PROCEDURE save_pax_docs(vpax_id     IN pax.pax_id%TYPE,
                        vdocument   IN VARCHAR2,
                        full_insert IN NUMBER DEFAULT 1)
IS
  CURSOR cur1 IS
    SELECT type,issue_country,no,nationality,
           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi,
           type_rcpt
    FROM crs_pax_doc
    WHERE pax_id=vpax_id AND no=vdocument
    ORDER BY DECODE(type,'P',0,NULL,2,1), DECODE(rem_code,'DOCS',0,1), no;
  row1 cur1%ROWTYPE;
  CURSOR cur2 IS
    SELECT birth_place,type,no,issue_place,issue_date,applic_country
    FROM crs_pax_doco
    WHERE pax_id=vpax_id AND rem_code='DOCO' AND type='V'
    ORDER BY no;
  row2 cur2%ROWTYPE;
BEGIN
  IF full_insert IS NULL THEN RETURN; END IF;
  IF full_insert<>0 THEN
    DELETE FROM pax_doc WHERE pax_id=vpax_id;
    DELETE FROM pax_doco WHERE pax_id=vpax_id;
    IF vdocument IS NOT NULL THEN
      OPEN cur1;
      FETCH cur1 INTO row1;
      IF cur1%FOUND THEN
        INSERT INTO pax_doc
          (pax_id,type,issue_country,no,nationality,
           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi,
           type_rcpt,scanned_attrs)
        VALUES
          (vpax_id,row1.type,row1.issue_country,row1.no,row1.nationality,
           row1.birth_date,row1.gender,row1.expiry_date,row1.surname,row1.first_name,row1.second_name,row1.pr_multi,
           row1.type_rcpt,0);
      ELSE
        INSERT INTO pax_doc
          (pax_id,no,pr_multi,scanned_attrs)
        VALUES
          (vpax_id,vdocument,0,0);
      END IF;
      CLOSE cur1;
    END IF;
    OPEN cur2;
    FETCH cur2 INTO row2;
    IF cur2%FOUND THEN
      INSERT INTO pax_doco
        (pax_id,birth_place,type,no,issue_place,issue_date,applic_country,scanned_attrs)
      VALUES
        (vpax_id,row2.birth_place,row2.type,row2.no,row2.issue_place,row2.issue_date,row2.applic_country,0);
    END IF;
    CLOSE cur2;
  ELSE
    IF vdocument IS NOT NULL THEN
      OPEN cur1;
      FETCH cur1 INTO row1;
      IF cur1%FOUND THEN
        UPDATE pax_doc SET type_rcpt=row1.type_rcpt WHERE pax_id=vpax_id;
      END IF;
      CLOSE cur1;
    END IF;
  END IF;
END;

END ckin;
/
