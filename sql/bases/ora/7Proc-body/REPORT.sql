create or replace PACKAGE BODY report
AS

FUNCTION get_airline_period_last_date(vairline pacts.airline%TYPE,
                                      period_first_date pacts.first_date%TYPE) RETURN DATE
IS
res DATE;
BEGIN
  SELECT MIN(period_last_date)
  INTO res
  FROM
    (SELECT first_date AS period_last_date
     FROM pacts
     WHERE airline=vairline AND first_date>period_first_date
     UNION
     SELECT last_date
     FROM pacts
     WHERE airline=vairline AND last_date>period_first_date AND last_date IS NOT NULL);
  RETURN res;
END;

FUNCTION get_airp_period_last_date(vairp pacts.airp%TYPE,
                                   period_first_date pacts.first_date%TYPE) RETURN DATE
IS
res DATE;
BEGIN
  SELECT MIN(period_last_date)
  INTO res
  FROM
    (SELECT first_date AS period_last_date
     FROM pacts
     WHERE airp=vairp AND first_date>period_first_date
     UNION
     SELECT last_date
     FROM pacts
     WHERE airp=vairp AND last_date>period_first_date AND last_date IS NOT NULL);
  RETURN res;
END;

FUNCTION get_TKNO(vpax_id IN crs_pax.pax_id%TYPE,
                  et_term IN CHAR DEFAULT '/',
                  only_TKNE IN NUMBER DEFAULT NULL) RETURN VARCHAR2
IS
CURSOR TKNCur IS
  SELECT ticket_no,DECODE(rem_code,'TKNE',coupon_no,NULL) AS coupon_no
  FROM crs_pax_tkn
  WHERE pax_id=vpax_id AND (NVL(only_TKNE,0)=0 OR rem_code='TKNE')
  ORDER BY DECODE(rem_code,'TKNE',0,'TKNA',1,'TKNO',2,3),ticket_no,coupon_no;
TKNCurRow TKNCur%ROWTYPE;
result VARCHAR2(250);
BEGIN
  result:=NULL;
  OPEN TKNCur;
  FETCH TKNCur INTO TKNCurRow;
  IF TKNCur%FOUND THEN
    result:=TKNCurRow.ticket_no;
    IF TKNCurRow.coupon_no IS NOT NULL THEN
      result:=result||et_term||TKNCurRow.coupon_no;
    END IF;
  END IF;
  CLOSE TKNCur;
  RETURN result;
END get_TKNO;

FUNCTION get_trfer_airline(str	      IN airlines.code%TYPE,
                           pr_lat     IN INTEGER) RETURN airlines.code%TYPE
IS
vairline	airlines.code%TYPE;
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  BEGIN
    SELECT DECODE(NVL(pr_lat,0),0,code,NVL(code_lat,code)) INTO vairline
    FROM airlines WHERE str IN (code,code_lat);
  EXCEPTION
    WHEN NO_DATA_FOUND THEN vairline:=str;
    WHEN TOO_MANY_ROWS THEN vairline:=str;
  END;
  RETURN vairline;
END get_trfer_airline;

FUNCTION get_trfer_airp(str	   IN airps.code%TYPE,
                        pr_lat     IN INTEGER) RETURN airps.code%TYPE
IS
vairp		airps.code%TYPE;
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  BEGIN
    SELECT DECODE(NVL(pr_lat,0),0,code,NVL(code_lat,code)) INTO vairp
    FROM airps WHERE str IN (code,code_lat);
  EXCEPTION
    WHEN NO_DATA_FOUND THEN vairp:=str;
    WHEN TOO_MANY_ROWS THEN vairp:=str;
  END;
  RETURN vairp;
END get_trfer_airp;

FUNCTION get_trfer_airp_name(str	IN airps.code%TYPE,
                             pr_lat     IN INTEGER) RETURN airps.name%TYPE
IS
vname	airps.name%TYPE;
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  BEGIN
    SELECT DECODE(NVL(pr_lat,0),0,name,NVL(name_lat,name)) INTO vname
    FROM airps WHERE str IN (code,code_lat);
  EXCEPTION
    WHEN NO_DATA_FOUND THEN vname:=str;
    WHEN TOO_MANY_ROWS THEN vname:=str;
  END;
  RETURN vname;
END get_trfer_airp_name;

FUNCTION get_last_trfer(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2
IS
res     VARCHAR2(50);
BEGIN
  SELECT airline||flt_no||suffix||'/'||airp_arv AS last_trfer
  INTO res
  FROM trfer_trips,transfer
  WHERE transfer.point_id_trfer=trfer_trips.point_id AND
        transfer.grp_id=vgrp_id AND transfer.pr_final<>0;
  RETURN res;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END;

FUNCTION get_last_trfer_airp(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2
IS
res     VARCHAR2(3);
BEGIN
  SELECT airp_arv
  INTO res
  FROM transfer
  WHERE transfer.grp_id=vgrp_id AND transfer.pr_final<>0;
  RETURN res;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END;

FUNCTION get_last_tckin_seg(vgrp_id IN pax_grp.grp_id%TYPE) RETURN VARCHAR2
IS
res     VARCHAR2(50);
BEGIN
  SELECT airline||flt_no||suffix||'/'||airp_arv INTO res
  FROM trfer_trips,tckin_segments
  WHERE tckin_segments.point_id_trfer=trfer_trips.point_id AND
        tckin_segments.grp_id=vgrp_id AND
        tckin_segments.pr_final<>0;
  RETURN res;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END;

END report;
/
