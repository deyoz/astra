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

FUNCTION get_word(str IN OUT VARCHAR2) RETURN VARCHAR2
IS
c       CHAR(1);
res	VARCHAR2(2000);
i       BINARY_INTEGER;
len     BINARY_INTEGER;
BEGIN
  i:=1;
  c:=NULL;
--пропускаем все символы не относящиеся к слову
  LOOP
    c:=SUBSTR(str,i,1);
    EXIT WHEN c IS NULL OR
        (c BETWEEN 'A' AND 'Z' OR
         c BETWEEN 'a' AND 'z' OR
         c BETWEEN 'А' AND 'Я' OR
         c BETWEEN 'а' AND 'я' OR
         c BETWEEN '0' AND '9');
    i:=i+1;
  END LOOP;
  len:=0;
  LOOP
    c:=SUBSTR(str,i+len,1);
    EXIT WHEN c IS NULL OR
     NOT(c BETWEEN 'A' AND 'Z' OR
         c BETWEEN 'a' AND 'z' OR
         c BETWEEN 'А' AND 'Я' OR
         c BETWEEN 'а' AND 'я' OR
         c BETWEEN '0' AND '9');
    len:=len+1;
  END LOOP;
  IF len=0 THEN res:=NULL; ELSE res:=SUBSTR(str,i,len); END IF;
  str:=SUBSTR(str,i+len);
  RETURN res;
END get_word;

FUNCTION get_TKNO2(vpax_id IN crs_pax.pax_id%TYPE,
                   et_term IN CHAR DEFAULT '/',
                   only_TKNE IN NUMBER DEFAULT NULL) RETURN VARCHAR2
IS
c       CHAR(1);
vrem    crs_pax_rem.rem%TYPE;
res	crs_pax_rem.rem%TYPE;
result  crs_pax_rem.rem%TYPE;
i       BINARY_INTEGER;
j       BINARY_INTEGER;
CURSOR RemCur IS
  SELECT rem_code,rem FROM crs_pax_rem
  WHERE pax_id=vpax_id AND
        rem_code IN ('TKNO','TKNA','TKNE','TKNM','TKN','TKT','TKTN','TKTNO','TTKNR','TTKNO') AND
        (NVL(only_TKNE,0)=0 OR rem_code='TKNE')
  ORDER BY DECODE(rem_code,'TKNE',0,'TKNA',1,'TKNO',2,3),rem;
BEGIN
  result:=NULL;
  FOR RemCurRow IN RemCur LOOP
    vrem:=RemCurRow.rem;

    res:=get_word(vrem);
    WHILE res IS NOT NULL LOOP
      j:=0;
      FOR i IN 1..LENGTH(res) LOOP
        c:=SUBSTR(res,i,1);
        IF c BETWEEN '0' AND '9' THEN
          j:=j+1;
          IF j>7 THEN EXIT; END IF;
        ELSE
          j:=0;
        END IF;
      END LOOP;
      IF j>7 THEN EXIT; END IF;
      res:=get_word(vrem);
    END LOOP;
    IF j>7 THEN
      IF RemCurRow.rem_code='TKNE' THEN
        result:=res;
        IF SUBSTR(vrem,1,1)='/' THEN
          res:=get_word(vrem);
          IF LENGTH(res)=1 AND SUBSTR(res,1,1) BETWEEN '1' AND '4' THEN
            result:=result||et_term||res;
          END IF;
        END IF;
        IF result LIKE 'INF%' THEN
          result:=SUBSTR(result,4);
        ELSE
          EXIT;
        END IF;
      ELSE
        IF result IS NULL THEN result:=res; END IF;
        EXIT;
      END IF;
    END IF;
  END LOOP;
  RETURN result;
END get_TKNO2;

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
result  crs_pax_rem.rem%TYPE;
BEGIN
  result:=NULL;
  OPEN TKNCur;
  FETCH TKNCur INTO TKNCurRow;
  IF TKNCur%FOUND THEN
    result:=TKNCurRow.ticket_no;
    IF TKNCurRow.coupon_no IS NOT NULL THEN
      result:=result||et_term||TKNCurRow.coupon_no;
    END IF;
  ELSE
    result:=get_TKNO2(vpax_id,et_term,only_TKNE);
  END IF;
  CLOSE TKNCur;
  RETURN result;
END get_TKNO;

FUNCTION get_PSPT2(vpax_id IN crs_pax.pax_id%TYPE,
                   with_issue_country IN NUMBER DEFAULT 0,
                   vlang   IN lang_types.code%TYPE DEFAULT 'RU') RETURN VARCHAR2
IS
c       CHAR(1);
vrem            crs_pax_rem.rem%TYPE;
res	        crs_pax_rem.rem%TYPE;
result          crs_pax_rem.rem%TYPE;
i               BINARY_INTEGER;
j               BINARY_INTEGER;
k               BINARY_INTEGER;
CURSOR RemCur IS
  SELECT rem FROM crs_pax_rem
  WHERE pax_id=vpax_id AND rem_code IN ('PSPT','DOCS')
  ORDER BY DECODE(rem_code,'DOCS',0,1),rem;
