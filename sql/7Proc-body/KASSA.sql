create or replace PACKAGE BODY kassa
AS

PROCEDURE check_period(pr_new           BOOLEAN,
                       vfirst_date      DATE,
                       vlast_date       DATE,
                       vnow             DATE,
                       first        OUT DATE,
                       last         OUT DATE,
                       pr_opd       OUT BOOLEAN)
IS
BEGIN
  IF (vfirst_date IS NULL) AND (vlast_date IS NULL) THEN
    system.raise_user_exception('MSG.TABLE.NOT_SET_RANGE');
  END IF;
  IF (vfirst_date IS NOT NULL) AND (vlast_date IS NOT NULL) AND
     (TRUNC(vfirst_date)>TRUNC(vlast_date)) THEN
    system.raise_user_exception('MSG.TABLE.INVALID_RANGE');
  END IF;

  first:=TRUNC(vfirst_date);
  IF first IS NOT NULL THEN
    IF first<TRUNC(vnow) THEN
      IF pr_new THEN
        system.raise_user_exception('MSG.TABLE.FIRST_DATE_BEFORE_TODAY');
      ELSE
        first:=TRUNC(vnow);
      END IF;
    END IF;
    IF first=TRUNC(vnow) THEN first:=vnow; END IF;
    pr_opd:=false;
  ELSE
    pr_opd:=true;
  END IF;
  last:=TRUNC(vlast_date);
  IF last IS NOT NULL THEN
    IF last<TRUNC(vnow) THEN
      system.raise_user_exception('MSG.TABLE.LAST_DATE_BEFORE_TODAY');
    END IF;
    last:=last+1;
  END IF;
  IF pr_opd THEN
    first:=last;
    last:=NULL;
  END IF;
END check_period;

PROCEDURE check_params(vid              bag_norms.id%TYPE,
                       vairline         airlines.code%TYPE,
                       vcity_dep        cities.code%TYPE,
                       vcity_arv        cities.code%TYPE,
                       vfirst_date      DATE,
                       vlast_date       DATE,
                       first        OUT DATE,
                       last         OUT DATE,
                       pr_opd       OUT BOOLEAN)
IS
BEGIN
/*  IF vairline IS NULL THEN system.raise_user_exception('MSG.AIRLINE.NOT_SPECIFY'); END IF;
  IF (vcity_dep IS NULL) AND (vcity_arv IS NOT NULL) THEN
    system.raise_user_exception('MSG.NOT_SET_POINT_DEP');
  END IF;
  IF (vcity_dep IS NOT NULL) AND (vcity_arv IS NULL) THEN
    system.raise_user_exception('MSG.NOT_SET_POINT_ARV');
  END IF; */
  check_period(vid IS NULL,vfirst_date,vlast_date,system.UTCSYSDATE,first,last,pr_opd);
END check_params;

PROCEDURE modify_bag_norm(
       vid              bag_norms.id%TYPE,
       vlast_date       bag_norms.last_date%TYPE,
       vtid             bag_norms.tid%TYPE DEFAULT NULL)
IS
r bag_norms%ROWTYPE;
BEGIN
  SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         first_date,bag_type,amount,weight,per_unit,norm_type,extra
  INTO   r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,r.pax_cat,r.subclass,r.class,r.flt_no,r.craft,r.trip_type,
         r.first_date,r.bag_type,r.amount,r.weight,r.per_unit,r.norm_type,r.extra
  FROM bag_norms WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_bag_norm(r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,r.pax_cat,r.subclass,r.class,r.flt_no,r.craft,r.trip_type,
               r.first_date,vlast_date,r.bag_type,r.amount,r.weight,r.per_unit,r.norm_type,r.extra,vtid);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_bag_norm;

PROCEDURE modify_bag_rate(
       vid              bag_rates.id%TYPE,
       vlast_date       bag_rates.last_date%TYPE,
       vtid             bag_rates.tid%TYPE DEFAULT NULL)
