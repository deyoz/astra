create or replace PACKAGE BODY salons
AS
RUS_NAME_LINES VARCHAR2(30) := 'АБВГДЕЖЗИКЛМНОПРСТУФХЦЧШЩ';
LAT_NAME_LINES VARCHAR2(30) := 'ABCDEFGHJKLMNOPQRSTUVWXYZ';
--функция нормализует названия псевдо-ИАТА линий салона к лат. буквам
--если не псевдо-ИАТА, то возвращает NULL
FUNCTION normalize_xname(vxname comp_elems.xname%TYPE) RETURN comp_elems.xname%TYPE
IS
res comp_elems.xname%TYPE;
i   BINARY_INTEGER;
BEGIN
  res:=TRIM(UPPER(vxname));
  IF res IS NULL THEN RETURN NULL; END IF;
  --проверим , что один символ
  IF LENGTH(res)<>1 THEN RETURN NULL; END IF;

  --попробуем найти среди лат. букв
  IF res BETWEEN 'A' AND 'Z' THEN
    IF INSTR(LAT_NAME_LINES,res)<=0 THEN RETURN NULL; ELSE RETURN res; END IF;
  END IF;

  --попробуем найти среди рус. букв
  IF res BETWEEN 'А' AND 'Я' OR res = 'Ё' THEN
    i:=INSTR(RUS_NAME_LINES,res);
    IF i<=0 THEN RETURN NULL; END IF;
    --переконвертим рус -> лат
    res:=SUBSTR(LAT_NAME_LINES,i,1);
    RETURN res;
  END IF;

  RETURN NULL;
END normalize_xname;

--функция нормализует названия псевдо-ИАТА рядов салона к диапазонам 001-099, 101-199
--если не псевдо-ИАТА, то возвращает NULL
FUNCTION normalize_yname(vyname comp_elems.yname%TYPE) RETURN comp_elems.yname%TYPE
IS
res comp_elems.yname%TYPE;
i   BINARY_INTEGER;
BEGIN
  res:=TRIM(vyname);
  IF res IS NULL THEN RETURN NULL; END IF;
  --проверим , название состоит только из цифр
  FOR i IN 1..LENGTH(res) LOOP
    IF NOT SUBSTR(res,i,1) BETWEEN '0' AND '9' THEN RETURN NULL; END IF;
  END LOOP;
  --конвертим в число
  i:=TO_NUMBER(res);
  --проверяем диапазоны
  IF i BETWEEN 001 AND 099 OR
     i BETWEEN 101 AND 199 THEN
    res:=LPAD(TO_CHAR(i),3,'0');
  ELSE
    res:=NULL;
  END IF;
  RETURN res;
EXCEPTION
  WHEN INVALID_NUMBER THEN
    RETURN NULL;
END normalize_yname;

FUNCTION denormalize_xname(vxname comp_elems.xname%TYPE,
                           pr_lat NUMBER) RETURN comp_elems.xname%TYPE
IS
i   BINARY_INTEGER;
BEGIN
  IF LENGTH(vxname)=1 THEN
    i:=INSTR(LAT_NAME_LINES,vxname);
    IF i>0 THEN
      IF pr_lat=0 THEN RETURN SUBSTR(RUS_NAME_LINES,i,1); ELSE RETURN vxname; END IF;
    END IF;
  END IF;
  RETURN NULL;
END denormalize_xname;

FUNCTION denormalize_yname(vyname comp_elems.yname%TYPE,
                           add_ch VARCHAR2) RETURN comp_elems.yname%TYPE
IS
BEGIN
  --проверяем диапазоны
  IF vyname BETWEEN '001' AND '099' OR
     vyname BETWEEN '101' AND '199' THEN
    IF add_ch IS NOT NULL THEN
      RETURN LPAD(LTRIM(vyname,'0'),3,add_ch);
    ELSE
      RETURN LTRIM(vyname,'0');
    END IF;
  END IF;
  RETURN NULL;
END denormalize_yname;