RemCurRow       RemCur%ROWTYPE;
issue_country   VARCHAR2(3);
BEGIN
  result:=NULL;
  issue_country:=NULL;
  OPEN RemCur;
  FETCH RemCur INTO RemCurRow;
  IF RemCur%FOUND THEN
    vrem:=RemCurRow.rem;
  ELSE
    vrem:='';
  END IF;
  CLOSE RemCur;
  res:=get_word(vrem);
  IF res='PSPT' THEN
    res:=get_word(vrem);
    WHILE res IS NOT NULL LOOP
      /*проверим на HK1 и т.д.*/
      IF LENGTH(res)=3 AND
         (SUBSTR(res,1,1) BETWEEN 'A' AND 'Z' OR
          SUBSTR(res,1,1) BETWEEN 'a' AND 'z') AND
         (SUBSTR(res,2,1) BETWEEN 'A' AND 'Z' OR
          SUBSTR(res,2,1) BETWEEN 'a' AND 'z') AND
         SUBSTR(res,3,1)='1' THEN
        NULL;
      ELSE
        j:=0;
        FOR i IN 1..LENGTH(res) LOOP
          c:=SUBSTR(res,i,1);
          IF c BETWEEN '0' AND '9' THEN
            j:=j+1;
            IF j>4 THEN EXIT; END IF;
          ELSE
            j:=0;
          END IF;
        END LOOP;
        IF j>4 THEN
          result:=res;
          res:=get_word(vrem);
          /*допишем код гос-ва*/
          IF LENGTH(res) IN (2,3) AND
             NOT SUBSTR(res,1,1) BETWEEN '0' AND '9' AND
             NOT SUBSTR(res,2,1) BETWEEN '0' AND '9' AND
             (SUBSTR(res,3,1) IS NULL OR
              NOT SUBSTR(res,3,1) BETWEEN '0' AND '9') THEN
            issue_country:=res;
          END IF;
          EXIT;
        END IF;
      END IF;
      res:=get_word(vrem);
    END LOOP;
  END IF;
  IF res='DOCS' THEN
    k:=1;
    res:=get_word(vrem);
    WHILE res IS NOT NULL LOOP
      IF k=3 THEN
        IF LENGTH(res) IN (2,3) AND
           NOT SUBSTR(res,1,1) BETWEEN '0' AND '9' AND
           NOT SUBSTR(res,2,1) BETWEEN '0' AND '9' AND
           (SUBSTR(res,3,1) IS NULL OR
            NOT SUBSTR(res,3,1) BETWEEN '0' AND '9') THEN
          issue_country:=res;
        END IF;
      END IF;
      IF k=4 THEN
        j:=0;
        FOR i IN 1..LENGTH(res) LOOP
          c:=SUBSTR(res,i,1);
          IF c BETWEEN '0' AND '9' THEN
            j:=j+1;
            IF j>4 THEN EXIT; END IF;
          ELSE
            j:=0;
          END IF;
        END LOOP;
        IF j>4 THEN
          result:=res;
        END IF;
        EXIT;
      END IF;
      k:=k+1;
      res:=get_word(vrem);
    END LOOP;
  END IF;
  IF result IS NOT NULL AND with_issue_country<>0 THEN
    BEGIN
      SELECT code INTO issue_country
      FROM pax_doc_countries
      WHERE code=issue_country AND pr_del=0;
      result:=result||' '||issue_country;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN
      BEGIN
        SELECT pax_doc_countries.code INTO issue_country
        FROM countries, pax_doc_countries
        WHERE pax_doc_countries.country=countries.code AND
              pax_doc_countries.pr_del=0 AND countries.pr_del=0 AND
              issue_country IN (countries.code, countries.code_lat, countries.code_iso);
        result:=result||' '||issue_country;
      EXCEPTION
       WHEN NO_DATA_FOUND THEN NULL;
       WHEN TOO_MANY_ROWS THEN NULL;
      END;
    END;
  END IF;
  RETURN result;
END get_PSPT2;

FUNCTION get_PSPT(vpax_id IN crs_pax.pax_id%TYPE,
                  with_issue_country IN NUMBER DEFAULT 0,
                  vlang   IN lang_types.code%TYPE DEFAULT 'RU') RETURN VARCHAR2
IS
CURSOR DocCur IS
  SELECT issue_country,no FROM crs_pax_doc
  WHERE pax_id=vpax_id AND no IS NOT NULL
  ORDER BY DECODE(type,'P',0,NULL,2,1),DECODE(rem_code,'DOCS',0,1),no;
DocCurRow DocCur%ROWTYPE;
result          crs_pax_rem.rem%TYPE;
BEGIN
  result:=NULL;
  OPEN DocCur;
  FETCH DocCur INTO DocCurRow;
  IF DocCur%FOUND THEN
    result:=DocCurRow.no;
    IF result IS NOT NULL AND with_issue_country<>0 AND
       DocCurRow.issue_country IS NOT NULL THEN
      result:=result||' '||DocCurRow.issue_country;
    END IF;
  ELSE
    result:=get_PSPT2(vpax_id, with_issue_country);
  END IF;
  CLOSE DocCur;
  RETURN result;
END get_PSPT;

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