IS
r bag_rates%ROWTYPE;
BEGIN
  SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         first_date,bag_type,rate,rate_cur,min_weight,extra
  INTO   r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,r.pax_cat,r.subclass,r.class,r.flt_no,r.craft,r.trip_type,
         r.first_date,r.bag_type,r.rate,r.rate_cur,r.min_weight,r.extra
  FROM bag_rates WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_bag_rate(r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,r.pax_cat,r.subclass,r.class,r.flt_no,r.craft,r.trip_type,
               r.first_date,vlast_date,r.bag_type,r.rate,r.rate_cur,r.min_weight,r.extra,vtid);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_bag_rate;

PROCEDURE modify_value_bag_tax(
       vid              value_bag_taxes.id%TYPE,
       vlast_date       value_bag_taxes.last_date%TYPE,
       vtid             value_bag_taxes.tid%TYPE DEFAULT NULL)
IS
r value_bag_taxes%ROWTYPE;
BEGIN
  SELECT id,airline,pr_trfer,city_dep,city_arv,
         first_date,tax,min_value,min_value_cur,extra
  INTO   r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,
         r.first_date,r.tax,r.min_value,r.min_value_cur,r.extra
  FROM value_bag_taxes WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_value_bag_tax(r.id,r.airline,r.pr_trfer,r.city_dep,r.city_arv,
                    r.first_date,vlast_date,r.tax,r.min_value,r.min_value_cur,r.extra,vtid);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_value_bag_tax;

PROCEDURE modify_exchange_rate(
       vid              exchange_rates.id%TYPE,
       vlast_date       exchange_rates.last_date%TYPE,
       vtid             exchange_rates.tid%TYPE DEFAULT NULL)
IS
r exchange_rates%ROWTYPE;
BEGIN
  SELECT id,airline,rate1,cur1,rate2,cur2,
         first_date,extra
  INTO   r.id,r.airline,r.rate1,r.cur1,r.rate2,r.cur2,
         r.first_date,r.extra
  FROM exchange_rates WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_exchange_rate(r.id,r.airline,r.rate1,r.cur1,r.rate2,r.cur2,
                    r.first_date,vlast_date,r.extra,vtid);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_exchange_rate;

PROCEDURE delete_bag_norm(
       vid              bag_norms.id%TYPE,
       vtid             bag_norms.tid%TYPE DEFAULT NULL)
IS
now             DATE;
vfirst_date     DATE;
vlast_date      DATE;
tidh    bag_norms.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM bag_norms
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>now THEN
    IF vfirst_date<now THEN
      UPDATE bag_norms SET last_date=now,tid=tidh WHERE id=vid;
    ELSE
      UPDATE bag_norms SET pr_del=1,tid=tidh WHERE id=vid;
    END IF;
  ELSE
    /* специально чтобы в кэше появилась неизмененная строка */
    UPDATE bag_norms SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_bag_norm;

PROCEDURE delete_bag_rate(
       vid              bag_rates.id%TYPE,
       vtid             bag_rates.tid%TYPE DEFAULT NULL)
IS
now             DATE;
vfirst_date     DATE;
vlast_date      DATE;
tidh    bag_rates.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM bag_rates
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>now THEN
    IF vfirst_date<now THEN
      UPDATE bag_rates SET last_date=now,tid=tidh WHERE id=vid;
    ELSE
      UPDATE bag_rates SET pr_del=1,tid=tidh WHERE id=vid;
    END IF;
  ELSE
    /* специально чтобы в кэше появилась неизмененная строка */
    UPDATE bag_rates SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_bag_rate;

PROCEDURE delete_value_bag_tax(
       vid              value_bag_taxes.id%TYPE,
       vtid             value_bag_taxes.tid%TYPE DEFAULT NULL)