FUNCTION get_crs_seat_no_int(vlayer_type IN tlg_comp_layers.layer_type%TYPE,
                             vseat_xname IN crs_pax.seat_xname%TYPE,
                             vseat_yname IN crs_pax.seat_yname%TYPE,
                             vseats      IN crs_pax.seats%TYPE,
                             vpoint_id   IN crs_pnr.point_id%TYPE,
                             fmt	     IN VARCHAR2,
                             row	     IN NUMBER DEFAULT 1,
                             only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
res VARCHAR2(20);
pr_lat  	NUMBER(1);
add_ch		VARCHAR2(1);
BEGIN
  IF only_lat=0 OR
     vlayer_type IS NOT NULL AND vlayer_type IN (cltPNLBeforePay, cltPNLAfterPay, cltProtBeforePay, cltProtAfterPay) THEN
    IF row<>1 AND
       vpoint_id=crsSeatInfo.point_id THEN
      NULL;
    ELSE
      crsSeatInfo.point_id:=vpoint_id;
      crsSeatInfo.pr_lat_seat:=1;
      crsSeatInfo.pr_paid_ckin:=0;
      BEGIN
        SELECT NVL(trip_sets.pr_lat_seat,1),
               NVL(trip_paid_ckin.pr_permit,0)
        INTO crsSeatInfo.pr_lat_seat,
             crsSeatInfo.pr_paid_ckin
        FROM tlg_binding,trip_sets,trip_paid_ckin
        WHERE tlg_binding.point_id_spp=trip_sets.point_id AND
              tlg_binding.point_id_spp=trip_paid_ckin.point_id(+) AND
              tlg_binding.point_id_tlg=vpoint_id AND
              rownum<=1;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN NULL;
      END;
    END IF;
    pr_lat:=crsSeatInfo.pr_lat_seat;
    IF vlayer_type IS NOT NULL AND
       vlayer_type IN (cltPNLBeforePay, cltPNLAfterPay, cltProtBeforePay, cltProtAfterPay) AND
       crsSeatInfo.pr_paid_ckin=0 THEN
      RETURN NULL;
    END IF;
  ELSE
    pr_lat:=1;
  END IF;
  IF UPPER(fmt) IN ('_LIST','_ONE','_SEATS') THEN add_ch:=' '; ELSE add_ch:=NULL; END IF;
  res:=NVL(denormalize_yname(vseat_yname,add_ch),vseat_yname)||
       NVL(denormalize_xname(vseat_xname,pr_lat),vseat_xname);
  IF UPPER(fmt) IN ('ONE','_ONE') THEN RETURN res; END IF;
  IF res IS NOT NULL AND vseats>1 THEN res:=res||'+'||TO_CHAR(vseats-1); END IF;
  RETURN res;
END get_crs_seat_no_int;

FUNCTION get_crs_seat_no(vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         fmt	     IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
BEGIN
  RETURN get_crs_seat_no_int(NULL, vseat_xname, vseat_yname, vseats, vpoint_id, fmt, row, only_lat);
END get_crs_seat_no;

FUNCTION get_crs_seat_no(vpax_id     IN crs_pax.pax_id%TYPE,
                         vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         fmt	     IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
vlayer_type tlg_comp_layers.layer_type%TYPE;
BEGIN
  vlayer_type:=NULL;
  RETURN get_crs_seat_no(vpax_id, vseat_xname, vseat_yname, vseats, vpoint_id, vlayer_type, fmt, row, only_lat);
END get_crs_seat_no;

FUNCTION get_crs_seat_no(vpax_id     IN crs_pax.pax_id%TYPE,
                         vseat_xname IN crs_pax.seat_xname%TYPE,
                         vseat_yname IN crs_pax.seat_yname%TYPE,
                         vseats      IN crs_pax.seats%TYPE,
                         vpoint_id   IN crs_pnr.point_id%TYPE,
                         vlayer_type OUT tlg_comp_layers.layer_type%TYPE,
                         fmt	     IN VARCHAR2,
                         row	     IN NUMBER DEFAULT 1,
                         only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
CURSOR cur IS
  SELECT first_xname AS xname,
         first_yname AS yname,
         layer_type
  FROM tlg_comp_layers, comp_layer_types
  WHERE crs_pax_id=vpax_id AND
        tlg_comp_layers.layer_type=comp_layer_types.code
  ORDER BY priority,tlg_comp_layers.layer_type,first_yname,first_xname;
curRow    cur%ROWTYPE;
res VARCHAR2(20);
vpoint_dep crs_pnr.point_id%TYPE;
BEGIN
  vlayer_type := NULL;
  IF vpax_id IS NULL THEN
    IF row=1 THEN crsSeatInfo.point_id:=NULL; END IF;
    RETURN NULL;
  END IF;
  res:=NULL;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    IF only_lat=0 THEN
      IF vpoint_id IS NULL THEN
        BEGIN
          SELECT point_id
          INTO vpoint_dep
          FROM crs_pnr,crs_pax
          WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND crs_pax.pax_id=vpax_id;
        EXCEPTION
          WHEN NO_DATA_FOUND THEN
            CLOSE cur;
            IF row=1 THEN crsSeatInfo.point_id:=NULL; END IF;
            RETURN NULL;
        END;
      ELSE
        vpoint_dep:=vpoint_id;
      END IF;
    END IF;
    WHILE cur%FOUND LOOP
      IF vlayer_type IS NULL OR vlayer_type <> curRow.layer_type THEN
        IF curRow.layer_type=cltPNLCkin THEN
         res:=get_crs_seat_no_int(curRow.layer_type,vseat_xname,vseat_yname,vseats,vpoint_dep,fmt,row,only_lat);
        ELSE
         res:=get_crs_seat_no_int(curRow.layer_type,curRow.xname,curRow.yname,vseats,vpoint_dep,fmt,row,only_lat);
        END IF;
        vlayer_type := curRow.layer_type;
      END IF;
      EXIT WHEN res IS NOT NULL;
      FETCH cur INTO curRow;
    END LOOP;
  END IF;
  CLOSE cur;
  RETURN res;
END get_crs_seat_no;

/*
   fmt - формат данных.
   Может принимать значения:
   'one':    одно левое верхнее место пассажира
   '_one':   одно левое верхнее место пассажира + пробелы слева
   'tlg':    перечисление псевдо-IATA мест пассажира. Сортировка по названию: 4А4Б4В
   'list':   перечисление всех мест пассажира через пробел. Сортировка по названию.
   '_list':  перечисление всех мест пассажира через пробел. Сортировка по названию + пробелы слева
   'voland': см. описание формата в процедуре. Используется для печати пос. талонов.
   '_seats': одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2 + пробелы слева
   иначе:    одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2
*/
FUNCTION get_seatno(vpax_id     IN pax.pax_id%TYPE,
                    vseats      IN pax.seats%TYPE,
                    vpoint_id   IN trip_sets.point_id%TYPE,
                    fmt	 IN VARCHAR2,
                    row	 IN NUMBER DEFAULT 1,
                    only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
CURSOR cur(vpoint_id   IN trip_sets.point_id%TYPE,
           vpax_id     IN pax.pax_id%TYPE) IS
  SELECT yname, xname FROM pax_seats
   WHERE pax_id=vpax_id
  ORDER BY yname, xname;
curRow    	cur%ROWTYPE;
vpoint_dep 	trip_sets.point_id%TYPE;
i		BINARY_INTEGER;
res_one         VARCHAR2(20);
res_num         VARCHAR2(20);
res_tlg         VARCHAR2(20);
res_list	VARCHAR2(50);
res_voland	VARCHAR2(20);
vxname		trip_comp_elems.xname%TYPE;
vyname		trip_comp_elems.yname%TYPE;
pr_lat  	NUMBER(1);
all_seat_norm   BOOLEAN;

vfirst_xname    VARCHAR2(1);
vlast_xname     VARCHAR2(1);
vfirst_yname	VARCHAR2(3);
vlast_yname	VARCHAR2(3);

add_ch		VARCHAR2(1);
BEGIN
  IF vpax_id IS NULL THEN
    IF row=1 THEN seatInfo.point_id:=NULL; END IF;
    RETURN NULL;
  END IF;
  res_num:=NULL;

  IF vpoint_id IS NULL THEN
    BEGIN
      SELECT pax_grp.point_dep
      INTO vpoint_dep
      FROM pax_grp,pax
      WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=vpax_id;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        IF row=1 THEN seatInfo.point_id:=NULL; END IF;
        RETURN NULL;
    END;
  ELSE
    vpoint_dep:=vpoint_id;
  END IF;

  IF row<>1 AND
     vpoint_dep=seatInfo.point_id THEN
    NULL;
  ELSE
    seatInfo.point_id:=vpoint_dep;
    seatInfo.pr_lat_seat:=1;
    seatInfo.pr_free_seating:=0;
    BEGIN
      SELECT NVL(trip_sets.pr_lat_seat,1),
             NVL(trip_sets.pr_free_seating,0)
      INTO seatInfo.pr_lat_seat,
           seatInfo.pr_free_seating
      FROM trip_sets
      WHERE point_id=vpoint_dep;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN NULL;
    END;
  END IF;
  IF seatInfo.pr_free_seating<>0 THEN
    RETURN NULL;
  END IF;

  IF only_lat=0 THEN
    pr_lat:=seatInfo.pr_lat_seat;
  ELSE
    pr_lat:=1;
  END IF;

  IF UPPER(fmt) IN ('_LIST','_ONE','_SEATS') THEN add_ch:=' '; ELSE add_ch:=NULL; END IF;
  all_seat_norm:=TRUE;
  i:=0;

  FOR curRow IN cur(vpoint_dep,vpax_id) LOOP
    vyname:=denormalize_yname(curRow.yname,add_ch);
    add_ch:=' ';
    vxname:=denormalize_xname(curRow.xname,pr_lat);
    IF vyname IS NULL OR vxname IS NULL THEN
      all_seat_norm:=FALSE;
    ELSE
      res_tlg:=res_tlg||vyname||vxname; /* простое перечисление нормальных номеров мест */
      IF i=0 THEN
        vfirst_xname:=vxname;
        vfirst_yname:=curRow.yname; /* важно, что спереди нули и длина = 3 цифрам */
      END IF;
      vlast_xname:=vxname;
      vlast_yname:=curRow.yname; /* важно, что спереди нули и длина = 3 цифрам */
    END IF;

    IF res_one IS NULL THEN

      /* составляем формат NUM (самое левое верхнее место салона) */
      res_one:=NVL(vyname,curRow.yname)||NVL(vxname,curRow.xname);
      IF res_one IS NOT NULL AND vseats>1 THEN
        res_num:=res_one||'+'||TO_CHAR(vseats-1);
      ELSE
        res_num:=res_one;
      END IF;
    END IF;

    res_list:=res_list||NVL(vyname,curRow.yname)||NVL(vxname,curRow.xname)||' ';
    i:=i+1;
  END LOOP;

  IF i>vseats THEN RETURN '???'; END IF;
  IF i<vseats THEN RETURN NULL; END IF; /* не все места пассажира реально заняты в салоне */

  IF UPPER(fmt) IN ('ONE','_ONE') THEN RETURN res_one; END IF;
  IF UPPER(fmt) IN ('TLG') THEN RETURN res_tlg; END IF;
  IF UPPER(fmt) IN ('LIST','_LIST') THEN RETURN TRIM(res_list); END IF;
  IF UPPER(fmt) IN ('VOLAND') AND all_seat_norm THEN
      -- Следующий формат используется при печати мест на пос. талоне
      -- максимальная длина строки - 6 символов
      -- Если рассадка горизонтальная, то формат однозначный 200А-Б.
      -- При вертикальной рассадке возможны 5 вариантов:
      -- 1. Переход номера ряда с 2-значного на 3-значный 99-01А
      -- 2. Не 3-начные номера ряда без перехода через десятку 45-7A
      -- 3. Не 3-начные номера ряда с переходом через десятку 49-51А
      -- 4. 3-начные номера ряда без перехода через десятку 150-2A
      -- 5. 3-начные номера ряда с переходом через десятку 19901А
    IF vfirst_yname=vlast_yname AND vfirst_xname=vlast_xname THEN
      /* одно место */
      res_voland:=LTRIM(vfirst_yname,'0')||vfirst_xname;
      RETURN res_voland;
    END IF;
    IF vfirst_yname=vlast_yname THEN
      /* рассадка горизонтальная */
      res_voland:=LTRIM(vfirst_yname,'0')||vfirst_xname||'-'||vlast_xname;
      RETURN res_voland;
    END IF;
    IF vfirst_xname=vlast_xname AND
       LENGTH(vfirst_yname)=3 AND LENGTH(vlast_yname)=3 THEN
      /* рассадка вертикальная */
      i:=1;
      WHILE i<=3 AND SUBSTR(vfirst_yname,i,1)=SUBSTR(vlast_yname,i,1) LOOP
        i:=i+1;
      END LOOP;
      IF i=1 THEN i:=2; END IF;
      res_voland:=LTRIM(vfirst_yname,'0')||'-'||SUBSTR(vlast_yname,i)||vfirst_xname;
      IF LENGTH(res_voland)>6 THEN
        res_voland:=LTRIM(vfirst_yname,'0')||SUBSTR(vlast_yname,i)||vfirst_xname;
      END IF;
      RETURN res_voland;
    END IF;
  END IF;
  RETURN res_num;
END get_seatno;

/*
   fmt - формат данных.
   Может принимать значения:
   'one':    одно левое верхнее место пассажира
   '_one':   одно левое верхнее место пассажира + пробелы слева
   'tlg':    перечисление псевдо-IATA мест пассажира. Сортировка по названию: 4А4Б4В
   'list':   перечисление всех мест пассажира через пробел. Сортировка по названию.
   '_list':  перечисление всех мест пассажира через пробел. Сортировка по названию + пробелы слева
   'voland': см. описание формата в процедуре. Используется для печати пос. талонов.
   '_seats': одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2 + пробелы слева
   иначе:    одно левое верхнее место пассажира плюс оставшееся кол-во мест: 4А+2
*/
FUNCTION get_seat_no(vpax_id     IN pax.pax_id%TYPE,
                     vseats      IN pax.seats%TYPE,
                     vgrp_status IN pax_grp.status%TYPE,
                     vpoint_id   IN trip_sets.point_id%TYPE,
                     fmt	 IN VARCHAR2,
                     row	 IN NUMBER DEFAULT 1,
                     only_lat    IN NUMBER DEFAULT 0) RETURN VARCHAR2
IS
CURSOR cur(vpax_id     IN pax.pax_id%TYPE,
           vgrp_status IN pax_grp.status%TYPE) IS
  SELECT DISTINCT
         trip_comp_elems.yname,
         trip_comp_elems.xname,
         trip_comp_elems.num,
         trip_comp_elems.x,
         trip_comp_elems.y,
         comp_layer_types.priority,
         trip_comp_layers.time_create,
         trip_comp_layers.pax_id
  FROM trip_comp_layers,trip_comp_ranges,trip_comp_elems,
       grp_status_types,comp_layer_types
  WHERE trip_comp_layers.range_id=trip_comp_ranges.range_id AND
        trip_comp_layers.point_id=trip_comp_ranges.point_id AND
        trip_comp_ranges.point_id=trip_comp_elems.point_id AND
        trip_comp_ranges.num=trip_comp_elems.num AND
        trip_comp_ranges.x=trip_comp_elems.x AND
        trip_comp_ranges.y=trip_comp_elems.y AND
        trip_comp_layers.layer_type=grp_status_types.layer_type AND
        trip_comp_layers.layer_type=comp_layer_types.code AND
        trip_comp_layers.pax_id=vpax_id AND grp_status_types.code=vgrp_status
  ORDER BY yname, xname;

CURSOR cur2(vpoint_id 	IN trip_comp_ranges.point_id%TYPE,
            vnum	IN trip_comp_ranges.num%TYPE,
            vx		IN trip_comp_ranges.x%TYPE,
            vy		IN trip_comp_ranges.y%TYPE,
            vpriority	IN comp_layer_types.priority%TYPE,
            vtime_create IN trip_comp_layers.time_create%TYPE) IS
  SELECT trip_comp_layers.point_dep,
         trip_comp_layers.layer_type,
         trip_comp_layers.pax_id
  FROM trip_comp_layers,trip_comp_ranges,comp_layer_types
  WHERE trip_comp_layers.range_id=trip_comp_ranges.range_id AND
        trip_comp_layers.point_id=trip_comp_ranges.point_id AND
        trip_comp_layers.layer_type=comp_layer_types.code AND
        trip_comp_ranges.point_id=vpoint_id AND
        trip_comp_ranges.num=vnum AND
        trip_comp_ranges.x=vx AND
        trip_comp_ranges.y=vy AND
        ( comp_layer_types.priority < vpriority  OR comp_layer_types.priority = vpriority AND trip_comp_layers.time_create > vtime_create );

CURSOR cur3(vpoint_num	   IN points.point_num%TYPE,
            vfirst_point   IN points.first_point%TYPE) IS
    SELECT points.point_id AS point_dep,
           DECODE(tlgs_in.type,'PRL',cltPRLTrzt,cltSOMTrzt) AS layer_type
    FROM tlg_binding,tlg_source,tlgs_in,
         (SELECT point_id,point_num FROM points
          WHERE vfirst_point IN (first_point,point_id) AND point_num<vpoint_num AND pr_del=0
          ORDER BY point_num
         ) points
    WHERE tlg_binding.point_id_spp=points.point_id AND
          tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND
          tlgs_in.id=tlg_source.tlg_id AND tlgs_in.num=1 AND tlgs_in.type IN ('PRL','SOM')
    ORDER BY point_num DESC,DECODE(tlgs_in.type,'PRL',0,1);

CURSOR cur4 IS
  SELECT code FROM comp_layer_types
   WHERE code NOT IN (cltTranzit,cltBlockTRZT,cltProtTrzt,cltPRLTrzt,cltSOMTrzt);

CURSOR cur5(vpoint_id	IN points.point_id%TYPE) IS
  SELECT pr_new,
         DECODE( tranzit_algo_seats.airp, NULL, 0, 4 ) +
         DECODE( tranzit_algo_seats.airline, NULL, 0, 2 ) +
         DECODE( tranzit_algo_seats.flt_no, NULL, 0, 1 ) AS priority
  FROM tranzit_algo_seats, points
  WHERE point_id=vpoint_id AND
        ( tranzit_algo_seats.airp IS NULL OR tranzit_algo_seats.airp=points.airp ) AND
        ( tranzit_algo_seats.airline IS NULL OR tranzit_algo_seats.airline=points.airline ) AND
        ( tranzit_algo_seats.flt_no IS NULL OR tranzit_algo_seats.flt_no=points.flt_no )
  ORDER BY priority DESC;


curRow    	cur%ROWTYPE;
curRow5		cur5%ROWTYPE;
vpoint_dep 	trip_sets.point_id%TYPE;
vstatus         pax_grp.status%TYPE;
i		BINARY_INTEGER;
res_one         VARCHAR2(20);
res_num         VARCHAR2(20);
res_tlg         VARCHAR2(20);
res_list	VARCHAR2(50);
res_voland	VARCHAR2(20);
vxname		trip_comp_elems.xname%TYPE;
vyname		trip_comp_elems.yname%TYPE;
pr_lat  	NUMBER(1);
all_seat_norm   BOOLEAN;

vfirst_x        trip_comp_elems.x%TYPE;
vfirst_y        trip_comp_elems.y%TYPE;
vlast_x         trip_comp_elems.x%TYPE;
vlast_y         trip_comp_elems.y%TYPE;
vnum            trip_comp_elems.num%TYPE;
vfirst_xname    VARCHAR2(1);
vlast_xname     VARCHAR2(1);
vfirst_yname	VARCHAR2(3);
vlast_yname	VARCHAR2(3);

vpoint_num	points.point_num%TYPE;
vfirst_point	points.first_point%TYPE;
cur3Row		cur3%ROWTYPE;
ind_layer	NUMBER;
vpriority_seats pax.seats%TYPE;
vtlg_layer_type trip_comp_layers.layer_type%TYPE;
add_ch		VARCHAR2(1);
vrow 		NUMBER;
BEGIN
  --dbms_output.put_line('pax_id='||vpax_id||' row='||row);
  IF row=1 THEN
   seatInfo.layers.DELETE; -- удаляем все элементы таблицы
  END IF;
  IF vpax_id IS NULL THEN
    IF row=1 THEN seatInfo.point_id:=NULL; END IF;
    RETURN NULL;
  END IF;
  res_num:=NULL;

  IF vpoint_id IS NULL OR vgrp_status IS NULL THEN
    BEGIN
      SELECT pax_grp.point_dep,pax_grp.status
      INTO vpoint_dep,vstatus
      FROM pax_grp,pax
      WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=vpax_id;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
        IF row=1 THEN seatInfo.point_id:=NULL; END IF;
        RETURN NULL;
    END;
  ELSE
    vpoint_dep:=vpoint_id;
    vstatus:=vgrp_status;
  END IF;

  IF row<>1 AND
     vpoint_dep=seatInfo.point_id THEN
    vrow:=row;
  ELSE
    vrow:=1;
    seatInfo.point_id:=vpoint_dep;
    seatInfo.pr_tranzit_salon:=0;
    OPEN cur5(vpoint_dep);
    FETCH cur5 INTO curRow5;
    IF cur5%FOUND THEN
      seatInfo.pr_tranzit_salon:=curRow5.pr_new;
    END IF;
    CLOSE cur5;
  END IF;
  IF seatInfo.pr_tranzit_salon<>0 THEN
    BEGIN
      SELECT 0 INTO seatInfo.pr_tranzit_salon FROM tranzit_algo_seats_points
       WHERE point_id=vpoint_dep;
    EXCEPTION WHEN NO_DATA_FOUND THEN NULL;
    END;
    IF seatInfo.pr_tranzit_salon<>0 THEN
      RETURN get_seatno(vpax_id,vseats,vpoint_dep,fmt,vrow,only_lat);
    END IF;
  END IF;


  OPEN cur(vpax_id,vstatus);
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    IF vrow<>1 AND
       vpoint_dep=seatInfo.point_id THEN
      NULL;
    ELSE
      vrow:=1; --на всякий случай
      seatInfo.point_id:=vpoint_dep;
      seatInfo.pr_lat_seat:=1;
      seatInfo.pr_tranzit:=0;
      seatInfo.pr_tranz_reg:=0;
      seatInfo.pr_block_trzt:=0;
      seatInfo.pr_free_seating:=0;
      --получим признак транзита
      SELECT NVL(ckin.get_pr_tranzit(vpoint_dep),0)
      INTO seatInfo.pr_tranzit
      FROM dual;

      BEGIN
        SELECT NVL(trip_sets.pr_lat_seat,1),
               NVL(trip_sets.pr_tranz_reg,0),
               NVL(trip_sets.pr_block_trzt,0),
               NVL(trip_sets.pr_free_seating,0)
        INTO seatInfo.pr_lat_seat,
             seatInfo.pr_tranz_reg,
             seatInfo.pr_block_trzt,
             seatInfo.pr_free_seating
        FROM trip_sets
        WHERE point_id=vpoint_dep;
        IF seatInfo.pr_tranz_reg<>0 THEN seatInfo.pr_block_trzt:=0; END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN NULL;
      END;

    END IF;

    IF seatInfo.pr_free_seating<>0 THEN
      RETURN NULL;
    END IF;

    IF only_lat=0 THEN
      pr_lat:=seatInfo.pr_lat_seat;
    ELSE
      pr_lat:=1;
    END IF;
    IF UPPER(fmt) IN ('_LIST','_ONE','_SEATS') THEN add_ch:=' '; ELSE add_ch:=NULL; END IF;
    all_seat_norm:=TRUE;
    i:=0;
    vfirst_x:=NULL;
    vfirst_y:=NULL;
    vlast_x:=NULL;
    vlast_y:=NULL;
    vnum:=curRow.num;
    WHILE cur%FOUND LOOP -- пробег по местам

      --проверим есть ли на этом месте слой с большим приоритетом
      FOR cur2Row IN cur2(vpoint_dep,curRow.num,curRow.x,curRow.y,curRow.priority,curRow.time_create) LOOP
        IF seatInfo.layers.COUNT = 0 THEN -- надо проиниализировать таблицу с разрешенными слоями
          ind_layer := 0;
          FOR cur4Row IN cur4 LOOP
           seatInfo.layers(ind_layer) := cur4Row.code;
           ind_layer := ind_layer + 1;
          END LOOP;
          IF seatInfo.pr_tranzit <> 0 THEN
            IF seatInfo.pr_tranz_reg <> 0 THEN
              seatInfo.layers(ind_layer) := cltProtTrzt;
              seatInfo.layers(ind_layer+1) := cltTranzit;
            ELSE
              IF seatInfo.pr_block_trzt <> 0 THEN
                seatInfo.layers(ind_layer) := cltBlockTrzt;
              ELSE
                seatInfo.tlg_point_dep := NULL;
                BEGIN
                  SELECT point_num, DECODE(pr_tranzit,0,point_id,first_point) INTO vpoint_num, vfirst_point
                   FROM points
                  WHERE points.point_id=vpoint_dep AND points.pr_del=0 AND points.pr_reg<>0;
                EXCEPTION
                  WHEN NO_DATA_FOUND THEN vpoint_num := NULL;
                END;
                IF vpoint_num IS NOT NULL THEN
                  OPEN cur3(vpoint_num,vfirst_point);
                  FETCH cur3 INTO cur3Row;
                  IF cur3%FOUND THEN
                    seatInfo.tlg_point_dep := cur3Row.point_dep;
                    seatInfo.layers(ind_layer) := cur3Row.layer_type;
                  END IF;
                  CLOSE cur3;
                END IF;
              END IF;
            END IF;
          END IF;
        END IF;
        -- сама проверка
        ind_layer := NULL;
        FOR i IN seatInfo.layers.FIRST..seatInfo.layers.LAST LOOP
         IF seatInfo.layers(i) = cur2Row.layer_type THEN
           ind_layer := i;
           -- получили более приоритетный слой. Если слой имеет pax_id, то проверить его на правильность!!!
           IF cur2Row.pax_id IS NOT NULL THEN
             BEGIN
               SELECT seats INTO vpriority_seats FROM pax where pax_id=cur2Row.pax_id;
              EXCEPTION
                WHEN NO_DATA_FOUND THEN ind_layer := NULL;
             END;
             IF ind_layer IS NOT NULL AND
                ( vpriority_seats < 2 OR
                  get_seat_no(cur2Row.pax_id,vpriority_seats,NULL,NULL,fmt,row+1,only_lat) IS NOT NULL ) THEN
                CLOSE cur;
                RETURN NULL;
             END IF;
             ind_layer := NULL;
           END IF;
         END IF;
         EXIT WHEN ( ind_layer IS NOT NULL );
        END LOOP;
        IF ind_layer IS NOT NULL AND -- слой в фильте найден - его надо учитывать
           ( cur2Row.layer_type <> cltSOMTrzt AND cur2Row.layer_type <> cltPRLTrzt OR cur2Row.point_dep = seatInfo.tlg_point_dep ) THEN
          CLOSE cur;
          RETURN NULL;
        END IF;
      END LOOP;

      vyname:=denormalize_yname(curRow.yname,add_ch);
      add_ch:=' ';
      vxname:=denormalize_xname(curRow.xname,pr_lat);
      IF vyname IS NULL OR vxname IS NULL THEN
        all_seat_norm:=FALSE;
      ELSE
        res_tlg:=res_tlg||vyname||vxname; /* простое перечисление нормальных номеров мест */
        IF i=0 THEN
          vfirst_xname:=vxname;
          vfirst_yname:=curRow.yname; /* важно, что спереди нули и длина = 3 цифрам */
        END IF;
        vlast_xname:=vxname;
        vlast_yname:=curRow.yname; /* важно, что спереди нули и длина = 3 цифрам */
      END IF;

      IF vfirst_x IS NULL OR vfirst_y IS NULL OR
         vfirst_y*1000+vfirst_x>curRow.y*1000+curRow.x THEN
        vfirst_x:=curRow.x;
        vfirst_y:=curRow.y;

        /* составляем формат NUM (самое левое верхнее место салона) */
        res_one:=NVL(vyname,curRow.yname)||NVL(vxname,curRow.xname);
        IF res_one IS NOT NULL AND vseats>1 THEN
          res_num:=res_one||'+'||TO_CHAR(vseats-1);
        ELSE
          res_num:=res_one;
        END IF;
      END IF;
      IF vlast_x IS NULL OR vlast_y IS NULL OR
         vlast_y*1000+vlast_x<curRow.y*1000+curRow.x THEN
        vlast_x:=curRow.x;
        vlast_y:=curRow.y;
      END IF;
      IF vnum<>curRow.num THEN /* места одного пассажира в разных салонах */
        CLOSE cur;
        RETURN NULL;
      END IF;

      res_list:=res_list||NVL(vyname,curRow.yname)||NVL(vxname,curRow.xname)||' ';

      FETCH cur INTO curRow;
      i:=i+1;
    END LOOP;
  END IF;
  CLOSE cur;
  IF i>vseats THEN RETURN '???'; END IF;
  IF i<vseats THEN RETURN NULL; END IF; /* не все места пассажира реально заняты в салоне */
  IF vfirst_x=vlast_x AND vfirst_y+i-1=vlast_y OR
     vfirst_y=vlast_y AND vfirst_x+i-1=vlast_x THEN
    NULL;
  ELSE
    RETURN NULL;  /* места пассажира идут не в ряд и не подряд */
  END IF;

  IF UPPER(fmt) IN ('ONE','_ONE') THEN RETURN res_one; END IF;
  IF UPPER(fmt) IN ('TLG') THEN RETURN res_tlg; END IF;
  IF UPPER(fmt) IN ('LIST','_LIST') THEN RETURN TRIM(res_list); END IF;
  IF UPPER(fmt) IN ('VOLAND') AND all_seat_norm THEN
        -- Следующий формат используется при печати мест на пос. талоне
        -- максимальная длина строки - 6 символов
        -- Если рассадка горизонтальная, то формат однозначный 200А-Б.
        -- При вертикальной рассадке возможны 5 вариантов:
        -- 1. Переход номера ряда с 2-значного на 3-значный 99-01А
        -- 2. Не 3-начные номера ряда без перехода через десятку 45-7A
        -- 3. Не 3-начные номера ряда с переходом через десятку 49-51А
        -- 4. 3-начные номера ряда без перехода через десятку 150-2A
        -- 5. 3-начные номера ряда с переходом через десятку 19901А
    IF vfirst_yname=vlast_yname AND vfirst_xname=vlast_xname THEN
      /* одно место */
      res_voland:=LTRIM(vfirst_yname,'0')||vfirst_xname;
      RETURN res_voland;
    END IF;
    IF vfirst_yname=vlast_yname THEN
      /* рассадка горизонтальная */
      res_voland:=LTRIM(vfirst_yname,'0')||vfirst_xname||'-'||vlast_xname;
      RETURN res_voland;
    END IF;
    IF vfirst_xname=vlast_xname AND
       LENGTH(vfirst_yname)=3 AND LENGTH(vlast_yname)=3 THEN
      /* рассадка вертикальная */
      i:=1;
      WHILE i<=3 AND SUBSTR(vfirst_yname,i,1)=SUBSTR(vlast_yname,i,1) LOOP
        i:=i+1;
      END LOOP;
      IF i=1 THEN i:=2; END IF;
      res_voland:=LTRIM(vfirst_yname,'0')||'-'||SUBSTR(vlast_yname,i)||vfirst_xname;
      IF LENGTH(res_voland)>6 THEN
        res_voland:=LTRIM(vfirst_yname,'0')||SUBSTR(vlast_yname,i)||vfirst_xname;
      END IF;
      RETURN res_voland;
    END IF;
  END IF;
  RETURN res_num;
END get_seat_no;

FUNCTION normalize_bort(str points.bort%TYPE) RETURN points.bort%TYPE
IS
vbort      points.bort%TYPE;
c          CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=UPPER(SUBSTR(str,i,1));
    IF NOT c BETWEEN 'A' AND 'Z' AND
       NOT c BETWEEN 'А' AND 'Я' AND
       c<>'Ё' AND
       NOT c BETWEEN '0' AND '9' THEN
      NULL;
    ELSE
      BEGIN
        vbort:=vbort||c;
      EXCEPTION
        WHEN VALUE_ERROR THEN NULL;
      END;
    END IF;
  END LOOP;
  RETURN vbort;
END normalize_bort;

FUNCTION check_bort(str VARCHAR2) RETURN points.bort%TYPE
IS
vbort      points.bort%TYPE;
c          CHAR(1);
last_delim CHAR(1);
lparams   system.TLexemeParams;
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  last_delim:=NULL;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF ASCII(c)>=0 AND ASCII(c)<ASCII(' ') THEN c:=' '; END IF;
    IF NOT c BETWEEN 'A' AND 'Z' AND
       NOT c BETWEEN 'А' AND 'Я' AND
       c<>'Ё' AND
       NOT c BETWEEN '0' AND '9' AND
       c<>'-' AND c<>' ' THEN
      lparams('symbol'):=c;
      system.raise_user_exception('MSG.INVALID_CHARS_IN_BOARD_NUM', lparams);
    END IF;

    IF c<>'-' AND c<>' ' THEN
      BEGIN
        IF vbort IS NULL THEN
          vbort:=c;
        ELSE
          vbort:=vbort||last_delim||c;
        END IF;
      EXCEPTION
        WHEN VALUE_ERROR THEN
          system.raise_user_exception('MSG.INVALID_BOARD_NUM');
      END;
      last_delim:=NULL;
    ELSE
      IF last_delim='-' THEN NULL; ELSE last_delim:=c; END IF;
    END IF;
  END LOOP;
  IF NOT LENGTH(vbort) BETWEEN 2 AND 10 THEN
    system.raise_user_exception('MSG.INVALID_BOARD_NUM');
  END IF;
  RETURN vbort;
END check_bort;

FUNCTION is_waitlist(vpax_id     IN pax.pax_id%TYPE,
                     vseats      IN pax.seats%TYPE,
                     vgrp_status IN pax_grp.status%TYPE,
                     vpoint_id   IN trip_sets.point_id%TYPE,
                     row	 IN NUMBER DEFAULT 1) RETURN NUMBER
IS
vpoint_dep 	trip_sets.point_id%TYPE;
vrow 		NUMBER;
BEGIN
--dbms_output.put_line( row );
  IF vpax_id IS NULL THEN
    seatInfo.point_id := NULL;
    RETURN 0;
  END IF;
  IF vpoint_id IS NULL THEN
    BEGIN
      SELECT pax_grp.point_dep
      INTO vpoint_dep
      FROM pax_grp,pax
      WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=vpax_id;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
       seatInfo.point_id:=NULL; --след. вызов инициализация seatInfo
       RETURN 0;
    END;
  ELSE
    vpoint_dep:=vpoint_id;
  END IF;

  IF row<>1 AND
     vpoint_dep=seatInfo.point_id THEN
    vrow:=row;
  ELSE --инициализация seatInfo
    vrow:=1;
    seatInfo.point_id:=vpoint_dep;
    seatInfo.pr_free_seating:=0;
    BEGIN
      SELECT NVL(trip_sets.pr_free_seating,0)
      INTO seatInfo.pr_free_seating
      FROM trip_sets
      WHERE point_id=vpoint_dep;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN NULL;
    END;
  END IF;
  IF seatInfo.pr_free_seating<>0 THEN
    RETURN 0;
  END IF;
  IF vseats=0 THEN
    IF vrow=1 AND vseats=0 THEN seatInfo.point_id:=NULL; END IF; --след. вызов инициализация seatInfo, т.к. в get_seat_no она не прошла
    RETURN 0;
  END IF;
  IF get_seat_no(vpax_id,vseats,vgrp_status,vpoint_id,'one',vrow) IS NULL THEN
   RETURN 1;
  ELSE
   RETURN 0;
  END IF;
END is_waitlist;

END salons;
/