IS
now             DATE;
vfirst_date     DATE;
vlast_date      DATE;
tidh    value_bag_taxes.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM value_bag_taxes
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>now THEN
    IF vfirst_date<now THEN
      UPDATE value_bag_taxes SET last_date=now,tid=tidh WHERE id=vid;
    ELSE
      UPDATE value_bag_taxes SET pr_del=1,tid=tidh WHERE id=vid;
    END IF;
  ELSE
    /* специально чтобы в кэше появилась неизмененная строка */
    UPDATE value_bag_taxes SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_value_bag_tax;

PROCEDURE delete_exchange_rate(
       vid              exchange_rates.id%TYPE,
       vtid             exchange_rates.tid%TYPE DEFAULT NULL)
IS
now             DATE;
vfirst_date     DATE;
vlast_date      DATE;
tidh    exchange_rates.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM exchange_rates
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>now THEN
    IF vfirst_date<now THEN
      UPDATE exchange_rates SET last_date=now,tid=tidh WHERE id=vid;
    ELSE
      UPDATE exchange_rates SET pr_del=1,tid=tidh WHERE id=vid;
    END IF;
  ELSE
    /* специально чтобы в кэше появилась неизмененная строка */
    UPDATE exchange_rates SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_exchange_rate;

PROCEDURE copy_basic_bag_norm(vairline         bag_norms.airline%TYPE)
IS
CURSOR cur IS
  SELECT id,first_date,last_date FROM bag_norms
  WHERE airline=vairline AND pr_del=0 FOR UPDATE;
curRow  cur%ROWTYPE;
now             DATE;
tidh    bag_norms.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT tid__seq.nextval INTO tidh FROM dual;
  FOR curRow IN cur LOOP
    IF curRow.last_date IS NULL OR curRow.last_date>now THEN
      IF curRow.first_date<now THEN
        UPDATE bag_norms SET last_date=now,tid=tidh WHERE id=curRow.id;
      ELSE
        UPDATE bag_norms SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  INSERT INTO bag_norms(id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,tid)
  SELECT id__seq.nextval,vairline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         DECODE(SIGN(now-first_date),1,now,first_date),last_date,
         bag_type,amount,weight,per_unit,norm_type,extra,pr_del,tidh
  FROM bag_norms WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date>now);
END copy_basic_bag_norm;

PROCEDURE copy_basic_bag_rate(vairline         bag_rates.airline%TYPE)
IS
CURSOR cur IS
  SELECT id,first_date,last_date FROM bag_rates
  WHERE airline=vairline AND pr_del=0 FOR UPDATE;
curRow  cur%ROWTYPE;
now             DATE;
tidh    bag_rates.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT tid__seq.nextval INTO tidh FROM dual;
  FOR curRow IN cur LOOP
    IF curRow.last_date IS NULL OR curRow.last_date>now THEN
      IF curRow.first_date<now THEN
        UPDATE bag_rates SET last_date=now,tid=tidh WHERE id=curRow.id;
      ELSE
        UPDATE bag_rates SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  INSERT INTO bag_rates(id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         first_date,last_date,bag_type,rate,rate_cur,min_weight,extra,pr_del,tid)
  SELECT id__seq.nextval,vairline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
         DECODE(SIGN(now-first_date),1,now,first_date),last_date,
         bag_type,rate,rate_cur,min_weight,extra,pr_del,tidh
  FROM bag_rates WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date>now);
END copy_basic_bag_rate;

PROCEDURE copy_basic_value_bag_tax(vairline         value_bag_taxes.airline%TYPE)
IS
CURSOR cur IS
  SELECT id,first_date,last_date FROM value_bag_taxes
  WHERE airline=vairline AND pr_del=0 FOR UPDATE;
curRow  cur%ROWTYPE;
now             DATE;
tidh    value_bag_taxes.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT tid__seq.nextval INTO tidh FROM dual;
  FOR curRow IN cur LOOP
    IF curRow.last_date IS NULL OR curRow.last_date>now THEN
      IF curRow.first_date<now THEN
        UPDATE value_bag_taxes SET last_date=now,tid=tidh WHERE id=curRow.id;
      ELSE
        UPDATE value_bag_taxes SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  INSERT INTO value_bag_taxes(id,airline,pr_trfer,city_dep,city_arv,
         first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid)
  SELECT id__seq.nextval,vairline,pr_trfer,city_dep,city_arv,
         DECODE(SIGN(now-first_date),1,now,first_date),last_date,
         tax,min_value,min_value_cur,extra,pr_del,tidh
  FROM value_bag_taxes WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date>now);
END copy_basic_value_bag_tax;

PROCEDURE copy_basic_exchange_rate(vairline         exchange_rates.airline%TYPE)
IS
CURSOR cur IS
  SELECT id,first_date,last_date FROM exchange_rates
  WHERE airline=vairline AND pr_del=0 FOR UPDATE;
curRow  cur%ROWTYPE;
now             DATE;
tidh    exchange_rates.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT tid__seq.nextval INTO tidh FROM dual;
  FOR curRow IN cur LOOP
    IF curRow.last_date IS NULL OR curRow.last_date>now THEN
      IF curRow.first_date<now THEN
        UPDATE exchange_rates SET last_date=now,tid=tidh WHERE id=curRow.id;
      ELSE
        UPDATE exchange_rates SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  INSERT INTO exchange_rates(id,airline,rate1,cur1,rate2,cur2,
         first_date,last_date,extra,pr_del,tid)
  SELECT id__seq.nextval,vairline,rate1,cur1,rate2,cur2,
         DECODE(SIGN(now-first_date),1,now,first_date),last_date,
         extra,pr_del,tidh
  FROM exchange_rates WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date>now);
END copy_basic_exchange_rate;

PROCEDURE add_bag_norm(
       vid       IN OUT bag_norms.id%TYPE,
       vairline         bag_norms.airline%TYPE,
       vpr_trfer        bag_norms.pr_trfer%TYPE,
       vcity_dep        bag_norms.city_dep%TYPE,
       vcity_arv        bag_norms.city_arv%TYPE,
       vpax_cat         bag_norms.pax_cat%TYPE,
       vsubclass        bag_norms.subclass%TYPE,
       vclass           bag_norms.class%TYPE,
       vflt_no          bag_norms.flt_no%TYPE,
       vcraft           bag_norms.craft%TYPE,
       vtrip_type       bag_norms.trip_type%TYPE,
       vfirst_date      bag_norms.first_date%TYPE,
       vlast_date       bag_norms.last_date%TYPE,
       vbag_type        bag_norms.bag_type%TYPE,
       vamount          bag_norms.amount%TYPE,
       vweight          bag_norms.weight%TYPE,
       vper_unit        bag_norms.per_unit%TYPE,
       vnorm_type       bag_norms.norm_type%TYPE,
       vextra           bag_norms.extra%TYPE,
       vtid             bag_norms.tid%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,first_date,last_date
  FROM bag_norms
/*попробовать оптимизировать запрос*/
  WHERE (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
        (pr_trfer IS NULL AND vpr_trfer IS NULL OR pr_trfer=vpr_trfer) AND
        (city_dep IS NULL AND vcity_dep IS NULL OR city_dep=vcity_dep) AND
        (city_arv IS NULL AND vcity_arv IS NULL OR city_arv=vcity_arv) AND
        (pax_cat IS NULL AND vpax_cat IS NULL OR pax_cat=vpax_cat) AND
        (subclass IS NULL AND vsubclass IS NULL OR subclass=vsubclass) AND
        (class IS NULL AND vclass IS NULL OR class=vclass) AND
        (flt_no IS NULL AND vflt_no IS NULL OR flt_no=vflt_no) AND
        (craft IS NULL AND vcraft IS NULL OR craft=vcraft) AND
        (trip_type IS NULL AND vtrip_type IS NULL OR trip_type=vtrip_type) AND
        (bag_type IS NULL AND vbag_type IS NULL OR bag_type=vbag_type) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     bag_norms.id%TYPE;
tidh    bag_norms.tid%TYPE;
pr_opd  BOOLEAN;
BEGIN
  check_params(vid,vairline,vcity_dep,vcity_arv,vfirst_date,vlast_date,first,last,pr_opd);

/*  IF (vnorm IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_BAG_NORM');
  END IF;*/

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;


  /* пробуем разбить на отрезки */
  FOR curRow IN cur LOOP
    idh:=curRow.id;
    IF vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id THEN
      IF curRow.first_date<first THEN
        /* отрезок [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE bag_norms SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO bag_norms(id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
                                first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
                 curRow.first_date,first,bag_type,amount,weight,per_unit,norm_type,extra,0,tidh
          FROM bag_norms WHERE id=curRow.id;
        END IF;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* отрезок [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE bag_norms SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO bag_norms(id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
                                first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
                 last,curRow.last_date,bag_type,amount,weight,per_unit,norm_type,extra,0,tidh
          FROM bag_norms WHERE id=curRow.id;
        END IF;
      END IF;

      IF idh IS NOT NULL THEN
        UPDATE bag_norms SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd THEN
      /*новый отрезок [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO bag_norms(id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,
                            first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,tid)
      VALUES(vid,vairline,vpr_trfer,vcity_dep,vcity_arv,vpax_cat,vsubclass,vclass,vflt_no,vcraft,vtrip_type,
             first,last,vbag_type,vamount,vweight,vper_unit,vnorm_type,vextra,0,tidh);
    END IF;
  ELSE
    /* при редактировании апдейтим строку */
    UPDATE bag_norms SET last_date=last,tid=tidh WHERE id=vid;
  END IF;
END add_bag_norm;

PROCEDURE add_bag_rate(
       vid       IN OUT bag_rates.id%TYPE,
       vairline         bag_rates.airline%TYPE,
       vpr_trfer        bag_rates.pr_trfer%TYPE,
       vcity_dep        bag_rates.city_dep%TYPE,
       vcity_arv        bag_rates.city_arv%TYPE,
       vpax_cat         bag_rates.pax_cat%TYPE,
       vsubclass        bag_rates.subclass%TYPE,
       vclass           bag_rates.class%TYPE,
       vflt_no          bag_rates.flt_no%TYPE,
       vcraft           bag_rates.craft%TYPE,
       vtrip_type       bag_rates.trip_type%TYPE,
       vfirst_date      bag_rates.first_date%TYPE,
       vlast_date       bag_rates.last_date%TYPE,
       vbag_type        bag_rates.bag_type%TYPE,
       vrate            bag_rates.rate%TYPE,
       vrate_cur        bag_rates.rate_cur%TYPE,
       vmin_weight      bag_rates.min_weight%TYPE,
       vextra           bag_rates.extra%TYPE,
       vtid             bag_rates.tid%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,first_date,last_date
  FROM bag_rates
  WHERE (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
        (pr_trfer IS NULL AND vpr_trfer IS NULL OR pr_trfer=vpr_trfer) AND
        (city_dep IS NULL AND vcity_dep IS NULL OR city_dep=vcity_dep) AND
        (city_arv IS NULL AND vcity_arv IS NULL OR city_arv=vcity_arv) AND
        (pax_cat IS NULL AND vpax_cat IS NULL OR pax_cat=vpax_cat) AND
        (subclass IS NULL AND vsubclass IS NULL OR subclass=vsubclass) AND
        (class IS NULL AND vclass IS NULL OR class=vclass) AND
        (flt_no IS NULL AND vflt_no IS NULL OR flt_no=vflt_no) AND
        (craft IS NULL AND vcraft IS NULL OR craft=vcraft) AND
        (trip_type IS NULL AND vtrip_type IS NULL OR trip_type=vtrip_type) AND
        (bag_type IS NULL AND vbag_type IS NULL OR bag_type=vbag_type) AND
        (min_weight IS NULL AND vmin_weight IS NULL OR min_weight=vmin_weight) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     bag_rates.id%TYPE;
tidh    bag_rates.tid%TYPE;
pr_opd  BOOLEAN;
BEGIN
  check_params(vid,vairline,vcity_dep,vcity_arv,vfirst_date,vlast_date,first,last,pr_opd);

  IF (vrate IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_BAG_RATE');
  END IF;
  IF (vrate_cur IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_BAG_RATE_CUR');
  END IF;

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;

  /* пробуем разбить на отрезки */
  FOR curRow IN cur LOOP
    idh:=curRow.id;
    IF vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id THEN
      IF curRow.first_date<first THEN
        /* отрезок [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE bag_rates SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO bag_rates(id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
                                first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
                 curRow.first_date,first,rate,rate_cur,min_weight,extra,0,tidh
          FROM bag_rates WHERE id=curRow.id;
        END IF;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* отрезок [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE bag_rates SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO bag_rates(id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
                                first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
                 last,curRow.last_date,rate,rate_cur,min_weight,extra,0,tidh
          FROM bag_rates WHERE id=curRow.id;
        END IF;
      END IF;

      IF idh IS NOT NULL THEN
        UPDATE bag_rates SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd THEN
      /*новый отрезок [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO bag_rates(id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type,
                            first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid)
      VALUES(vid,vairline,vpr_trfer,vcity_dep,vcity_arv,vbag_type,vpax_cat,vsubclass,vclass,vflt_no,vcraft,vtrip_type,
             first,last,vrate,vrate_cur,vmin_weight,vextra,0,tidh);
    END IF;
  ELSE
    /* при редактировании апдейтим строку */
    UPDATE bag_rates SET last_date=last,tid=tidh WHERE id=vid;
  END IF;
END add_bag_rate;

PROCEDURE add_value_bag_tax(
       vid       IN OUT value_bag_taxes.id%TYPE,
       vairline         value_bag_taxes.airline%TYPE,
       vpr_trfer        value_bag_taxes.pr_trfer%TYPE,
       vcity_dep        value_bag_taxes.city_dep%TYPE,
       vcity_arv        value_bag_taxes.city_arv%TYPE,
       vfirst_date      value_bag_taxes.first_date%TYPE,
       vlast_date       value_bag_taxes.last_date%TYPE,
       vtax             value_bag_taxes.tax%TYPE,
       vmin_value       value_bag_taxes.min_value%TYPE,
       vmin_value_cur   value_bag_taxes.min_value_cur%TYPE,
       vextra           value_bag_taxes.extra%TYPE,
       vtid             value_bag_taxes.tid%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,first_date,last_date
  FROM value_bag_taxes
  WHERE (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
        (pr_trfer IS NULL AND vpr_trfer IS NULL OR pr_trfer=vpr_trfer) AND
        (city_dep IS NULL AND vcity_dep IS NULL OR city_dep=vcity_dep) AND
        (city_arv IS NULL AND vcity_arv IS NULL OR city_arv=vcity_arv) AND
        (min_value IS NULL AND min_value_cur IS NULL AND
         vmin_value IS NULL AND vmin_value_cur IS NULL OR
         min_value=vmin_value AND min_value_cur=vmin_value_cur) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     value_bag_taxes.id%TYPE;
tidh    value_bag_taxes.tid%TYPE;
pr_opd  BOOLEAN;
BEGIN
  check_params(vid,vairline,vcity_dep,vcity_arv,vfirst_date,vlast_date,first,last,pr_opd);

  IF (vtax IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_VALUE_BAG_TAX');
  END IF;

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;

  /* пробуем разбить на отрезки */
  FOR curRow IN cur LOOP
    idh:=curRow.id;
    IF vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id THEN
      IF curRow.first_date<first THEN
        /* отрезок [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE value_bag_taxes SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO value_bag_taxes(id,airline,pr_trfer,city_dep,city_arv,
                                      first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,
                 curRow.first_date,first,tax,min_value,min_value_cur,extra,0,tidh
          FROM value_bag_taxes WHERE id=curRow.id;
        END IF;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* отрезок [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE value_bag_taxes SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO value_bag_taxes(id,airline,pr_trfer,city_dep,city_arv,
                                first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,pr_trfer,city_dep,city_arv,
                 last,curRow.last_date,tax,min_value,min_value_cur,extra,0,tidh
          FROM value_bag_taxes WHERE id=curRow.id;
        END IF;
      END IF;

      IF idh IS NOT NULL THEN
        UPDATE value_bag_taxes SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd THEN
      /*новый отрезок [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO value_bag_taxes(id,airline,pr_trfer,city_dep,city_arv,
                            first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid)
      VALUES(vid,vairline,vpr_trfer,vcity_dep,vcity_arv,
             first,last,vtax,vmin_value,vmin_value_cur,vextra,0,tidh);
    END IF;
  ELSE
    /* при редактировании апдейтим строку */
    UPDATE value_bag_taxes SET last_date=last,tid=tidh WHERE id=vid;
  END IF;
END add_value_bag_tax;

PROCEDURE add_exchange_rate(
       vid       IN OUT exchange_rates.id%TYPE,
       vairline         exchange_rates.airline%TYPE,
       vrate1           exchange_rates.rate1%TYPE,
       vcur1            exchange_rates.cur1%TYPE,
       vrate2           exchange_rates.rate2%TYPE,
       vcur2            exchange_rates.cur2%TYPE,
       vfirst_date      exchange_rates.first_date%TYPE,
       vlast_date       exchange_rates.last_date%TYPE,
       vextra           exchange_rates.extra%TYPE,
       vtid             exchange_rates.tid%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,first_date,last_date
  FROM exchange_rates
  WHERE (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
        (cur1=vcur1) AND
        (cur2=vcur2) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     exchange_rates.id%TYPE;
tidh    exchange_rates.tid%TYPE;
pr_opd  BOOLEAN;
BEGIN
  check_params(vid,vairline,NULL,NULL,vfirst_date,vlast_date,first,last,pr_opd);

  IF (vrate1 IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_FIRST_CUR_VALUE');
  END IF;
  IF (vcur1 IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_FIRST_CUR_CODE');
  END IF;
  IF (vrate2 IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_SECOND_CUR_VALUE');
  END IF;
  IF (vcur2 IS NULL) AND not(pr_opd) THEN
    system.raise_user_exception('MSG.PAYMENT.NOT_SET_SECOND_CUR_CODE');
  END IF;

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;


  /* пробуем разбить на отрезки */
  FOR curRow IN cur LOOP
    idh:=curRow.id;
    IF vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id THEN
      IF curRow.first_date<first THEN
        /* отрезок [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE exchange_rates SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO exchange_rates(id,airline,rate1,cur1,rate2,cur2,
                                      first_date,last_date,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,rate1,cur1,rate2,cur2,
                 curRow.first_date,first,extra,0,tidh
          FROM exchange_rates WHERE id=curRow.id;
        END IF;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* отрезок [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE exchange_rates SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          idh:=NULL;
        ELSE
          INSERT INTO exchange_rates(id,airline,rate1,cur1,rate2,cur2,
                                first_date,last_date,extra,pr_del,tid)
          SELECT id__seq.nextval,airline,rate1,cur1,rate2,cur2,
                 last,curRow.last_date,extra,0,tidh
          FROM exchange_rates WHERE id=curRow.id;
        END IF;
      END IF;

      IF idh IS NOT NULL THEN
        UPDATE exchange_rates SET pr_del=1,tid=tidh WHERE id=curRow.id;
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd THEN
      /*новый отрезок [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO exchange_rates(id,airline,rate1,cur1,rate2,cur2,
                            first_date,last_date,extra,pr_del,tid)
      VALUES(vid,vairline,vrate1,vcur1,vrate2,vcur2,
             first,last,vextra,0,tidh);
    END IF;
  ELSE
    /* при редактировании апдейтим строку */
    UPDATE exchange_rates SET last_date=last,tid=tidh WHERE id=vid;
  END IF;
END add_exchange_rate;

END kassa;
/
