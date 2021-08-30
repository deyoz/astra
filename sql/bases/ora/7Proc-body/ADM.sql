create or replace PACKAGE BODY adm
AS

TYPE TFltList IS TABLE OF trfer_set_flts.flt_no%TYPE INDEX BY BINARY_INTEGER;
/*TYPE TRemList IS TABLE OF rem_grp_list.rem_code%TYPE INDEX BY BINARY_INTEGER;*/

TYPE sync_typeb_options_cur IS REF CURSOR;

TYPE periods_row IS RECORD
(
id NUMBER(9),
first_date DATE,
last_date DATE
);

TYPE periods_cur IS REF CURSOR RETURN periods_row;

PROCEDURE check_chars_in_name(str	  IN VARCHAR2,
                              pr_lat      IN INTEGER,
                              symbols     IN VARCHAR2,
                              cache_code  IN cache_tables.code%TYPE,
                              cache_field IN cache_fields.name%TYPE,
                              vlang       IN lang_types.code%TYPE)
IS
c CHAR(1);
info	 adm.TCacheInfo;
lparams  system.TLexemeParams;
BEGIN
  c:=system.invalid_char_in_name(str, pr_lat, symbols);
  IF c IS NOT NULL THEN
    info:=adm.get_cache_info(cache_code);
    lparams('field_name'):=info.field_title(cache_field);
    lparams('symbol'):=c;
    system.raise_user_exception('MSG.FIELD_INCLUDE_INVALID_CHARACTER1', lparams);
  END IF;
END check_chars_in_name;

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

PROCEDURE modify_originator(
       vid              typeb_originators.id%TYPE,
       vlast_date       typeb_originators.last_date%TYPE,
       vlang            lang_types.code%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             typeb_originators.tid%TYPE DEFAULT NULL)
IS
r typeb_originators%ROWTYPE;
BEGIN
  SELECT id,airline,airp_dep,tlg_type,first_date,addr,double_sign,descr
  INTO   r.id,r.airline,r.airp_dep,r.tlg_type,r.first_date,r.addr,r.double_sign,r.descr
  FROM typeb_originators WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_originator(r.id,r.airline,r.airp_dep,r.tlg_type,
                 r.first_date,vlast_date,r.addr,r.double_sign,r.descr,vtid,vlang,vsetting_user,vstation);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_originator;

PROCEDURE delete_originator(
       vid              typeb_originators.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             typeb_originators.tid%TYPE DEFAULT NULL)
IS
now             DATE;
vfirst_date     DATE;
vlast_date      DATE;
tidh    typeb_originators.tid%TYPE;
BEGIN
  now:=system.UTCSYSDATE;
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM typeb_originators
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>now THEN
    IF vfirst_date<now THEN
      UPDATE typeb_originators SET last_date=now,tid=tidh WHERE id=vid;
      hist.synchronize_history('typeb_originators',vid,vsetting_user,vstation);
    ELSE
      UPDATE typeb_originators SET pr_del=1,tid=tidh WHERE id=vid;
      hist.synchronize_history('typeb_originators',vid,vsetting_user,vstation);
    END IF;
  ELSE
    /* ᯥ樠�쭮 �⮡� � ��� ������ ������������ ��ப� */
    UPDATE typeb_originators SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_originator;

PROCEDURE add_originator(
       vid       IN OUT typeb_originators.id%TYPE,
       vairline         typeb_originators.airline%TYPE,
       vairp_dep        typeb_originators.airp_dep%TYPE,
       vtlg_type        typeb_originators.tlg_type%TYPE,
       vfirst_date      typeb_originators.first_date%TYPE,
       vlast_date       typeb_originators.last_date%TYPE,
       vaddr            typeb_originators.addr%TYPE,
       vdouble_sign     typeb_originators.double_sign%TYPE,
       vdescr           typeb_originators.descr%TYPE,
       vtid             typeb_originators.tid%TYPE,
       vlang            lang_types.code%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,first_date,last_date
  FROM typeb_originators
/*���஡����� ��⨬���஢��� �����*/
  WHERE (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
        (airp_dep IS NULL AND vairp_dep IS NULL OR airp_dep=vairp_dep) AND
        (tlg_type IS NULL AND vtlg_type IS NULL OR tlg_type=vtlg_type) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     typeb_originators.id%TYPE;
tidh    typeb_originators.tid%TYPE;
pr_opd  BOOLEAN;
info	 adm.TCacheInfo;
lparams  system.TLexemeParams;
BEGIN
  adm.check_period(vid IS NULL,vfirst_date,vlast_date,system.UTCSYSDATE,first,last,pr_opd);

  IF not(pr_opd) THEN
    IF vaddr IS NULL THEN
      info:=adm.get_cache_info('TYPEB_ORIGINATORS');
      lparams('fieldname'):=info.field_title('ADDR');
      system.raise_user_exception('MSG.TABLE.NOT_SET_FIELD_VALUE', lparams);
    ELSE
      check_sita_addr(vaddr, 'TYPEB_ORIGINATORS', 'ADDR', vlang);
    END IF;
    IF vdouble_sign IS NOT NULL THEN
      IF LENGTH(vdouble_sign)<>2 OR system.is_upp_let_dig(vdouble_sign,1)=0 THEN
        info:=adm.get_cache_info('TYPEB_ORIGINATORS');
        lparams('fieldname'):=info.field_title('DOUBLE_SIGN');
        system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
      END IF;
    END IF;
    IF vdescr IS NULL THEN
      info:=adm.get_cache_info('TYPEB_ORIGINATORS');
      lparams('fieldname'):=info.field_title('DESCR');
      system.raise_user_exception('MSG.TABLE.NOT_SET_FIELD_VALUE', lparams);
    END IF;
  END IF;

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;


  /* �஡㥬 ࠧ���� �� ��१�� */
  FOR curRow IN cur LOOP
    idh:=curRow.id;
    IF vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id THEN
      IF curRow.first_date<first THEN
        /* ��१�� [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE typeb_originators SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          hist.synchronize_history('typeb_originators',curRow.id,vsetting_user,vstation);
          idh:=NULL;
        ELSE
          SELECT id__seq.nextval INTO idh FROM dual;
          INSERT INTO typeb_originators(id,airline,airp_dep,tlg_type,
                                        first_date,last_date,addr,double_sign,descr,pr_del,tid)
          SELECT idh,airline,airp_dep,tlg_type,
                 curRow.first_date,first,addr,double_sign,descr,0,tidh
          FROM typeb_originators WHERE id=curRow.id;
          hist.synchronize_history('typeb_originators',idh,vsetting_user,vstation);
          idh:=NULL;
        END IF;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* ��१�� [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE typeb_originators SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          hist.synchronize_history('typeb_originators',curRow.id,vsetting_user,vstation);
          idh:=NULL;
        ELSE
          SELECT id__seq.nextval INTO idh FROM dual;
          INSERT INTO typeb_originators(id,airline,airp_dep,tlg_type,
                                first_date,last_date,addr,double_sign,descr,pr_del,tid)
          SELECT idh,airline,airp_dep,tlg_type,
                 last,curRow.last_date,addr,double_sign,descr,0,tidh
          FROM typeb_originators WHERE id=curRow.id;
          hist.synchronize_history('typeb_originators',idh,vsetting_user,vstation);
          idh:=NULL;
        END IF;
      END IF;

      IF idh IS NOT NULL THEN
        UPDATE typeb_originators SET pr_del=1,tid=tidh WHERE id=curRow.id;
        hist.synchronize_history('typeb_originators',curRow.id,vsetting_user,vstation);
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd THEN
      /*���� ��१�� [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO typeb_originators(id,airline,airp_dep,tlg_type,
                            first_date,last_date,addr,double_sign,descr,pr_del,tid)
      VALUES(vid,vairline,vairp_dep,vtlg_type,
             first,last,vaddr,vdouble_sign,vdescr,0,tidh) RETURNING id INTO vid;
      hist.synchronize_history('typeb_originators',vid,vsetting_user,vstation);
    END IF;
  ELSE
    /* �� ।���஢���� �����⨬ ��ப� */
    UPDATE typeb_originators SET last_date=last,tid=tidh WHERE id=vid;
    hist.synchronize_history('typeb_originators',vid,vsetting_user,vstation);
  END IF;
END add_originator;

PROCEDURE check_hall_airp(vhall_id       IN halls2.id%TYPE,
                          vpoint_id      IN points.point_id%TYPE)
IS
vairp airps.code%TYPE;
BEGIN
  IF vpoint_id IS NOT NULL THEN
    SELECT airp INTO vairp FROM points WHERE point_id=vpoint_id;
    vairp:=check_hall_airp(vhall_id, vairp);
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END check_hall_airp;

FUNCTION check_hall_airp(vhall_id       IN halls2.id%TYPE,
                         vairp          IN airps.code%TYPE) RETURN airps.code%TYPE
IS
vairp2 airps.code%TYPE;
BEGIN
  vairp2:=vairp;
  IF vhall_id IS NOT NULL THEN
    SELECT airp INTO vairp2 FROM halls2 WHERE id=vhall_id;
    IF vairp IS NOT NULL AND vairp<>vairp2 THEN
        system.raise_user_exception('MSG.HALL_DOES_NOT_MEET_AIRP_DEP');
    END IF;
  END IF;
  RETURN vairp2;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN vairp;
END check_hall_airp;

PROCEDURE check_terminal_airp(vterminal_id   IN airp_terminals.id%TYPE,
                              vpoint_id      IN points.point_id%TYPE)
IS
vairp airps.code%TYPE;
BEGIN
  IF vpoint_id IS NOT NULL THEN
    SELECT airp INTO vairp FROM points WHERE point_id=vpoint_id;
    vairp:=check_terminal_airp(vterminal_id, vairp);
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END check_terminal_airp;

FUNCTION check_terminal_airp(vterminal_id   IN airp_terminals.id%TYPE,
                             vairp          IN airps.code%TYPE) RETURN airps.code%TYPE
IS
vairp2 airps.code%TYPE;
BEGIN
  vairp2:=vairp;
  IF vterminal_id IS NOT NULL THEN
    SELECT airp INTO vairp2 FROM airp_terminals WHERE id=vterminal_id;
    IF vairp IS NOT NULL AND vairp<>vairp2 THEN
        system.raise_user_exception('MSG.TERMINAL_DOES_NOT_MEET_AIRP_DEP');
    END IF;
  END IF;
  RETURN vairp2;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN vairp;
END check_terminal_airp;

FUNCTION check_right_access(vright_id IN rights_list.ida%TYPE,
                            vuser_id IN users2.user_id%TYPE,
                            vexception IN NUMBER) RETURN rights_list.ida%TYPE
IS
n       INTEGER;
BEGIN
  SELECT COUNT(*) INTO n
  FROM user_roles,role_assign_rights
  WHERE user_roles.role_id=role_assign_rights.role_id AND
        user_roles.user_id=vuser_id AND role_assign_rights.right_id=vright_id AND rownum<2;
  IF n=0 THEN
    IF vexception<>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_SYSTEM_OPERATION');
      ELSE
        system.raise_user_exception('MSG.ACCESS.NO_PERM_MODIFY_SYSTEM_OPERATION');
      END IF;
    END IF;
  END IF;
  RETURN vright_id;
END check_right_access;

FUNCTION check_role_aro_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE,
                           view_only IN BOOLEAN) RETURN NUMBER
IS
  CURSOR cur1(vuser_id IN users2.user_id%TYPE) IS
    SELECT airline FROM aro_airlines WHERE aro_id=vuser_id;
  CURSOR cur2(vuser_id IN users2.user_id%TYPE) IS
    SELECT airp FROM aro_airps WHERE aro_id=vuser_id;
vairline airlines.code%TYPE;
vairp airps.code%TYPE;
cur1Row cur1%ROWTYPE;
cur2Row cur2%ROWTYPE;
TYPE TAirlines IS TABLE OF aro_airlines.airline%TYPE INDEX BY BINARY_INTEGER;
TYPE TAirps    IS TABLE OF aro_airps.airp%TYPE      INDEX BY BINARY_INTEGER;
aro_airlines2   TAirlines;
aro_airps2      TAirps;
i               BINARY_INTEGER;
i2              BINARY_INTEGER;
j2              BINARY_INTEGER;
access_denied   BOOLEAN;
BEGIN
  SELECT airline,airp INTO vairline,vairp FROM roles WHERE role_id=vrole_id;
  IF check_airline_access(vairline,vuser_id)<>0 AND
     check_airp_access(vairp,vuser_id)<>0 THEN
    NULL;
  ELSE
    /* �஢�ਬ �᪫�祭�� */
    access_denied:=TRUE;
    i:=1;
    aro_airlines2(1):=NULL;
    FOR cur1Row IN cur1(vuser_id) LOOP
      aro_airlines2(i):=cur1Row.airline;
      i:=i+1;
    END LOOP;
    i:=1;
    aro_airps2(1):=NULL;
    FOR cur2Row IN cur2(vuser_id) LOOP
      aro_airps2(i):=cur2Row.airp;
      i:=i+1;
    END LOOP;
    FOR i2 IN 1..aro_airlines2.count LOOP
      FOR j2 IN 1..aro_airps2.count LOOP
        IF view_only THEN
          SELECT COUNT(*) INTO i
          FROM extra_role_access
          WHERE (airline_from=aro_airlines2(i2) OR airline_from IS NULL AND aro_airlines2(i2) IS NULL) AND
                (airline_to=vairline OR airline_to IS NULL AND vairline IS NULL) AND
                (airp_from=aro_airps2(j2) OR airp_from IS NULL AND aro_airps2(j2) IS NULL) AND
                (airp_to=vairp OR airp_to IS NULL AND vairp IS NULL);
        ELSE
          SELECT COUNT(*) INTO i
          FROM extra_role_access
          WHERE (airline_from=aro_airlines2(i2) OR airline_from IS NULL AND aro_airlines2(i2) IS NULL) AND
                (airline_to=vairline OR airline_to IS NULL AND vairline IS NULL) AND
                (airp_from=aro_airps2(j2) OR airp_from IS NULL AND aro_airps2(j2) IS NULL) AND
                (airp_to=vairp OR airp_to IS NULL AND vairp IS NULL) AND
                full_access<>0;
        END IF;
        IF i>0 THEN
          access_denied:=FALSE;
          GOTO end_loop;
        END IF;
      END LOOP;
    END LOOP;
    <<end_loop>>
    aro_airlines2.delete;
    aro_airps2.delete;
    IF access_denied THEN RETURN 0; END IF;
  END IF;
  RETURN 1;
END check_role_aro_access;

FUNCTION check_role_aro_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE) RETURN NUMBER
IS
begin
    return check_role_aro_access(vrole_id, vuser_id, FALSE);
end;

FUNCTION check_role_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE,
                           view_only IN BOOLEAN) RETURN NUMBER
IS
vright_id rights_list.ida%TYPE;
BEGIN
  IF check_role_aro_access(vrole_id, vuser_id, view_only) = 0 THEN
    RETURN 0;
  ELSIF view_only THEN
    RETURN 1;
  END IF;

  BEGIN
    SELECT role_rights.right_id INTO vright_id
    FROM
      (SELECT role_rights.right_id
       FROM role_rights
       WHERE role_id=vrole_id
       UNION
       SELECT role_assign_rights.right_id
       FROM role_assign_rights
       WHERE role_id=vrole_id) role_rights,
      (SELECT role_assign_rights.right_id
       FROM user_roles,role_assign_rights
       WHERE user_roles.role_id=role_assign_rights.role_id AND
             user_roles.user_id=vuser_id) user_rights
    WHERE role_rights.right_id=user_rights.right_id(+) AND
          user_rights.right_id IS NULL AND rownum<2;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      RETURN 1;
  END;

  RETURN 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    RETURN 1;
END check_role_access;

FUNCTION check_role_view_access(vrole_id IN roles.role_id%TYPE,
                                vuser_id IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_role_access(vrole_id,vuser_id,TRUE);
END check_role_view_access;

FUNCTION check_role_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_role_access(vrole_id,vuser_id,FALSE);
END check_role_access;

FUNCTION check_role_access(vrole_id IN roles.role_id%TYPE,
                           vuser_id IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN roles.role_id%TYPE
IS
BEGIN
  IF check_role_access(vrole_id,vuser_id,FALSE)=0 THEN
    IF vexception<>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_USER_ROLE');
      ELSE
        system.raise_user_exception('MSG.ACCESS.NO_PERM_MODIFY_USER_ROLE');
      END IF;
    END IF;
  END IF;
  RETURN vrole_id;
END check_role_access;

FUNCTION check_user_access(vuser_id1 IN users2.user_id%TYPE,
                           vuser_id2 IN users2.user_id%TYPE,
                           view_only IN BOOLEAN) RETURN NUMBER
IS
  CURSOR cur1(vuser_id IN users2.user_id%TYPE) IS
    SELECT airline FROM aro_airlines WHERE aro_id=vuser_id;
  CURSOR cur2(vuser_id IN users2.user_id%TYPE) IS
    SELECT airp FROM aro_airps WHERE aro_id=vuser_id;

vright_id   rights_list.ida%TYPE;
vuser_type1 user_types.code%TYPE;
vuser_type2 user_types.code%TYPE;
vpr_denial  users2.pr_denial%TYPE;
cur1Row cur1%ROWTYPE;
cur2Row cur2%ROWTYPE;
TYPE TAirlines IS TABLE OF aro_airlines.airline%TYPE INDEX BY BINARY_INTEGER;
TYPE TAirps    IS TABLE OF aro_airps.airp%TYPE      INDEX BY BINARY_INTEGER;
aro_airlines1   TAirlines;
aro_airlines2   TAirlines;
aro_airps1      TAirps;
aro_airps2      TAirps;
i               BINARY_INTEGER;
i1              BINARY_INTEGER;
i2              BINARY_INTEGER;
j1              BINARY_INTEGER;
j2              BINARY_INTEGER;
access_denied   BOOLEAN;
BEGIN
  IF NOT(view_only) AND vuser_id1=vuser_id2 THEN
    /* ����㯠 � ᠬ��� ᥡ� ���! */
    RETURN 0;
  END IF;

/*  --�஢�ઠ �ࠢ ����㯠 � ������
  BEGIN
    SELECT user_rights1.right_id INTO vright_id
    FROM
      (SELECT role_rights.right_id
       FROM user_roles,role_rights
       WHERE user_roles.role_id=role_rights.role_id AND
             user_roles.user_id=vuser_id1) user_rights1,
      (SELECT role_rights.right_id
       FROM user_roles,role_rights
       WHERE user_roles.role_id=role_rights.role_id AND
             user_roles.user_id=vuser_id2) user_rights2
    WHERE user_rights1.right_id=user_rights2.right_id(+) AND
          user_rights2.right_id IS NULL AND rownum<2;
    RETURN 0;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN NULL;
  END;*/

  SELECT type,pr_denial INTO vuser_type1,vpr_denial
  FROM users2 WHERE user_id=vuser_id1;

  IF vpr_denial=-1 THEN RETURN 0; END IF;

  SELECT type INTO vuser_type2
  FROM users2 WHERE user_id=vuser_id2;

  access_denied:=FALSE;

  IF vuser_type2<>utSupport AND vuser_type1<>vuser_type2 THEN access_denied:=TRUE; END IF;

  i:=1;
  FOR cur1Row IN cur1(vuser_id1) LOOP
    aro_airlines1(i):=cur1Row.airline;
    IF NOT(access_denied) AND check_airline_access(cur1Row.airline,vuser_id2)=0 THEN
      access_denied:=TRUE;
    END IF;
    i:=i+1;
  END LOOP;
  IF aro_airlines1.count=0 THEN
    aro_airlines1(1):=NULL;
    IF NOT(access_denied) AND
       vuser_type1 IN (utSupport,utAirport) AND check_airline_access(NULL,vuser_id2)=0 THEN
      access_denied:=TRUE;
    END IF;
  END IF;
  i:=1;
  FOR cur2Row IN cur2(vuser_id1) LOOP
    aro_airps1(i):=cur2Row.airp;
    IF NOT(access_denied) AND check_airp_access(cur2Row.airp,vuser_id2)=0 THEN
      access_denied:=TRUE;
    END IF;
    i:=i+1;
  END LOOP;
  IF aro_airps1.count=0 THEN
    aro_airps1(1):=NULL;
    IF NOT(access_denied) AND
       vuser_type1 IN (utSupport,utAirline) AND check_airp_access(NULL,vuser_id2)=0 THEN
      access_denied:=TRUE;
    END IF;
  END IF;

  IF access_denied THEN
    /* �஢�ਬ �᪫�祭�� */
    i:=1;
    aro_airlines2(1):=NULL;
    FOR cur1Row IN cur1(vuser_id2) LOOP
      aro_airlines2(i):=cur1Row.airline;
      i:=i+1;
    END LOOP;
    i:=1;
    aro_airps2(1):=NULL;
    FOR cur2Row IN cur2(vuser_id2) LOOP
      aro_airps2(i):=cur2Row.airp;
      i:=i+1;
    END LOOP;

    FOR i1 IN 1..aro_airlines1.count LOOP
      FOR i2 IN 1..aro_airlines2.count LOOP
        FOR j1 IN 1..aro_airps1.count LOOP
          FOR j2 IN 1..aro_airps2.count LOOP
            IF view_only THEN
              SELECT COUNT(*) INTO i
              FROM extra_user_access
              WHERE type_from=vuser_type2 AND type_to=vuser_type1 AND
                    (airline_from=aro_airlines2(i2) OR airline_from IS NULL AND aro_airlines2(i2) IS NULL) AND
                    (airline_to=aro_airlines1(i1) OR airline_to IS NULL AND aro_airlines1(i1) IS NULL) AND
                    (airp_from=aro_airps2(j2) OR airp_from IS NULL AND aro_airps2(j2) IS NULL) AND
                    (airp_to=aro_airps1(j1) OR airp_to IS NULL AND aro_airps1(j1) IS NULL);
            ELSE
              SELECT COUNT(*) INTO i
              FROM extra_user_access
              WHERE type_from=vuser_type2 AND type_to=vuser_type1 AND
                    (airline_from=aro_airlines2(i2) OR airline_from IS NULL AND aro_airlines2(i2) IS NULL) AND
                    (airline_to=aro_airlines1(i1) OR airline_to IS NULL AND aro_airlines1(i1) IS NULL) AND
                    (airp_from=aro_airps2(j2) OR airp_from IS NULL AND aro_airps2(j2) IS NULL) AND
                    (airp_to=aro_airps1(j1) OR airp_to IS NULL AND aro_airps1(j1) IS NULL) AND
                    full_access<>0;
            END IF;
            IF i>0 THEN
              access_denied:=FALSE;
              GOTO end_loop;
            END IF;
          END LOOP;
        END LOOP;
      END LOOP;
    END LOOP;
  END IF;

  <<end_loop>>
  aro_airlines1.delete;
  aro_airlines2.delete;
  aro_airps1.delete;
  aro_airps2.delete;

  IF access_denied THEN
    RETURN 0;
  ELSE
    RETURN 1;
  END IF;


/*
  --�஢�ઠ �ࠢ ����㯠 � ������������
  OPEN cur1;
  FETCH cur1 INTO cur1Row;
  IF cur1%FOUND THEN
    WHILE cur1%FOUND LOOP
      IF check_airline_access(cur1Row.airline,vuser_id2)=0 THEN
        CLOSE cur1;
        RETURN 0;
      END IF;
      FETCH cur1 INTO cur1Row;
    END LOOP;
  ELSE
    IF vuser_type1 IN (utSupport,utAirport) AND check_airline_access(NULL,vuser_id2)=0 THEN
      CLOSE cur1;
      RETURN 0;
    END IF;
  END IF;
  CLOSE cur1;

  OPEN cur2;
  FETCH cur2 INTO cur2Row;
  IF cur2%FOUND THEN
    WHILE cur2%FOUND LOOP
      IF check_airp_access(cur2Row.airp,vuser_id2)=0 THEN
        CLOSE cur2;
        RETURN 0;
      END IF;
      FETCH cur2 INTO cur2Row;
    END LOOP;
  ELSE
    IF vuser_type1 IN (utSupport,utAirline) AND check_airp_access(NULL,vuser_id2)=0 THEN
      CLOSE cur2;
      RETURN 0;
    END IF;
  END IF;
  CLOSE cur2;
  RETURN 1; */
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN 1;
END check_user_access;

FUNCTION check_user_view_access(vuser_id1 IN users2.user_id%TYPE,
                                vuser_id2 IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_user_access(vuser_id1,vuser_id2,TRUE);
END check_user_view_access;

FUNCTION check_user_access(vuser_id1 IN users2.user_id%TYPE,
                           vuser_id2 IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_user_access(vuser_id1,vuser_id2,FALSE);
END check_user_access;

FUNCTION check_user_access(vuser_id1 IN users2.user_id%TYPE,
                           vuser_id2 IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN users2.user_id%TYPE
IS
BEGIN
  IF check_user_access(vuser_id1,vuser_id2,FALSE)=0 THEN
    IF vexception<>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_USER');
      ELSE
        system.raise_user_exception('MSG.ACCESS.NO_PERM_MODIFY_USER');
      END IF;
    END IF;
  END IF;
  RETURN vuser_id1;
END check_user_access;

FUNCTION check_desk_access(vdesk IN desks.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE,
                           view_only IN BOOLEAN) RETURN NUMBER
IS
vairline  airlines.code%TYPE;
vairp     airps.code%TYPE;
desk_tmp  desk_owners.desk%TYPE;
BEGIN
  SELECT airline,airp INTO vairline,vairp
  FROM desk_grp,desks WHERE desk_grp.grp_id=desks.grp_id AND code=vdesk;
  IF check_airline_access(vairline,vuser_id)<>0 AND
     check_airp_access(vairp,vuser_id)<>0 THEN
    RETURN 1;
  ELSE
    IF view_only AND check_airp_access(vairp,vuser_id)<>0 THEN
      -- �஢�ਬ desk_owners
      BEGIN
        SELECT desk_owners.desk INTO desk_tmp
        FROM desk_owners
        WHERE desk_owners.desk=vdesk AND
              desk_owners.airline IS NOT NULL AND
              check_airline_access(desk_owners.airline,vuser_id)<>0 AND
              rownum<2;
        RETURN 1;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN NULL;
      END;
    END IF;
  END IF;
  RETURN 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    RETURN 1;
END check_desk_access;

FUNCTION check_desk_view_access(vdesk IN desks.code%TYPE,
                                vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_desk_access(vdesk,vuser_id,TRUE);
END check_desk_view_access;

FUNCTION check_desk_access(vdesk IN desks.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_desk_access(vdesk,vuser_id,FALSE);
END check_desk_access;

FUNCTION check_desk_access(vdesk IN desks.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN desks.code%TYPE
IS
vairline  airlines.code%TYPE;
vairp     airps.code%TYPE;
lparams   system.TLexemeParams;
BEGIN
  IF check_desk_access(vdesk,vuser_id,FALSE)=0 THEN
    IF vexception<>0 THEN
      lparams('desk'):=vdesk;
      IF vexception=1 THEN
        system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_DESK', lparams);
      ELSE
        system.raise_user_exception('MSG.ACCESS.NO_PERM_MODIFY_DESK', lparams);
      END IF;
    END IF;
  END IF;
  RETURN vdesk;
END check_desk_access;

FUNCTION check_desk_grp_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                               vuser_id  IN users2.user_id%TYPE,
                               view_only IN BOOLEAN) RETURN NUMBER
IS
vairline  airlines.code%TYPE;
vairp     airps.code%TYPE;
desk_tmp  desk_owners.desk%TYPE;
BEGIN
  SELECT airline,airp INTO vairline,vairp FROM desk_grp WHERE grp_id=vgrp_id;
  IF check_airline_access(vairline,vuser_id)<>0 AND
     check_airp_access(vairp,vuser_id)<>0 THEN
    RETURN 1;
  ELSE
    IF view_only AND check_airp_access(vairp,vuser_id)<>0 THEN
      -- �஢�ਬ desk_owners
      BEGIN
        SELECT desk_owners.desk INTO desk_tmp
        FROM desks,desk_owners
        WHERE desks.code=desk_owners.desk AND desks.grp_id=vgrp_id AND
              desk_owners.airline IS NOT NULL AND
              check_airline_access(desk_owners.airline,vuser_id)<>0 AND
              rownum<2;
        RETURN 1;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN NULL;
      END;
    END IF;
  END IF;
  RETURN 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    RETURN 1;
END check_desk_grp_access;

FUNCTION check_desk_grp_view_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                                    vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_desk_grp_access(vgrp_id,vuser_id,TRUE);
END check_desk_grp_view_access;

FUNCTION check_desk_grp_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                               vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
BEGIN
  RETURN check_desk_grp_access(vgrp_id,vuser_id,FALSE);
END check_desk_grp_access;

FUNCTION check_desk_grp_access(vgrp_id   IN desk_grp.grp_id%TYPE,
                               vuser_id  IN users2.user_id%TYPE,
                               vexception IN NUMBER) RETURN desk_grp.grp_id%TYPE
IS
BEGIN
  IF check_desk_grp_access(vgrp_id,vuser_id,FALSE)=0 THEN
    IF vexception<>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_DESK_GRP');
      ELSE
        system.raise_user_exception('MSG.ACCESS.NO_PERM_MODIFY_DESK_GRP');
      END IF;
    END IF;
  END IF;
  RETURN vgrp_id;
END check_desk_grp_access;

FUNCTION check_airline_access(vairline      IN airlines.code%TYPE,
                              vairline_view IN VARCHAR2,
                              vuser_id      IN users2.user_id%TYPE,
                              vexception    IN NUMBER) RETURN airlines.code%TYPE
IS
lparams   system.TLexemeParams;
BEGIN
  IF check_airline_access(vairline,vuser_id)=0 THEN
    IF vexception<>0 THEN
      IF vairline IS NOT NULL THEN
        lparams('airline'):=vairline_view;
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NO_PERM_ENTER_AIRLINE', lparams);
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_AIRLINE', lparams);
        END IF;
      ELSE
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NEED_SET_CODE_AIRLINE');
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRLINE');
        END IF;
      END IF;
    END IF;
  END IF;
  RETURN vairline;
END check_airline_access;

PROCEDURE check_airline_access(vairline1      IN airlines.code%TYPE,
                               vairline1_view IN VARCHAR2,
                               vairline2      IN airlines.code%TYPE,
                               vairline2_view IN VARCHAR2,
                               vuser_id       IN users2.user_id%TYPE,
                               vexception     IN NUMBER)
IS
vairline      airlines.code%TYPE;
lparams system.TLexemeParams;
BEGIN
  IF vairline1 IS NULL AND vairline2 IS NULL OR
     vairline1 IS NOT NULL AND vairline2 IS NOT NULL AND vairline1=vairline2 THEN
    vairline:=check_airline_access(vairline1,vairline1_view,vuser_id,vexception);
  ELSE
    IF check_airline_access(vairline1,vuser_id)=0 AND
       check_airline_access(vairline2,vuser_id)=0 THEN
      IF vexception<>0 THEN
        IF vairline1 IS NOT NULL AND vairline2 IS NOT NULL THEN
          lparams('airline1'):=vairline1_view;
          lparams('airline2'):=vairline2_view;
          IF vexception=1 THEN
            system.raise_user_exception('MSG.NO_PERM_ENTER_AL_AND_AL', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AL_AND_AL', lparams);
          END IF;
        ELSE
          IF vairline1 IS NOT NULL THEN
            lparams('airline'):=vairline1_view;
          ELSE
            lparams('airline'):=vairline2_view;
          END IF;
          IF vexception=1 THEN
            system.raise_user_exception('MSG.NO_PERM_ENTER_AL_AND_INDEFINITE_AL', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AL_AND_INDEFINITE_AL', lparams);
          END IF;
        END IF;
      END IF;
    END IF;
  END IF;
END check_airline_access;

FUNCTION check_airline_access(vairline  IN airlines.code%TYPE,
                              vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
  CURSOR cur IS
    SELECT airline FROM aro_airlines WHERE aro_id=vuser_id;
curRow          cur%ROWTYPE;
vuser_type      user_types.code%TYPE;
res             NUMBER(1);
BEGIN
  res:=0;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    IF vairline IS NOT NULL THEN
      WHILE cur%FOUND LOOP
        EXIT WHEN curRow.airline=vairline;
        FETCH cur INTO curRow;
      END LOOP;
      IF cur%FOUND THEN res:=1; END IF;
    END IF;
  ELSE
    BEGIN
      SELECT type INTO vuser_type FROM users2 WHERE user_id=vuser_id;
      IF vuser_type IN (0,1) THEN res:=1; END IF;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN NULL;
    END;
  END IF;
  CLOSE cur;
  RETURN res;
END check_airline_access;

FUNCTION check_city_access(vcity      IN cities.code%TYPE,
                           vcity_view IN VARCHAR2,
                           vuser_id   IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN cities.code%TYPE
IS
lparams system.TLexemeParams;
BEGIN
  IF check_city_access(vcity,vuser_id)=0 THEN
    IF vexception<>0 THEN
      IF vcity IS NOT NULL THEN
        lparams('city'):=vcity_view;
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NO_PERM_ENTER_CITY', lparams);
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_CITY', lparams);
        END IF;
      ELSE
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NEED_SET_CODE_CITY');
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_CITY');
        END IF;
      END IF;
    END IF;
  END IF;
  RETURN vcity;
END check_city_access;

FUNCTION check_city_access(vcity  IN cities.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
  CURSOR cur IS
    SELECT DISTINCT city FROM aro_airps,airps
    WHERE aro_airps.airp=airps.code AND aro_id=vuser_id;
curRow          cur%ROWTYPE;
vuser_type      user_types.code%TYPE;
res             NUMBER(1);
BEGIN
  res:=0;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    IF vcity IS NOT NULL THEN
      WHILE cur%FOUND LOOP
        EXIT WHEN curRow.city=vcity;
        FETCH cur INTO curRow;
      END LOOP;
      IF cur%FOUND THEN res:=1; END IF;
    END IF;
  ELSE
    BEGIN
      SELECT type INTO vuser_type FROM users2 WHERE user_id=vuser_id;
      IF vuser_type IN (0,2) THEN res:=1; END IF;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN NULL;
    END;
  END IF;
  CLOSE cur;
  RETURN res;
END check_city_access;

FUNCTION check_airp_access(vairp      IN airps.code%TYPE,
                           vairp_view IN VARCHAR2,
                           vuser_id   IN users2.user_id%TYPE,
                           vexception IN NUMBER) RETURN airps.code%TYPE
IS
lparams system.TLexemeParams;
BEGIN
  IF check_airp_access(vairp,vuser_id)=0 THEN
    IF vexception<>0 THEN
      IF vairp IS NOT NULL THEN
        lparams('airp'):=vairp_view;
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NO_PERM_ENTER_AIRPORT', lparams);
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_AIRPORT', lparams);
        END IF;
      ELSE
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NEED_SET_CODE_AIRP');
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRPORT');
        END IF;
      END IF;
    END IF;
  END IF;
  RETURN vairp;
END check_airp_access;

PROCEDURE check_airp_access(vairp1      IN airps.code%TYPE,
                            vairp1_view IN VARCHAR2,
                            vairp2      IN airps.code%TYPE,
                            vairp2_view IN VARCHAR2,
                            vuser_id  IN users2.user_id%TYPE,
                            vexception IN NUMBER)
IS
vairp airps.code%TYPE;
lparams system.TLexemeParams;
BEGIN
  IF vairp1 IS NULL AND vairp2 IS NULL OR
     vairp1 IS NOT NULL AND vairp2 IS NOT NULL AND vairp1=vairp2 THEN
    vairp:=check_airp_access(vairp1,vairp1_view,vuser_id,vexception);
  ELSE
    IF check_airp_access(vairp1,vuser_id)=0 AND
       check_airp_access(vairp2,vuser_id)=0 THEN
      IF vexception<>0 THEN
        IF vairp1 IS NOT NULL AND vairp2 IS NOT NULL THEN
          lparams('airp1'):=vairp1_view;
          lparams('airp2'):=vairp2_view;
          IF vexception=1 THEN
            system.raise_user_exception('MSG.NO_PERM_ENTER_AP_AND_AP', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AP_AND_AP', lparams);
          END IF;
        ELSE
          IF vairp1 IS NOT NULL THEN
            lparams('airp'):=vairp1_view;
          ELSE
            lparams('airp'):=vairp2_view;
          END IF;
          IF vexception=1 THEN
            system.raise_user_exception('MSG.NO_PERM_ENTER_AP_AND_INDEFINITE_AP', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AP_AND_INDEFINITE_AP', lparams);
          END IF;
        END IF;
      END IF;
    END IF;
  END IF;
END check_airp_access;

FUNCTION check_airp_access(vairp  IN airps.code%TYPE,
                           vuser_id  IN users2.user_id%TYPE) RETURN NUMBER
IS
  CURSOR cur IS
    SELECT airp FROM aro_airps WHERE aro_id=vuser_id;
curRow          cur%ROWTYPE;
vuser_type      user_types.code%TYPE;
res             NUMBER(1);
BEGIN
  res:=0;
  OPEN cur;
  FETCH cur INTO curRow;
  IF cur%FOUND THEN
    IF vairp IS NOT NULL THEN
      WHILE cur%FOUND LOOP
        EXIT WHEN curRow.airp=vairp;
        FETCH cur INTO curRow;
      END LOOP;
      IF cur%FOUND THEN res:=1; END IF;
    END IF;
  ELSE
    BEGIN
      SELECT type INTO vuser_type FROM users2 WHERE user_id=vuser_id;
      IF vuser_type IN (0,2) THEN res:=1; END IF;
    EXCEPTION
      WHEN NO_DATA_FOUND THEN NULL;
    END;
  END IF;
  CLOSE cur;
  RETURN res;
END check_airp_access;

FUNCTION check_desk_code(vcode IN desks.code%TYPE) RETURN desks.code%TYPE
IS
BEGIN
  IF vcode IS NOT NULL THEN
    IF LENGTH(vcode)<>6 THEN
      system.raise_user_exception('MSG.INVALID_DESK_LENGTH');
    END IF;
    IF system.is_upp_let_dig(vcode)=0 THEN
      system.raise_user_exception('MSG.INVALID_DESK_CHARS');
    END IF;
  END IF;
  RETURN vcode;
END check_desk_code;

FUNCTION check_desk_grp_id(vdesk     IN desks.code%TYPE,
                           vdesk_grp IN desk_grp.grp_id%TYPE) RETURN desk_grp.grp_id%TYPE
IS
vdesk_grp2 desk_grp.grp_id%TYPE;
BEGIN
  IF vdesk IS NULL THEN
    IF vdesk_grp IS NULL THEN
      system.raise_user_exception('MSG.NEED_SET_DESK_OR_DESK_GRP');
    END IF;
    RETURN vdesk_grp;
  END IF;

  SELECT grp_id INTO vdesk_grp2 FROM desks WHERE code=vdesk;

  IF vdesk_grp IS NOT NULL AND vdesk_grp<>vdesk_grp2 THEN
    system.raise_user_exception('MSG.DESK_GRP_DOES_NOT_MEET_DESK');
  END IF;

  RETURN vdesk_grp2;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    system.raise_user_exception('MSG.INVALID_DESK');
END check_desk_grp_id;

PROCEDURE check_typeb_addrs(vtlg_type      IN typeb_addrs.tlg_type%TYPE,
                            vairline       IN typeb_addrs.airline%TYPE,
                            vairline_view  IN VARCHAR2,
                            vairp_dep      IN typeb_addrs.airp_dep%TYPE,
                            vairp_dep_view IN VARCHAR2,
                            vairp_arv      IN typeb_addrs.airp_arv%TYPE,
                            vairp_arv_view IN VARCHAR2,
                            vuser_id       IN users2.user_id%TYPE,
                            vexception     IN NUMBER)
IS
row             typeb_types%ROWTYPE;
vairlineh       airlines.code%TYPE;
vairp           airps.code%TYPE;
vairp_view      VARCHAR2(10);
lparams system.TLexemeParams;
BEGIN
  SELECT pr_dep INTO row.pr_dep FROM typeb_types WHERE code=vtlg_type;
  vairlineh:=check_airline_access(vairline,vairline_view,vuser_id,vexception);
  IF row.pr_dep IS NULL THEN
    IF check_airp_access(vairp_dep,vuser_id)=0 AND
       check_airp_access(vairp_arv,vuser_id)=0 THEN
      IF vairp_dep IS NOT NULL OR
         vairp_arv IS NOT NULL THEN
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NO_PERM_ENTER_AIRP_DEP_AND_ARV');
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_AIRP_DEP_AND_ARV');
        END IF;
      ELSE
        IF vexception=1 THEN
          system.raise_user_exception('MSG.NEED_SET_CODE_AIRP_DEP_OR_ARV');
        ELSE
          system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRP_DEP_AND_ARV');
        END IF;
      END IF;
    END IF;
  ELSE
    IF row.pr_dep<>0 THEN
      vairp:=vairp_dep;
      vairp_view:=vairp_dep_view;
    ELSE
      vairp:=vairp_arv;
      vairp_view:=vairp_arv_view;
    END IF;
    IF check_airp_access(vairp,vuser_id)=0 THEN
      IF vairp IS NOT NULL THEN
        lparams('airp'):=vairp_view;
        IF vexception=1 THEN
          IF row.pr_dep<>0 THEN
            system.raise_user_exception('MSG.NO_PERM_ENTER_AIRP_DEP', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_ENTER_AIRP_ARV', lparams);
          END IF;
        ELSE
          IF row.pr_dep<>0 THEN
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AIRP_DEP', lparams);
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_AIRP_ARV', lparams);
          END IF;
        END IF;
      ELSE
        IF vexception=1 THEN
          IF row.pr_dep<>0 THEN
            system.raise_user_exception('MSG.NEED_SET_CODE_AIRP_DEP');
          ELSE
            system.raise_user_exception('MSG.NEED_SET_CODE_AIRP_ARV');
          END IF;
        ELSE
          IF row.pr_dep<>0 THEN
            system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRP_DEP');
          ELSE
            system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRP_ARV');
          END IF;
        END IF;
      END IF;
    END IF;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    system.raise_user_exception('MSG.TLG.TYPE_WRONG_SPECIFIED');
END check_typeb_addrs;

FUNCTION check_misc_set_access(vtype IN misc_set.type%TYPE,
                               vuser_id IN users2.user_id%TYPE) RETURN NUMBER
IS
n       INTEGER;
vright_id rights_list.ida%TYPE;
BEGIN
  vright_id:=NULL;
  IF vtype IN (11,12,13,15) THEN vright_id:=0761; END IF; /*���樠��� ����ன�� (�᪫��� �����): ��ᬮ��, ����, ���������*/
  IF vtype IN (22)          THEN vright_id:=0577; END IF; /*����������⢨� ��⥬: ����ன��: ��ᬮ��, ����, ���������*/
  IF vtype IN (24,25)       THEN vright_id:=0252; END IF; /*�����⮢�� ॣ����樨: ����஫� APIS: ����, ���������*/
  IF vright_id IS NOT NULL THEN
    SELECT COUNT(*) INTO n
    FROM user_roles,role_rights
    WHERE user_roles.role_id=role_rights.role_id AND
          user_roles.user_id=vuser_id AND role_rights.right_id=vright_id AND rownum<2;
    IF n=0 THEN RETURN 0; END IF;
  END IF;
  RETURN 1;
END check_misc_set_access;

FUNCTION check_misc_set_access(vtype IN misc_set.type%TYPE,
                               vuser_id IN users2.user_id%TYPE,
                               vexception IN NUMBER) RETURN misc_set.type%TYPE
IS
BEGIN
  IF check_misc_set_access(vtype,vuser_id)=0 THEN
    IF vexception<>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.NO_PERM_ENTER_SYSTEM_SETTING');
      ELSE
        system.raise_user_exception('MSG.NO_PERM_MODIFY_SYSTEM_SETTING');
      END IF;
    END IF;
  END IF;
  RETURN vtype;
END check_misc_set_access;

PROCEDURE check_airline_codes(vid               IN airlines.id%TYPE,
                              vcode             IN airlines.code%TYPE,
                              vcode_lat         IN airlines.code_lat%TYPE,
                              vcode_icao        IN airlines.code_icao%TYPE,
                              vcode_icao_lat    IN airlines.code_icao_lat%TYPE,
                              vaircode          IN airlines.aircode%TYPE,
                              vlang             IN lang_types.code%TYPE)
IS
  info	 adm.TCacheInfo;
  vfield_code  VARCHAR2(10);
  vfield_title cache_fields.title%TYPE;
  CURSOR cur(lid      airlines.id%TYPE,
             lcode    vfield_code%TYPE) IS
    SELECT code,'CODE' AS title FROM airlines
    WHERE code=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_LAT' AS title FROM airlines
    WHERE code_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ICAO' AS title FROM airlines
    WHERE code_icao=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ICAO_LAT' AS title FROM airlines
    WHERE code_icao_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1;
  CURSOR cur2(lid      airlines.id%TYPE,
              lcode    airlines.aircode%TYPE) IS
    SELECT code,'AIRCODE' AS title FROM airlines
    WHERE aircode=lcode AND id<>lid AND pr_del=0 AND rownum=1;
lparams system.TLexemeParams;
BEGIN
  info:=adm.get_cache_info('AIRLINES');
  FOR i IN 1..5 LOOP
    vfield_code:=
           CASE i
             WHEN 1 THEN vcode
             WHEN 2 THEN vcode_lat
             WHEN 3 THEN vcode_icao
             WHEN 4 THEN vcode_icao_lat
             WHEN 5 THEN vaircode
           END;
    vfield_title:=
           CASE i
             WHEN 1 THEN 'CODE'
             WHEN 2 THEN 'CODE_LAT'
             WHEN 3 THEN 'CODE_ICAO'
             WHEN 4 THEN 'CODE_ICAO_LAT'
             WHEN 5 THEN 'AIRCODE'
           END;
    IF vfield_code IS NOT NULL THEN
      IF i BETWEEN 1 AND 4 THEN
        FOR curRow IN cur(vid,vfield_code) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('airline'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_AIRLINE', lparams);
        END LOOP;
      ELSE
        FOR curRow IN cur2(vid,vfield_code) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('airline'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_AIRLINE', lparams);
        END LOOP;
      END IF;
    END IF;
  END LOOP;
END check_airline_codes;

PROCEDURE check_country_codes(vid               IN countries.id%TYPE,
                              vcode             IN countries.code%TYPE,
                              vcode_lat         IN countries.code_lat%TYPE,
                              vcode_iso         IN countries.code_iso%TYPE,
                              vlang             IN lang_types.code%TYPE)
IS
  info	 adm.TCacheInfo;
  vfield_code  VARCHAR2(10);
  vfield_title cache_fields.title%TYPE;
  CURSOR cur(lid      countries.id%TYPE,
             lcode    vfield_code%TYPE) IS
    SELECT code,'CODE' AS title FROM countries
    WHERE code=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_LAT' AS title FROM countries
    WHERE code_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ISO' AS title FROM countries
    WHERE code_iso=lcode AND id<>lid AND pr_del=0 AND rownum=1;
  lparams system.TLexemeParams;
BEGIN
  info:=adm.get_cache_info('COUNTRIES');
  FOR i IN 1..3 LOOP
    vfield_code:=
           CASE i
             WHEN 1 THEN vcode
             WHEN 2 THEN vcode_lat
             WHEN 3 THEN vcode_iso
           END;
    vfield_title:=
           CASE i
             WHEN 1 THEN 'CODE'
             WHEN 2 THEN 'CODE_LAT'
             WHEN 3 THEN 'CODE_ISO'
           END;
    IF vfield_code IS NOT NULL THEN
      IF i BETWEEN 1 AND 3 THEN
        FOR curRow IN cur(vid,vfield_code) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('country'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_COUNTRY', lparams);
        END LOOP;
      END IF;
    END IF;
  END LOOP;
END check_country_codes;

PROCEDURE check_city_codes(vid               IN cities.id%TYPE,
                           vcode             IN cities.code%TYPE,
                           vcode_lat         IN cities.code_lat%TYPE,
                           vlang             IN lang_types.code%TYPE)
IS
  info	 adm.TCacheInfo;
  vfield_code  VARCHAR2(10);
  vfield_title cache_fields.title%TYPE;
  CURSOR cur(lid      cities.id%TYPE,
             lcode    vfield_code%TYPE) IS
    SELECT code,'CODE' AS title FROM cities
    WHERE code=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_LAT' AS title FROM cities
    WHERE code_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1;
  lparams system.TLexemeParams;
BEGIN
  info:=adm.get_cache_info('CITIES');
  FOR i IN 1..2 LOOP
    vfield_code:=
           CASE i
             WHEN 1 THEN vcode
             WHEN 2 THEN vcode_lat
           END;
    vfield_title:=
           CASE i
             WHEN 1 THEN 'CODE'
             WHEN 2 THEN 'CODE_LAT'
           END;
    IF vfield_code IS NOT NULL THEN
      IF i BETWEEN 1 AND 2 THEN
        FOR curRow IN cur(vid,vfield_code) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('city'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_CITY', lparams);
        END LOOP;
      END IF;
    END IF;
  END LOOP;
END check_city_codes;

PROCEDURE check_airp_codes(vid               IN airps.id%TYPE,
                           vcode             IN airps.code%TYPE,
                           vcode_lat         IN airps.code_lat%TYPE,
                           vcode_icao        IN airps.code_icao%TYPE,
                           vcode_icao_lat    IN airps.code_icao_lat%TYPE,
                           vlang             IN lang_types.code%TYPE)
IS
  info	 adm.TCacheInfo;
  vfield_code  VARCHAR2(10);
  vfield_title cache_fields.title%TYPE;
  CURSOR cur(lid      airps.id%TYPE,
             lcode    vfield_code%TYPE) IS
    SELECT code,'CODE' AS title FROM airps
    WHERE code=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_LAT' AS title FROM airps
    WHERE code_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ICAO' AS title FROM airps
    WHERE code_icao=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ICAO_LAT' AS title FROM airps
    WHERE code_icao_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1;
  lparams system.TLexemeParams;
BEGIN
  info:=adm.get_cache_info('AIRPS');
  FOR i IN 1..4 LOOP
    vfield_code:=
           CASE i
             WHEN 1 THEN vcode
             WHEN 2 THEN vcode_lat
             WHEN 3 THEN vcode_icao
             WHEN 4 THEN vcode_icao_lat
           END;
    vfield_title:=
           CASE i
             WHEN 1 THEN 'CODE'
             WHEN 2 THEN 'CODE_LAT'
             WHEN 3 THEN 'CODE_ICAO'
             WHEN 4 THEN 'CODE_ICAO_LAT'
           END;
    IF vfield_code IS NOT NULL THEN
      IF i BETWEEN 1 AND 4 THEN
        FOR curRow IN cur(vid,vfield_code) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('airp'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_AIRPORT', lparams);
        END LOOP;
      END IF;
    END IF;
  END LOOP;
END check_airp_codes;

PROCEDURE check_craft_codes(vid               IN crafts.id%TYPE,
                            vcode             IN crafts.code%TYPE,
                            vcode_lat         IN crafts.code_lat%TYPE,
                            vcode_icao        IN crafts.code_icao%TYPE,
                            vcode_icao_lat    IN crafts.code_icao_lat%TYPE,
                            vlang             IN lang_types.code%TYPE)
IS
  info	 adm.TCacheInfo;
  vfield_code  VARCHAR2(10);
  vfield_title cache_fields.title%TYPE;
  CURSOR cur(lid      crafts.id%TYPE,
             lcode    vfield_code%TYPE,
             lcol     INTEGER) IS
    SELECT code,'CODE' AS title FROM crafts
    WHERE code=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_LAT' AS title FROM crafts
    WHERE code_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1
    UNION
    SELECT code,'CODE_ICAO' AS title FROM crafts
    WHERE code_icao=lcode AND id<>lid AND pr_del=0 AND rownum=1 AND lcol<>3
    UNION
    SELECT code,'CODE_ICAO_LAT' AS title FROM crafts
    WHERE code_icao_lat=lcode AND id<>lid AND pr_del=0 AND rownum=1 AND lcol<>4;
  lparams system.TLexemeParams;
BEGIN
  info:=adm.get_cache_info('CRAFTS');
  FOR i IN 1..4 LOOP
    vfield_code:=
           CASE i
             WHEN 1 THEN vcode
             WHEN 2 THEN vcode_lat
             WHEN 3 THEN vcode_icao
             WHEN 4 THEN vcode_icao_lat
           END;
    vfield_title:=
           CASE i
             WHEN 1 THEN 'CODE'
             WHEN 2 THEN 'CODE_LAT'
             WHEN 3 THEN 'CODE_ICAO'
             WHEN 4 THEN 'CODE_ICAO_LAT'
           END;
    IF vfield_code IS NOT NULL THEN
      IF i BETWEEN 1 AND 4 THEN
        FOR curRow IN cur(vid,vfield_code,i) LOOP
          lparams('field1'):=info.field_title(vfield_title);
          lparams('value'):=vfield_code;
          lparams('field2'):=info.field_title(curRow.title);
          lparams('craft'):=curRow.code;
          system.raise_user_exception('MSG.CODE_ALREADY_USED_FOR_AIRCRAFT', lparams);
        END LOOP;
      END IF;
    END IF;
  END LOOP;
END check_craft_codes;

PROCEDURE insert_user(vlogin             IN users2.login%TYPE,
                      vdescr             IN users2.descr%TYPE,
                      vtype              IN users2.type%TYPE,
                      vpr_denial         IN users2.pr_denial%TYPE,
                      vsys_user_id       IN users2.user_id%TYPE,
                      vtime_fmt  	 IN user_sets.time%TYPE,
                      vdisp_airline_fmt  IN user_sets.disp_airline%TYPE,
                      vdisp_airp_fmt  	 IN user_sets.disp_airp%TYPE,
                      vdisp_craft_fmt  	 IN user_sets.disp_craft%TYPE,
                      vdisp_suffix_fmt   IN user_sets.disp_suffix%TYPE,
                      vsetting_user      IN history_events.open_user%TYPE,
                      vstation           IN history_events.open_desk%TYPE)
IS
vuser_id        users2.user_id%TYPE;
vid             NUMBER(9);
BEGIN
  BEGIN
    SELECT id__seq.nextval INTO vuser_id FROM dual;
    INSERT INTO users2(user_id,login,passwd,descr,type,pr_denial)
    VALUES(vuser_id,vlogin,vlogin,vdescr,vtype,vpr_denial);
    hist.synchronize_history('users2', vuser_id, vsetting_user, vstation);
  EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      system.raise_user_exception('MSG.ACCESS.USERNAME_ALREADY_IN_SYSTEM');
  END;
  SELECT id__seq.nextval INTO vid FROM dual;
  INSERT INTO aro_airlines(aro_id,airline,id)
  SELECT vuser_id,airline,id__seq.nextval FROM aro_airlines WHERE aro_id=vsys_user_id;
  hist.synchronize_history('aro_airlines', vid, vsetting_user, vstation);
  INSERT INTO aro_airps(aro_id,airp,id)
  SELECT vuser_id,airp,id__seq.nextval FROM aro_airps WHERE aro_id=vsys_user_id;
  hist.synchronize_history('aro_airps', vid, vsetting_user, vstation);
  IF check_user_access(vuser_id,vsys_user_id,FALSE)=0 THEN
    system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_USER_TYPE');
  END IF;

  IF NVL(vtime_fmt,01)=01 AND
     NVL(vdisp_airline_fmt,09)=09 AND
     NVL(vdisp_airp_fmt,09)=09 AND
     NVL(vdisp_craft_fmt,09)=09 AND
     NVL(vdisp_suffix_fmt,17)=17 THEN
    NULL;
  ELSE
    INSERT INTO user_sets(user_id,time,disp_airline,disp_airp,disp_craft,disp_suffix)
    VALUES(vuser_id,vtime_fmt,vdisp_airline_fmt,vdisp_airp_fmt,vdisp_craft_fmt,vdisp_suffix_fmt);
    hist.synchronize_history('user_sets', vuser_id, vsetting_user, vstation);
  END IF;
END insert_user;

PROCEDURE update_user(vold_user_id       IN users2.user_id%TYPE,
                      vlogin             IN users2.login%TYPE,
                      vtype              IN users2.type%TYPE,
                      vpr_denial         IN users2.pr_denial%TYPE,
                      vsys_user_id       IN users2.user_id%TYPE,
                      vtime_fmt  	 IN user_sets.time%TYPE,
                      vdisp_airline_fmt  IN user_sets.disp_airline%TYPE,
                      vdisp_airp_fmt  	 IN user_sets.disp_airp%TYPE,
                      vdisp_craft_fmt  	 IN user_sets.disp_craft%TYPE,
                      vdisp_suffix_fmt   IN user_sets.disp_suffix%TYPE,
                      vsetting_user      IN history_events.open_user%TYPE,
                      vstation           IN history_events.open_desk%TYPE)
IS
vuser_id        users2.user_id%TYPE;
BEGIN
  vuser_id:=check_user_access(vold_user_id,vsys_user_id,2);
  BEGIN
    UPDATE users2
    SET login=vlogin,type=vtype,pr_denial=vpr_denial
    WHERE user_id=vold_user_id;
    hist.synchronize_history('users2', vold_user_id, vsetting_user, vstation);
  EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      system.raise_user_exception('MSG.ACCESS.USERNAME_ALREADY_IN_SYSTEM');
  END;

  IF check_user_access(vold_user_id,vsys_user_id,FALSE)=0 THEN
    system.raise_user_exception('MSG.ACCESS.NO_PERM_ENTER_USER_TYPE');
  END IF;

  IF NVL(vtime_fmt,01)=01 AND
     NVL(vdisp_airline_fmt,09)=09 AND
     NVL(vdisp_airp_fmt,09)=09 AND
     NVL(vdisp_craft_fmt,09)=09 AND
     NVL(vdisp_suffix_fmt,17)=17 THEN
    DELETE FROM user_sets WHERE user_id=vold_user_id;
    hist.synchronize_history('user_sets', vold_user_id, vsetting_user, vstation);
  ELSE
    UPDATE user_sets
    SET time=vtime_fmt,
        disp_airline=vdisp_airline_fmt,
        disp_airp=vdisp_airp_fmt,
        disp_craft=vdisp_craft_fmt,
        disp_suffix=vdisp_suffix_fmt
    WHERE user_id=vold_user_id;
    IF SQL%NOTFOUND THEN
      INSERT INTO user_sets(user_id,time,disp_airline,disp_airp,disp_craft,disp_suffix)
      VALUES(vold_user_id,vtime_fmt,vdisp_airline_fmt,vdisp_airp_fmt,vdisp_craft_fmt,vdisp_suffix_fmt);
    END IF;
    hist.synchronize_history('user_sets', vold_user_id, vsetting_user, vstation);
  END IF;
END update_user;

PROCEDURE delete_user(vold_user_id  IN users2.user_id%TYPE,
                      vsys_user_id  IN users2.user_id%TYPE,
                      vsetting_user IN history_events.open_user%TYPE,
                      vstation      IN history_events.open_desk%TYPE)
IS
vuser_id          users2.user_id%TYPE;
vid               NUMBER(9);
TYPE TIdsTable IS TABLE OF NUMBER(9);
ids            TIdsTable;
i                 BINARY_INTEGER;
BEGIN
  vuser_id:=check_user_access(vold_user_id,vsys_user_id,2);
  DELETE FROM web_clients WHERE user_id=vold_user_id RETURNING id INTO vid;
  IF SQL%ROWCOUNT>0 THEN
    hist.synchronize_history('web_clients',vid,vsetting_user,vstation);
  END IF;
  DELETE FROM form_packs WHERE user_id=vold_user_id;
  DELETE FROM user_roles WHERE user_id=vold_user_id RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST
    LOOP
      hist.synchronize_history('user_roles',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;
  DELETE FROM user_sets WHERE user_id=vold_user_id;
  hist.synchronize_history('user_sets', vold_user_id, vsetting_user, vstation);
  UPDATE users2
  SET login=NULL,passwd=NULL,pr_denial=-1,desk=NULL
  WHERE user_id=vold_user_id;
  hist.synchronize_history('users2', vold_user_id, vsetting_user, vstation);
END delete_user;

FUNCTION get_cache_info(vcode	cache_tables.code%TYPE) RETURN TCacheInfo
IS
  CURSOR cur IS
    SELECT name,title FROM cache_fields WHERE code=vcode;
info TCacheInfo;
BEGIN
  SELECT title INTO info.title FROM cache_tables WHERE code=vcode;
  FOR curRow IN cur LOOP
    info.field_title(curRow.name):=NVL(curRow.title,curRow.name);
  END LOOP;
  RETURN info;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    system.raise_user_exception('Cache '||vcode||' not found');
END get_cache_info;

FUNCTION get_trfer_set_flts(vtrfer_set_id trfer_set_flts.trfer_set_id%TYPE,
                            vpr_onward    trfer_set_flts.pr_onward%TYPE) RETURN VARCHAR2
IS
CURSOR cur IS
  SELECT flt_no FROM trfer_set_flts
  WHERE trfer_set_id=vtrfer_set_id AND pr_onward=vpr_onward
  ORDER BY flt_no;
res VARCHAR2(2000);
BEGIN
  res:=NULL;
  FOR curRow IN cur LOOP
    IF res IS NOT NULL THEN res:=res||' '; END IF;

    IF LENGTH(curRow.flt_no)<3 THEN
      res:=res||LPAD(curRow.flt_no,3,'0');
    ELSE
      res:=res||curRow.flt_no;
    END IF;
  END LOOP;
  RETURN res;
END get_trfer_set_flts;

FUNCTION get_flt_list(str IN VARCHAR2,
                      list IN OUT TFltList) RETURN VARCHAR2
IS
i       BINARY_INTEGER;
pos     BINARY_INTEGER;
len     BINARY_INTEGER;
flts    VARCHAR2(2000);
flt     VARCHAR2(5);
flt_no  trfer_set_flts.flt_no%TYPE;
BEGIN
  list.delete;
  flts:=LTRIM(str);
  WHILE flts IS NOT NULL LOOP
    pos:=INSTR(flts,' ');
    IF pos>0 THEN len:=pos-1; ELSE len:=LENGTH(flts); END IF;
    IF len>5 THEN RETURN SUBSTR(flts,1,len); END IF;
    flt:=SUBSTR(flts,1,len);
    FOR i IN 1..LENGTH(flt) LOOP
      IF NOT(SUBSTR(flt,i,1) BETWEEN '0' AND '9') THEN RETURN flt; END IF;
    END LOOP;
    flt_no:=TO_NUMBER(flt);
    IF flt_no<=0 THEN RETURN flt; END IF;
    i:=0;
    WHILE i<list.count LOOP
      EXIT WHEN list(i)=flt_no;
      i:=i+1;
    END LOOP;
    IF i>=list.count THEN
      list(list.count):=flt_no;
    END IF;
    flts:=LTRIM(SUBSTR(flts,len+1));
  END LOOP;
  RETURN NULL;
END get_flt_list;

FUNCTION check_trfer_sets_interval(str         IN VARCHAR2,
                                   cache_table IN cache_tables.code%TYPE,
                                   cache_field IN cache_fields.name%TYPE,
                                   vlang       IN lang_types.code%TYPE) RETURN trfer_sets.min_interval%TYPE
IS
info	 adm.TCacheInfo;
lparams   system.TLexemeParams;
hours VARCHAR2(2);
mins  VARCHAR2(2);
i     BINARY_INTEGER;
h     NUMBER(2);
m     NUMBER(2);
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  i:=INSTR(str,'-');
  IF i=0 THEN RAISE VALUE_ERROR; END IF;
  hours:=TRIM(SUBSTR(str,1,i-1));
  mins:=TRIM(SUBSTR(str,i+1));
  IF hours IS NULL OR mins IS NULL OR
     system.is_dig(hours)=0 OR system.is_dig(mins)=0 THEN RAISE VALUE_ERROR; END IF;
  h:=TO_NUMBER(hours);
  m:=TO_NUMBER(mins);
  IF h<0 OR h>99 OR m<0 OR m>59 THEN RAISE VALUE_ERROR; END IF;
  RETURN h*60+m;
EXCEPTION
  WHEN VALUE_ERROR THEN
    info:=adm.get_cache_info(cache_table);
    lparams('fieldname'):=info.field_title(cache_field);
    system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
END check_trfer_sets_interval;

PROCEDURE check_range(vmin         IN NUMBER,
                      vmax         IN NUMBER,
                      vcache_table IN cache_tables.code%TYPE,
                      vcache_field IN cache_fields.name%TYPE,
                      vlang        IN lang_types.code%TYPE)
IS
info	        adm.TCacheInfo;
lparams         system.TLexemeParams;
BEGIN
   IF vmin IS NOT NULL AND
      vmax IS NOT NULL AND
      vmin>vmax THEN
    info:=adm.get_cache_info(vcache_table);
    lparams('fieldname'):=info.field_title(vcache_field);
    system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
  END IF;
END check_range;



PROCEDURE insert_trfer_sets(vid		      IN trfer_sets.id%TYPE,
                            vairline_in       IN trfer_sets.airline_in%TYPE,
                            vairline_in_view  IN VARCHAR2,
                            vflt_no_in 	      IN VARCHAR2,
                            vairp 	      IN trfer_sets.airp%TYPE,
                            vairp_view        IN VARCHAR2,
                            vairline_out      IN trfer_sets.airline_out%TYPE,
                            vairline_out_view IN VARCHAR2,
                            vflt_no_out       IN VARCHAR2,
                            vtrfer_permit     IN trfer_sets.trfer_permit%TYPE,
                            vtrfer_outboard   IN trfer_sets.trfer_outboard%TYPE,
                            vtckin_permit     IN trfer_sets.tckin_permit%TYPE,
                            vtckin_waitlist   IN trfer_sets.tckin_waitlist%TYPE,
                            vtckin_norec      IN trfer_sets.tckin_norec%TYPE,
                            vmin_interval     IN VARCHAR2,
                            vmax_interval     IN VARCHAR2,
                            vsys_user_id      IN users2.user_id%TYPE,
                            vlang             IN lang_types.code%TYPE,
                            vsetting_user     IN history_events.open_user%TYPE,
                            vstation          IN history_events.open_desk%TYPE)
IS
vidh		trfer_sets.id%TYPE;
vairph          airps.code%TYPE;
flts_in		TFltList;
flts_out        TFltList;
err_elem        VARCHAR2(20);
i		BINARY_INTEGER;
k1		BINARY_INTEGER;
k2		BINARY_INTEGER;
flts_str        VARCHAR2(40);
TYPE trfer_sets_cur IS REF CURSOR;
cur             trfer_sets_cur;
cur_row         trfer_sets%ROWTYPE;
cur_id		trfer_sets.id%TYPE;
info	        adm.TCacheInfo;
lparams         system.TLexemeParams;
min_intervalh   trfer_sets.min_interval%TYPE;
max_intervalh   trfer_sets.max_interval%TYPE;
TYPE TIdsTable  IS TABLE OF NUMBER(9);
fltids          TIdsTable;
vflts_id        trfer_set_flts.id%TYPE;
BEGIN
  IF check_airline_access(vairline_in,vsys_user_id)=0 AND
     check_airline_access(vairline_out,vsys_user_id)=0 THEN
    lparams('airline1'):=vairline_in_view;
    lparams('airline2'):=vairline_out_view;
    system.raise_user_exception('MSG.NO_PERM_ENTER_AL_AND_AL', lparams);
  END IF;
  vairph:=check_airp_access(vairp,vairp_view,vsys_user_id,1);

  --�ਫ��
  err_elem:=SUBSTR(get_flt_list(vflt_no_in,flts_in),1,20);
  IF err_elem IS NOT NULL THEN
    lparams('flight'):=err_elem;
    system.raise_user_exception('MSG.INVALID_FLT_NO_ARV', lparams);
  END IF;

  --�뫥�
  err_elem:=SUBSTR(get_flt_list(vflt_no_out,flts_out),1,20);
  IF err_elem IS NOT NULL THEN
    lparams('flight'):=err_elem;
    system.raise_user_exception('MSG.INVALID_FLT_NO_DEP', lparams);
  END IF;

  min_intervalh:=check_trfer_sets_interval(vmin_interval,'TRFER_SETS','MIN_INTERVAL',vlang);
  max_intervalh:=check_trfer_sets_interval(vmax_interval,'TRFER_SETS','MAX_INTERVAL',vlang);
  check_range(min_intervalh, max_intervalh, 'TRFER_SETS', 'MAX_INTERVAL', vlang);

  --ᮡ�⢥��� ������ � ����
  IF vid IS NULL THEN
    SELECT id__seq.nextval INTO vidh FROM dual;
    INSERT INTO trfer_sets(id,airline_in,airp,airline_out,
      trfer_permit,trfer_outboard,tckin_permit,tckin_waitlist,tckin_norec,min_interval,max_interval)
    VALUES(vidh,vairline_in,vairp,vairline_out,
      vtrfer_permit,vtrfer_outboard,vtckin_permit,vtckin_waitlist,vtckin_norec,min_intervalh,max_intervalh);
    hist.synchronize_history('trfer_sets',vidh,vsetting_user,vstation);
  ELSE
    vidh:=vid;
    DELETE FROM trfer_set_flts WHERE trfer_set_id=vidh RETURNING id BULK COLLECT INTO fltids;
    IF SQL%ROWCOUNT>0 THEN
      FOR i IN fltids.FIRST..fltids.LAST
      LOOP
        hist.synchronize_history('trfer_set_flts',fltids(i),vsetting_user,vstation);
      END LOOP;
    END IF;
    UPDATE trfer_sets
    SET airline_in=vairline_in,
        airp=vairp,
        airline_out=vairline_out,
        trfer_permit=vtrfer_permit,
        trfer_outboard=vtrfer_outboard,
        tckin_permit=vtckin_permit,
        tckin_waitlist=vtckin_waitlist,
        tckin_norec=vtckin_norec,
        min_interval=min_intervalh,
        max_interval=max_intervalh
    WHERE id=vidh;
    hist.synchronize_history('trfer_sets',vidh,vsetting_user,vstation);
  END IF;
  FOR i IN 0..flts_in.count-1 LOOP
    INSERT INTO trfer_set_flts(trfer_set_id,pr_onward,flt_no,id)
    VALUES(vidh,0,flts_in(i),id__seq.nextval) RETURNING id INTO vflts_id;
    hist.synchronize_history('trfer_set_flts',vflts_id,vsetting_user,vstation);
  END LOOP;
  FOR i IN 0..flts_out.count-1 LOOP
    INSERT INTO trfer_set_flts(trfer_set_id,pr_onward,flt_no,id)
    VALUES(vidh,1,flts_out(i),id__seq.nextval) RETURNING id INTO vflts_id;
    hist.synchronize_history('trfer_set_flts',vflts_id,vsetting_user,vstation);
  END LOOP;

  BEGIN
    SELECT get_trfer_set_flts(vidh,0) INTO flts_str FROM dual;
  EXCEPTION
    WHEN VALUE_ERROR THEN
      system.raise_user_exception('MSG.TOO_MANY_ARRIVING_FLIGHTS');
  END;
  BEGIN
    SELECT get_trfer_set_flts(vidh,1) INTO flts_str FROM dual;
  EXCEPTION
    WHEN VALUE_ERROR THEN
      system.raise_user_exception('MSG.TOO_MANY_DEPARTING_FLIGHTS');
  END;

  OPEN cur FOR
    SELECT DISTINCT trfer_sets.id,
                    trfer_sets.trfer_permit,
                    trfer_sets.trfer_outboard,
                    trfer_sets.tckin_permit,
                    trfer_sets.tckin_waitlist,
                    trfer_sets.tckin_norec,
                    trfer_sets.min_interval,
                    trfer_sets.max_interval
    FROM trfer_sets,
         trfer_set_flts flts_in,trfer_set_flts flts_out,
         trfer_sets curr_trfer_sets,
         trfer_set_flts curr_flts_in,trfer_set_flts curr_flts_out
    WHERE trfer_sets.id=flts_in.trfer_set_id(+) AND flts_in.pr_onward(+)=0 AND
          trfer_sets.id=flts_out.trfer_set_id(+) AND flts_out.pr_onward(+)=1 AND
          trfer_sets.id<>vidh AND
          curr_trfer_sets.id=curr_trfer_sets.id AND
          curr_trfer_sets.id=curr_flts_in.trfer_set_id(+) AND curr_flts_in.pr_onward(+)=0 AND
          curr_trfer_sets.id=curr_flts_out.trfer_set_id(+) AND curr_flts_out.pr_onward(+)=1 AND
          curr_trfer_sets.id=vidh AND
          trfer_sets.airline_in=curr_trfer_sets.airline_in AND
          trfer_sets.airline_out=curr_trfer_sets.airline_out AND
          trfer_sets.airp=curr_trfer_sets.airp AND
          NVL(curr_flts_in.flt_no,-1)=NVL(flts_in.flt_no,-1) AND
          NVL(curr_flts_out.flt_no,-1)=NVL(flts_out.flt_no,-1);

  --�஠�������㥬 � 㡥६ �०�� �������� ᮢ�����騥 �����
  FETCH cur INTO cur_row.id,
                 cur_row.trfer_permit,
                 cur_row.trfer_outboard,
                 cur_row.tckin_permit,
                 cur_row.tckin_waitlist,
                 cur_row.tckin_norec,
                 cur_row.min_interval,
                 cur_row.max_interval;

  WHILE cur%FOUND LOOP
--  FOR curRow IN cur(vidh) LOOP

    cur_id:=cur_row.id;

    IF flts_in.count>0 THEN
      SELECT COUNT(*)
      INTO k1
      FROM
        (SELECT flt_no FROM trfer_set_flts WHERE trfer_set_id=cur_id AND pr_onward=0
         MINUS
         SELECT flt_no FROM trfer_set_flts WHERE trfer_set_id=vidh AND pr_onward=0);
    ELSE
      k1:=0;
    END IF;
    IF flts_out.count>0 THEN
      SELECT COUNT(*)
      INTO k2
      FROM
        (SELECT flt_no FROM trfer_set_flts WHERE trfer_set_id=cur_id AND pr_onward=1
         MINUS
         SELECT flt_no FROM trfer_set_flts WHERE trfer_set_id=vidh AND pr_onward=1);
    ELSE
      k2:=0;
    END IF;

    IF k1>0 THEN
      IF k2>0 THEN
        IF vtrfer_permit   <> cur_row.trfer_permit OR
           vtrfer_outboard <> cur_row.trfer_outboard OR
           vtckin_permit   <> cur_row.tckin_permit OR
           vtckin_waitlist <> cur_row.tckin_waitlist OR
           vtckin_norec    <> cur_row.tckin_norec OR
           NVL(min_intervalh,1E10) <> NVL(cur_row.min_interval,1E10) OR
           NVL(max_intervalh,1E10) <> NVL(cur_row.max_interval,1E10) THEN
          system.raise_user_exception('MSG.CONFLICT_WITH_EARLIER_SETTINGS');
        END IF;
      ELSE
        --���� �� trfer_set_flts � id=cur_id � ३�� �� �ਫ��, ����� � vidh
        FOR i IN 0..flts_in.count-1 LOOP
          DELETE FROM trfer_set_flts WHERE trfer_set_id=cur_id AND pr_onward=0 AND flt_no=flts_in(i) RETURNING id INTO vflts_id;
          IF SQL%ROWCOUNT>0 THEN
            hist.synchronize_history('trfer_set_flts',vflts_id,vsetting_user,vstation);
          END IF;
        END LOOP;
      END IF;
    ELSE
      IF k2>0 THEN
        --���� �� trfer_set_flts � id=cur_id � ३�� �� �뫥�, ����� � vidh
        FOR i IN 0..flts_out.count-1 LOOP
          DELETE FROM trfer_set_flts WHERE trfer_set_id=cur_id AND pr_onward=1 AND flt_no=flts_out(i) RETURNING id INTO vflts_id;
          IF SQL%ROWCOUNT>0 THEN
            hist.synchronize_history('trfer_set_flts',vflts_id,vsetting_user,vstation);
          END IF;
        END LOOP;
      ELSE
        delete_trfer_sets(cur_id,vsetting_user,vstation);
      END IF;
    END IF;

    FETCH cur INTO cur_row.id,
                   cur_row.trfer_permit,
                   cur_row.trfer_outboard,
                   cur_row.tckin_permit,
                   cur_row.tckin_waitlist,
                   cur_row.tckin_norec,
                   cur_row.min_interval,
                   cur_row.max_interval;
  END LOOP;
  CLOSE cur;
END insert_trfer_sets;

PROCEDURE delete_trfer_sets(vid		      IN trfer_sets.id%TYPE,
                            vsetting_user     IN history_events.open_user%TYPE,
                            vstation          IN history_events.open_desk%TYPE)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
fltids            TIdsTable;
i                 BINARY_INTEGER;
BEGIN
  DELETE FROM trfer_set_flts WHERE trfer_set_id=vid RETURNING id BULK COLLECT INTO fltids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN fltids.FIRST..fltids.LAST
    LOOP
      hist.synchronize_history('trfer_set_flts',fltids(i),vsetting_user,vstation);
    END LOOP;
  END IF;
  DELETE FROM trfer_sets WHERE id=vid;
  hist.synchronize_history('trfer_sets',vid,vsetting_user,vstation);
END delete_trfer_sets;

FUNCTION normalize_days(days IN OUT VARCHAR2) RETURN BOOLEAN
IS
i       BINARY_INTEGER;
j       BINARY_INTEGER;
vdays   VARCHAR2(7);
c       CHAR(1);
BEGIN
  IF days IS NULL THEN RETURN TRUE; END IF;
  vdays:='.......';
  FOR i IN 1..LENGTH(days) LOOP
    c:=SUBSTR(days,i,1);
    IF NOT(c BETWEEN '1' AND '7') AND (c<>'.') THEN RETURN FALSE; END IF;
    IF c BETWEEN '1' AND '7' THEN
      j:=TO_NUMBER(c);
      vdays:=SUBSTR(vdays,1,j-1)||c||SUBSTR(vdays,j+1,7);
    END IF;
  END LOOP;
  IF vdays='.......' THEN RETURN FALSE; END IF;
  IF vdays<>'1234567' THEN days:=vdays; ELSE days:=NULL; END IF;
  RETURN TRUE;
END normalize_days;

PROCEDURE subtract_days(days_from IN  VARCHAR2,
                        days_what IN  VARCHAR2,
                        days_res  IN OUT VARCHAR2)
IS
i               BINARY_INTEGER;
c               CHAR(1);
vdays_from      VARCHAR2(7);
vdays_what      VARCHAR2(7);
BEGIN
  days_res:=NULL;
  vdays_from:=days_from;
  vdays_what:=days_what;
  IF normalize_days(vdays_from) AND
     normalize_days(vdays_what) THEN
    days_res:=vdays_from;
    IF days_res IS NULL THEN days_res:='1234567'; END IF;
    FOR i IN 1..LENGTH(days_res) LOOP
      c:=SUBSTR(days_res,i,1);
      IF vdays_what IS NULL OR
         vdays_what IS NOT NULL AND INSTR(vdays_what,c)<>0 THEN
        days_res:=SUBSTR(days_res,1,i-1)||'.'||SUBSTR(days_res,i+1,7);
      END IF;
    END LOOP;
    IF days_res='.......' THEN days_res:=NULL; END IF;
  END IF;
END subtract_days;

PROCEDURE add_codeshare_set(
       vid       IN OUT codeshare_sets.id%TYPE,
       vairline_oper    codeshare_sets.airline_oper%TYPE,
       vflt_no_oper     codeshare_sets.flt_no_oper%TYPE,
       vsuffix_oper     codeshare_sets.suffix_oper%TYPE,
       vairp_dep        codeshare_sets.airp_dep%TYPE,
       vairline_mark    codeshare_sets.airline_mark%TYPE,
       vflt_no_mark     codeshare_sets.flt_no_mark%TYPE,
       vsuffix_mark     codeshare_sets.suffix_mark%TYPE,
       vpr_mark_norms   codeshare_sets.pr_mark_norms%TYPE,
       vpr_mark_bp      codeshare_sets.pr_mark_bp%TYPE,
       vpr_mark_rpt     codeshare_sets.pr_mark_rpt%TYPE,
       vdays            codeshare_sets.days%TYPE,
       vfirst_date      codeshare_sets.first_date%TYPE,
       vlast_date       codeshare_sets.last_date%TYPE,
       vnow             DATE,
       vtid             codeshare_sets.tid%TYPE,
       vpr_denial       NUMBER,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE)
IS
first   DATE;
last    DATE;
CURSOR cur IS
  SELECT id,
         pr_mark_norms,pr_mark_bp,pr_mark_rpt,
         days,first_date,last_date
  FROM codeshare_sets
  WHERE airline_oper=vairline_oper AND
        flt_no_oper=vflt_no_oper AND
        (suffix_oper IS NULL AND vsuffix_oper IS NULL OR suffix_oper=vsuffix_oper) AND
        airp_dep=vairp_dep AND
        airline_mark=vairline_mark AND
        flt_no_mark=vflt_no_mark AND
        (suffix_mark IS NULL AND vsuffix_mark IS NULL OR suffix_mark=vsuffix_mark) AND
        ( last_date IS NULL OR last_date>first) AND
        ( last IS NULL OR last>first_date) AND
        pr_del=0
  FOR UPDATE;
curRow  cur%ROWTYPE;
idh     codeshare_sets.id%TYPE;
tidh    codeshare_sets.tid%TYPE;
pr_opd  BOOLEAN;
vdaysh          VARCHAR2(7);
vdays_rest      VARCHAR2(7);
i	BINARY_INTEGER;
c	CHAR(1);
first2  DATE;
last2   DATE;
lparams system.TLexemeParams;
BEGIN
  IF vairline_oper=vairline_mark AND
     vflt_no_oper=vflt_no_mark AND
     (vsuffix_oper IS NULL AND vsuffix_mark IS NULL OR
      vsuffix_oper IS NOT NULL AND vsuffix_mark IS NOT NULL AND vsuffix_oper=vsuffix_mark) THEN
    system.raise_user_exception('MSG.OPER_AND_MARK_FLIGHTS_MUST_DIFFER');
  END IF;

  vdaysh:=vdays;
  IF NOT normalize_days(vdaysh) THEN
    lparams('days'):=vdays;
    system.raise_user_exception('MSG.INVALID_FLIGHT_DAYS', lparams);
  END IF;

  adm.check_period(vid IS NULL,vfirst_date,vlast_date,vnow,first,last,pr_opd);

  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;

  /* �஡㥬 ࠧ���� �� ��१�� */
  FOR curRow IN cur LOOP
    /* �஢�ਬ �� ���ᥪ������� ��� ������ */
    subtract_days(curRow.days,vdaysh,vdays_rest);
    idh:=curRow.id;
    IF (curRow.days IS NULL OR vdays_rest IS NULL OR
        curRow.days IS NOT NULL AND vdays_rest IS NOT NULL AND curRow.days<>vdays_rest) AND
       (vid IS NULL OR vid IS NOT NULL AND vid<>curRow.id) THEN

      IF curRow.first_date<first THEN
        /* ��१�� [first_date,first) */
        IF idh IS NOT NULL THEN
          UPDATE codeshare_sets SET first_date=curRow.first_date,last_date=first,tid=tidh WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',curRow.id,vsetting_user,vstation);
          idh:=NULL;
        ELSE
          SELECT id__seq.nextval INTO idh FROM dual;
          INSERT INTO codeshare_sets(id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid)
          SELECT idh,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,curRow.first_date,first,0,tidh
          FROM codeshare_sets WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',idh,vsetting_user,vstation);
          idh:=NULL;
        END IF;
        first2:=first;
      ELSE
        first2:=curRow.first_date;
      END IF;
      IF last IS NOT NULL AND
         (curRow.last_date IS NULL OR curRow.last_date>last) THEN
        /* ��१�� [last,last_date)  */
        IF idh IS NOT NULL THEN
          UPDATE codeshare_sets SET first_date=last,last_date=curRow.last_date,tid=tidh WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',curRow.id,vsetting_user,vstation);
          idh:=NULL;
        ELSE
          SELECT id__seq.nextval INTO idh FROM dual;
          INSERT INTO codeshare_sets(id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid)
          SELECT idh,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,last,curRow.last_date,0,tidh
          FROM codeshare_sets WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',idh,vsetting_user,vstation);
          idh:=NULL;
        END IF;
        last2:=last;
      ELSE
        last2:=curRow.last_date;
      END IF;

      IF idh IS NOT NULL THEN
        IF vdays_rest IS NOT NULL THEN /*��-� �� ���� ��⠫���*/
          UPDATE codeshare_sets
          SET first_date=first2,last_date=last2,days=vdays_rest,tid=tidh
          WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',curRow.id,vsetting_user,vstation);
        ELSE
          UPDATE codeshare_sets SET pr_del=1,tid=tidh WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',curRow.id,vsetting_user,vstation);
        END IF;
      ELSE
        IF vdays_rest IS NOT NULL THEN /*��-� �� ���� ��⠫���*/
          /* ��⠢�� ����� ��ப� */
          SELECT id__seq.nextval INTO idh FROM dual;
          INSERT INTO codeshare_sets(id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid)
          SELECT idh,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                     pr_mark_norms,pr_mark_bp,pr_mark_rpt,vdays_rest,first2,last2,0,tidh
          FROM codeshare_sets WHERE id=curRow.id;
          hist.synchronize_history('codeshare_sets',idh,vsetting_user,vstation);
          idh:=NULL;
        END IF;
      END IF;
    END IF;
  END LOOP;
  IF vid IS NULL THEN
    IF NOT pr_opd AND vpr_denial=0 THEN
      /*���� ��१�� [first,last) */
      SELECT id__seq.nextval INTO vid FROM dual;
      INSERT INTO codeshare_sets(id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,
                                 pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid)
      VALUES(vid,vairline_oper,vflt_no_oper,vsuffix_oper,vairp_dep,vairline_mark,vflt_no_mark,vsuffix_mark,
                                 vpr_mark_norms,vpr_mark_bp,vpr_mark_rpt,vdaysh,first,last,0,tidh);
      hist.synchronize_history('codeshare_sets',vid,vsetting_user,vstation);
    END IF;
  ELSE
    /* �� ।���஢���� �����⨬ ��ப� */
    UPDATE codeshare_sets SET last_date=last,tid=tidh WHERE id=vid;
    hist.synchronize_history('codeshare_sets',vid,vsetting_user,vstation);
  END IF;
END add_codeshare_set;

PROCEDURE modify_codeshare_set(
       vid              codeshare_sets.id%TYPE,
       vlast_date       codeshare_sets.last_date%TYPE,
       vnow             DATE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             codeshare_sets.tid%TYPE DEFAULT NULL)
IS
r codeshare_sets%ROWTYPE;
BEGIN
  SELECT id,airline_oper,flt_no_oper,airp_dep,airline_mark,flt_no_mark,
         pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date
  INTO   r.id,r.airline_oper,r.flt_no_oper,r.airp_dep,r.airline_mark,r.flt_no_mark,
         r.pr_mark_norms,r.pr_mark_bp,r.pr_mark_rpt,r.days,r.first_date
  FROM codeshare_sets WHERE id=vid AND pr_del=0 FOR UPDATE;
  add_codeshare_set(r.id,r.airline_oper,r.flt_no_oper,r.suffix_oper,r.airp_dep,r.airline_mark,r.flt_no_mark,r.suffix_mark,
                    r.pr_mark_norms,r.pr_mark_bp,r.pr_mark_rpt,r.days,r.first_date,vlast_date,vnow,vtid,0,vsetting_user,vstation);
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END modify_codeshare_set;

PROCEDURE delete_codeshare_set(
       vid              codeshare_sets.id%TYPE,
       vnow             DATE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             codeshare_sets.tid%TYPE DEFAULT NULL)
IS
vfirst_date     DATE;
vlast_date      DATE;
tidh    codeshare_sets.tid%TYPE;
BEGIN
  SELECT first_date,last_date INTO vfirst_date,vlast_date FROM codeshare_sets
  WHERE id=vid AND pr_del=0 FOR UPDATE;
  IF vtid IS NULL THEN SELECT tid__seq.nextval INTO tidh FROM dual; ELSE tidh:=vtid; END IF;
  IF vlast_date IS NULL OR vlast_date>vnow THEN
    IF vfirst_date<vnow THEN
      UPDATE codeshare_sets SET last_date=vnow,tid=tidh WHERE id=vid;
      hist.synchronize_history('codeshare_sets',vid,vsetting_user,vstation);
    ELSE
      UPDATE codeshare_sets SET pr_del=1,tid=tidh WHERE id=vid;
      hist.synchronize_history('codeshare_sets',vid,vsetting_user,vstation);
    END IF;
  ELSE
    /* ᯥ樠�쭮 �⮡� � ��� ������ ������������ ��ப� */
    UPDATE codeshare_sets SET tid=tidh WHERE id=vid;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END delete_codeshare_set;

FUNCTION check_date_wo_year(str         IN VARCHAR2,
                            cache_table IN cache_tables.code%TYPE,
                            cache_field IN cache_fields.name%TYPE,
                            vlang       IN lang_types.code%TYPE) RETURN DATE
IS
info	 adm.TCacheInfo;
lparams   system.TLexemeParams;
BEGIN
  RETURN TO_DATE(str||'.2000','DD.MM.YYYY');
EXCEPTION
  WHEN OTHERS THEN
    info:=adm.get_cache_info(cache_table);
    lparams('fieldname'):=info.field_title(cache_field);
    system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
END check_date_wo_year;

PROCEDURE check_sita_addr(str         IN VARCHAR2,
                          cache_table IN cache_tables.code%TYPE,
                          cache_field IN cache_fields.name%TYPE,
                          vlang       IN lang_types.code%TYPE)
IS
info	 adm.TCacheInfo;
lparams   system.TLexemeParams;
BEGIN
  IF LENGTH(str)<>7 OR system.is_upp_let_dig(str,1)=0 THEN
    info:=adm.get_cache_info(cache_table);
    lparams('fieldname'):=info.field_title(cache_field);
    system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
  END IF;
END check_sita_addr;

PROCEDURE check_canon_name(str         IN VARCHAR2,
                           cache_table IN cache_tables.code%TYPE,
                           cache_field IN cache_fields.name%TYPE,
                           vlang       IN lang_types.code%TYPE)
IS
info	 adm.TCacheInfo;
lparams   system.TLexemeParams;
BEGIN
  IF LENGTH(str)<>5 OR system.is_upp_let_dig(str,1)=0 THEN
    info:=adm.get_cache_info(cache_table);
    lparams('fieldname'):=info.field_title(cache_field);
    system.raise_user_exception('MSG.TABLE.INVALID_FIELD_VALUE', lparams);
  END IF;
END check_canon_name;

PROCEDURE check_pers_weights(vid            IN pers_weights.id%TYPE,
                             vairline       IN pers_weights.airline%TYPE,
                             vcraft         IN pers_weights.craft%TYPE,
                             vbort          IN OUT pers_weights.bort%TYPE,
                             vclass         IN pers_weights.class%TYPE,
                             vsubclass      IN pers_weights.subclass%TYPE,
                             vpr_summer     IN pers_weights.pr_summer%TYPE,
                             vfirst_date    IN VARCHAR2,
                             vlast_date     IN VARCHAR2,
                             first_date_out OUT pers_weights.first_date%TYPE,
                             last_date_out  OUT pers_weights.last_date%TYPE,
                             vlang          IN lang_types.code%TYPE)
IS
info	 adm.TCacheInfo;
lparams  system.TLexemeParams;
n        BINARY_INTEGER;
BEGIN
  vbort:=salons.check_bort(vbort);
  IF vpr_summer IS NULL AND vfirst_date IS NULL AND vlast_date IS NULL THEN
    info:=adm.get_cache_info('AIRLINE_PERS_WEIGHTS');
    lparams('fieldname'):=info.field_title('SEASON_NAME');
    system.raise_user_exception('MSG.TABLE.NOT_SET_FIELD_VALUE', lparams);
  END IF;
  IF vfirst_date IS NOT NULL THEN
    first_date_out:=check_date_wo_year(vfirst_date,'AIRLINE_PERS_WEIGHTS','FIRST_DATE',vlang);
    IF vpr_summer IS NOT NULL THEN
      system.raise_user_exception('MSG.TABLE.AT_SET_SEASON_DATE_RANGE_NOT_UNDERLINED');
    END IF;
  END IF;
  IF vlast_date IS NOT NULL THEN
    last_date_out:=check_date_wo_year(vlast_date,'AIRLINE_PERS_WEIGHTS','LAST_DATE',vlang);
    IF vpr_summer IS NOT NULL THEN
      system.raise_user_exception('MSG.TABLE.AT_SET_SEASON_DATE_RANGE_NOT_UNDERLINED');
    END IF;
  END IF;
  IF vfirst_date IS NULL AND vlast_date IS NOT NULL THEN
    info:=adm.get_cache_info('AIRLINE_PERS_WEIGHTS');
    lparams('fieldname'):=info.field_title('FIRST_DATE');
    system.raise_user_exception('MSG.TABLE.NOT_SET_FIELD_VALUE', lparams);
  END IF;
  IF vfirst_date IS NOT NULL AND vlast_date IS NULL THEN
    info:=adm.get_cache_info('AIRLINE_PERS_WEIGHTS');
    lparams('fieldname'):=info.field_title('LAST_DATE');
    system.raise_user_exception('MSG.TABLE.NOT_SET_FIELD_VALUE', lparams);
  END IF;
  IF vpr_summer IS NOT NULL THEN
    SELECT COUNT(*) INTO n
    FROM pers_weights
    WHERE (vid IS NULL OR id<>vid) AND
          (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
          (craft IS NULL AND vcraft IS NULL OR craft=vcraft) AND
          (bort IS NULL AND vbort IS NULL OR bort=vbort) AND
          (class IS NULL AND vclass IS NULL OR class=vclass) AND
          (subclass IS NULL AND vsubclass IS NULL OR subclass=vsubclass) AND
          pr_summer=vpr_summer AND
          rownum<2;
  ELSE
    SELECT COUNT(*) INTO n
    FROM pers_weights
    WHERE (vid IS NULL OR id<>vid) AND
          (airline IS NULL AND vairline IS NULL OR airline=vairline) AND
          (craft IS NULL AND vcraft IS NULL OR craft=vcraft) AND
          (bort IS NULL AND vbort IS NULL OR bort=vbort) AND
          (class IS NULL AND vclass IS NULL OR class=vclass) AND
          (subclass IS NULL AND vsubclass IS NULL OR subclass=vsubclass) AND
          (last_date_out>=first_date_out AND last_date>=first_date AND (last_date_out>=first_date AND first_date_out<=last_date) OR
           last_date_out>=first_date_out AND last_date<first_date AND (last_date_out>=first_date OR first_date_out<=last_date) OR
           last_date_out<first_date_out AND last_date>=first_date AND (last_date_out>=first_date OR first_date_out<=last_date) OR
           last_date_out<first_date_out AND last_date<first_date) AND
          rownum<2;
  END IF;
  IF n>0 THEN
    system.raise_user_exception('MSG.CONFLICT_WITH_EARLIER_SETTINGS');
  END IF;
END check_pers_weights;

PROCEDURE check_not_airp_user(vuser_id   IN users2.user_id%TYPE,
                              vexception IN NUMBER)
IS
n BINARY_INTEGER;
BEGIN
  IF vexception<>0 THEN
    SELECT COUNT(*) INTO n FROM users2 WHERE user_id=vuser_id AND type=utAirport;
    IF n>0 THEN
      IF vexception=1 THEN
        system.raise_user_exception('MSG.NO_PERM_ENTER_INDEFINITE_AIRPORT');
      ELSE
        system.raise_user_exception('MSG.NO_PERM_MODIFY_INDEFINITE_AIRPORT');
      END IF;
    END IF;
  END IF;
EXCEPTION
  WHEN NO_DATA_FOUND THEN NULL;
END check_not_airp_user;

PROCEDURE modify_rem_event_sets(old_set_id          rem_event_sets.set_id%TYPE,
                                old_airline         rem_event_sets.airline%TYPE,
                                old_rem_code        rem_event_sets.rem_code%TYPE,
                                vairline            rem_event_sets.airline%TYPE,
                                vrem_code           rem_event_sets.rem_code%TYPE,
                                bp                  rem_event_sets.event_value%TYPE,
                                alarm_ss            rem_event_sets.event_value%TYPE,
                                pnl_sel             rem_event_sets.event_value%TYPE,
                                brd_view            rem_event_sets.event_value%TYPE,
                                brd_warn            rem_event_sets.event_value%TYPE,
                                rpt_ss              rem_event_sets.event_value%TYPE,
                                rpt_pm              rem_event_sets.event_value%TYPE,
                                ckin_view           rem_event_sets.event_value%TYPE,
                                typeb_psm           rem_event_sets.event_value%TYPE,
                                typeb_pil           rem_event_sets.event_value%TYPE,
                                rem_stat            rem_event_sets.event_value%TYPE,
                                limited_capab_stat  rem_event_sets.event_value%TYPE,
                                self_ckin_exchange  rem_event_sets.event_value%TYPE,
                                web                 rem_event_sets.event_value%TYPE,
                                kiosk               rem_event_sets.event_value%TYPE,
                                mob                 rem_event_sets.event_value%TYPE,
                                free_seat           rem_event_sets.event_value%TYPE,
                                vsetting_user       history_events.open_user%TYPE,
                                vstation            history_events.open_desk%TYPE)
IS
i BINARY_INTEGER;
vevent_type  rem_event_sets.event_type%TYPE;
vevent_value rem_event_sets.event_value%TYPE;
vid          rem_event_sets.id%TYPE;
vset_id      rem_event_sets.set_id%TYPE;
BEGIN
  SELECT id__seq.nextval INTO vset_id FROM dual;
  FOR i IN 1..17 LOOP
    vevent_type:= CASE i
                    WHEN 1  THEN 'BP'
                    WHEN 2  THEN 'ALARM_SS'
                    WHEN 3  THEN 'PNL_SEL'
                    WHEN 4  THEN 'BRD_VIEW'
                    WHEN 5  THEN 'BRD_WARN'
                    WHEN 6  THEN 'RPT_SS'
                    WHEN 7  THEN 'RPT_PM'
                    WHEN 8  THEN 'CKIN_VIEW'
                    WHEN 9  THEN 'TYPEB_PSM'
                    WHEN 10 THEN 'TYPEB_PIL'
                    WHEN 11 THEN 'REM_STAT'
                    WHEN 12 THEN 'LIMITED_CAPAB_STAT'
                    WHEN 13 THEN 'SELF_CKIN_EXCHANGE'
                    WHEN 14 THEN 'WEB'
                    WHEN 15 THEN 'KIOSK'
                    WHEN 16 THEN 'MOB'
                    WHEN 17 THEN 'FREE_SEAT'
                  END;
    vevent_value:=CASE i
                    WHEN 1  THEN bp
                    WHEN 2  THEN alarm_ss
                    WHEN 3  THEN pnl_sel
                    WHEN 4  THEN brd_view
                    WHEN 5  THEN brd_warn
                    WHEN 6  THEN rpt_ss
                    WHEN 7  THEN rpt_pm
                    WHEN 8  THEN ckin_view
                    WHEN 9  THEN typeb_psm
                    WHEN 10 THEN typeb_pil
                    WHEN 11 THEN rem_stat
                    WHEN 12 THEN limited_capab_stat
                    WHEN 13 THEN self_ckin_exchange
                    WHEN 14 THEN web
                    WHEN 15 THEN kiosk
                    WHEN 16 THEN mob
                    WHEN 17 THEN free_seat
                  END;

    IF vevent_type IN ('SELF_CKIN_EXCHANGE', 'WEB', 'KIOSK', 'MOB') THEN
      BEGIN
        SELECT 0 INTO vevent_value FROM rem_cats WHERE rem_code=vrem_code AND category IN ('DOC','DOCO','DOCA','TKN');
      EXCEPTION
        WHEN NO_DATA_FOUND THEN NULL;
      END;
    END IF;

    IF old_set_id IS NOT NULL THEN
      IF vrem_code IS NULL THEN
        DELETE FROM rem_event_sets WHERE set_id=old_set_id AND event_type = vevent_type RETURNING id INTO vid;
      ELSE
        UPDATE rem_event_sets
        SET airline = vairline, rem_code = vrem_code, event_value = vevent_value
        WHERE set_id=old_set_id AND event_type = vevent_type RETURNING id INTO vid;
      END IF;
      IF SQL%ROWCOUNT>0 THEN
        hist.synchronize_history('rem_event_sets',vid,vsetting_user,vstation);
      END IF;
    ELSE
      INSERT INTO rem_event_sets(id, set_id, airline, rem_code, event_type, event_value)
      VALUES(id__seq.nextval, vset_id, vairline, vrem_code, vevent_type, vevent_value) RETURNING id INTO vid;
      hist.synchronize_history('rem_event_sets',vid,vsetting_user,vstation);
    END IF;
  END LOOP;
END modify_rem_event_sets;

PROCEDURE delete_create_points(vid            typeb_addrs.id%TYPE,
                               vsetting_user  history_events.open_user%TYPE,
                               vstation       history_events.open_desk%TYPE)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
ids               TIdsTable;
i                 BINARY_INTEGER;
BEGIN
  DELETE FROM typeb_create_points WHERE typeb_addrs_id=vid RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST LOOP
      hist.synchronize_history('typeb_create_points',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;
END delete_create_points;

FUNCTION get_typeb_option(vid         typeb_addrs.id%TYPE,
                          vbasic_type typeb_addr_options.tlg_type%TYPE,
                          vcategory   typeb_addr_options.category%TYPE) RETURN typeb_addr_options.value%TYPE
IS
result typeb_addr_options.value%TYPE;
BEGIN
  SELECT value INTO result FROM typeb_addr_options
  WHERE typeb_addrs_id=vid AND tlg_type=vbasic_type AND category=vcategory;
  RETURN result;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END get_typeb_option;

PROCEDURE delete_typeb_options(vid            typeb_addrs.id%TYPE,
                               vsetting_user  history_events.open_user%TYPE,
                               vstation       history_events.open_desk%TYPE)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
ids               TIdsTable;
i                 BINARY_INTEGER;
BEGIN
  DELETE FROM typeb_addr_options WHERE typeb_addrs_id=vid RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST LOOP
      hist.synchronize_history('typeb_addr_options',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;
END delete_typeb_options;

PROCEDURE sync_typeb_options(cur sync_typeb_options_cur,
                             vsetting_user  history_events.open_user%TYPE,
                             vstation       history_events.open_desk%TYPE)
IS
curRow typeb_addr_options%ROWTYPE;
BEGIN
  LOOP
    FETCH cur
    INTO curRow.typeb_addrs_id,
         curRow.tlg_type,
         curRow.category,
         curRow.id,
         curRow.value;
    EXIT WHEN not(cur%FOUND);
    IF curRow.id IS NOT NULL THEN
      IF curRow.category IS NULL THEN
        --㤠�塞 ��ப� �� dest
        DELETE FROM typeb_addr_options WHERE id=curRow.id;
      ELSE
        UPDATE typeb_addr_options
        SET value=curRow.value
        WHERE id=curRow.id AND ' '||value<>' '||curRow.value;
      END IF;
      IF SQL%ROWCOUNT>0 THEN
        hist.synchronize_history('typeb_addr_options',curRow.id,vsetting_user,vstation);
      END IF;
    ELSE
      --������塞 ��ப� � dest
      INSERT INTO typeb_addr_options(typeb_addrs_id, tlg_type, category, value, id)
      VALUES(curRow.typeb_addrs_id, curRow.tlg_type, curRow.category, curRow.value, id__seq.nextval);
      hist.synchronize_history('typeb_addr_options',id__seq.currval,vsetting_user,vstation);
    END IF;
  END LOOP;
END sync_typeb_options;

PROCEDURE sync_typeb_options(vid            typeb_addrs.id%TYPE,
                             vbasic_type    typeb_addr_options.tlg_type%TYPE,
                             vsetting_user  history_events.open_user%TYPE,
                             vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(dest.id, NULL, default_value, dest.value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_typeb_options;

PROCEDURE sync_COM_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'VERSION',       vversion,
                                                 default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_COM_options;

PROCEDURE sync_ETL_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vrbd           typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category,
                                'RBD',         vrbd,
                                                 default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_ETL_options;

PROCEDURE sync_MVT_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vnoend         typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category,
                                'VERSION',       vversion,
                                'NOEND',         vnoend,
                                                 default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_MVT_options;

PROCEDURE sync_LDM_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vcfg           typeb_addr_options.value%TYPE,
                           vcabin_baggage typeb_addr_options.value%TYPE,
                           vgender        typeb_addr_options.value%TYPE,
                           vexb           typeb_addr_options.value%TYPE,
                           vnoend         typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'VERSION',       vversion,
                                'CFG',           vcfg,
                                'CABIN_BAGGAGE', vcabin_baggage,
                                'GENDER',        vgender,
                                'EXB',           vexb,
                                'NOEND',         vnoend,
                                                 default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_LDM_options;

PROCEDURE sync_LCI_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vequipment     typeb_addr_options.value%TYPE,
                           vweignt_avail  typeb_addr_options.value%TYPE,
                           vseating       typeb_addr_options.value%TYPE,
                           vweight_mode   typeb_addr_options.value%TYPE,
                           vseat_restrict typeb_addr_options.value%TYPE,
                           vpas_totals    typeb_addr_options.value%TYPE,
                           vbag_totals    typeb_addr_options.value%TYPE,
                           vpas_distrib   typeb_addr_options.value%TYPE,
                           vseat_plan     typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'VERSION',      vversion,
                                'EQUIPMENT',    vequipment,
                                'WEIGHT_AVAIL', vweignt_avail,
                                'SEATING',      vseating,
                                'WEIGHT_MODE',  vweight_mode,
                                'SEAT_RESTRICT',vseat_restrict,
                                'PAS_TOTALS',   vpas_totals,
                                'BAG_TOTALS',   vbag_totals,
                                'PAS_DISTRIB',  vpas_distrib,
                                'SEAT_PLAN',    vseat_plan,
                                                default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_LCI_options;

PROCEDURE sync_PRL_options(vid            typeb_addrs.id%TYPE,
                           vbasic_type    typeb_addr_options.tlg_type%TYPE,
                           vversion       typeb_addr_options.value%TYPE,
                           vcreate_point  typeb_addr_options.value%TYPE,
                           vpax_state     typeb_addr_options.value%TYPE,
                           vrbd           typeb_addr_options.value%TYPE,
                           vxbag          typeb_addr_options.value%TYPE,
                           vsetting_user  history_events.open_user%TYPE,
                           vstation       history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'VERSION',      vversion,
                                'CREATE_POINT', vcreate_point,
                                'PAX_STATE',    vpax_state,
                                'RBD',          vrbd,
                                'XBAG',         vxbag,
                                                default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_PRL_options;

PROCEDURE sync_BSM_options(vid              typeb_addrs.id%TYPE,
                           vbasic_type      typeb_addr_options.tlg_type%TYPE,
                           vclass_of_travel typeb_addr_options.value%TYPE,
                           vtag_printer_id  typeb_addr_options.value%TYPE,
                           vpas_name_rp1745 typeb_addr_options.value%TYPE,
                           vactual_dep_date typeb_addr_options.value%TYPE,
                           vbrd              typeb_addr_options.value%TYPE,
                           vtrfer_in         typeb_addr_options.value%TYPE,
                           vlong_flt_no      typeb_addr_options.value%TYPE,
                           vsetting_user    history_events.open_user%TYPE,
                           vstation         history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'CLASS_OF_TRAVEL', vclass_of_travel,
                                'TAG_PRINTER_ID',  vtag_printer_id,
                                'PAS_NAME_RP1745', vpas_name_rp1745,
                                'ACTUAL_DEP_DATE', vactual_dep_date,
                                'BRD',             vbrd,
                                'TRFER_IN',        vtrfer_in,
                                'LONG_FLT_NO',     vlong_flt_no,
                                                   default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_BSM_options;

PROCEDURE sync_PNL_options(vid              typeb_addrs.id%TYPE,
                           vbasic_type      typeb_addr_options.tlg_type%TYPE,
                           vforwarding      typeb_addr_options.value%TYPE,
                           vsetting_user    history_events.open_user%TYPE,
                           vstation         history_events.open_desk%TYPE)
IS
cur sync_typeb_options_cur;
BEGIN
  --��稬 ����ன��
  UPDATE typeb_addrs SET id=id WHERE id=vid;
  OPEN cur FOR
    SELECT vid AS typeb_addrs_id, src.tlg_type, src.category, dest.id,
           DECODE(src.category, 'FORWARDING', vforwarding,
                                              default_value) AS value
    FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=vid) dest
         FULL OUTER JOIN
         (SELECT * FROM typeb_options WHERE tlg_type=vbasic_type) src
    ON (dest.tlg_type=src.tlg_type AND dest.category=src.category);
  sync_typeb_options(cur, vsetting_user, vstation);
  CLOSE cur;
END sync_PNL_options;

PROCEDURE modify_airline_offices(vid           airline_offices.id%TYPE,
                                 vairline      airline_offices.airline%TYPE,
                                 vcountry_control airline_offices.country_control%TYPE,
                                 vcontact_name airline_offices.contact_name%TYPE,
                                 vphone        airline_offices.phone%TYPE,
                                 vfax          airline_offices.fax%TYPE,
                                 vto_apis      airline_offices.to_apis%TYPE,
                                 vlang         lang_types.code%TYPE,
                                 vsetting_user history_events.open_user%TYPE,
                                 vstation      history_events.open_desk%TYPE)
IS
i BINARY_INTEGER;
vidh     airline_offices.id%TYPE;
BEGIN
  IF vto_apis<>0 THEN
    check_chars_in_name(vcontact_name, 1, ' -', 'AIRLINE_OFFICES', 'CONTACT_NAME', vlang);
    check_chars_in_name(vphone,        1, ' -', 'AIRLINE_OFFICES', 'PHONE',        vlang);
    check_chars_in_name(vfax,          1, ' -', 'AIRLINE_OFFICES', 'FAX',          vlang);
  END IF;
  IF vid IS NULL THEN
    INSERT INTO airline_offices(id, airline, country_control, contact_name, phone, fax, to_apis)
    VALUES(id__seq.nextval, vairline, vcountry_control, vcontact_name, vphone, vfax, vto_apis)
    RETURNING id INTO vidh;
    hist.synchronize_history('airline_offices',vid,vsetting_user,vstation);
  ELSE
    UPDATE airline_offices
    SET airline=vairline, country_control=vcountry_control, contact_name=vcontact_name,
        phone=vphone, fax=vfax, to_apis=vto_apis
    WHERE id=vid;
    hist.synchronize_history('airline_offices',vid,vsetting_user,vstation);
  END IF;
END modify_airline_offices;

PROCEDURE insert_roles(vname          roles.name%TYPE,
                       vairline       airlines.code%TYPE,
                       vairp          airps.code%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE)
IS
vrole_id roles.role_id%TYPE;
BEGIN
  SELECT id__seq.nextval INTO vrole_id FROM dual;
  INSERT INTO roles(role_id,name,airline,airp)
  VALUES(vrole_id,vname,vairline,vairp);
  hist.synchronize_history('roles',vrole_id,vsetting_user,vstation);
  INSERT INTO trip_list_days(id, role_id, shift_down, shift_up)
  SELECT vrole_id, vrole_id, -1, 1
  FROM dual
  WHERE lower(vname) like '����� ��%' OR
        lower(vname) like '�����%' OR
        lower(vname) like '��ᯮ��� ����஫�%' OR
        lower(vname) like '��ᬮ��%';
  hist.synchronize_history('trip_list_days',vrole_id,vsetting_user,vstation);
END insert_roles;

PROCEDURE modify_roles(vrole_id       roles.role_id%TYPE,
                       vname          roles.name%TYPE,
                       vairline       airlines.code%TYPE,
                       vairp          airps.code%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE)
IS
BEGIN
  DELETE FROM trip_list_days WHERE role_id=vrole_id;
  hist.synchronize_history('trip_list_days',vrole_id,vsetting_user,vstation);
  UPDATE roles SET name=vname,airline=vairline,airp=vairp
  WHERE role_id=vrole_id;
  hist.synchronize_history('roles',vrole_id,vsetting_user,vstation);
  INSERT INTO trip_list_days(id, role_id, shift_down, shift_up)
  SELECT vrole_id, vrole_id, -1, 1
  FROM dual
  WHERE lower(vname) like '����� ��%' OR
        lower(vname) like '�����%' OR
        lower(vname) like '��ᯮ��� ����஫�%' OR
        lower(vname) like '��ᬮ��%';
  hist.synchronize_history('trip_list_days',vrole_id,vsetting_user,vstation);
END modify_roles;

PROCEDURE delete_airline_profiles(vprofile_id       airline_profiles.profile_id%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
ids               TIdsTable;
i                 BINARY_INTEGER;
begin
  DELETE FROM profile_rights WHERE profile_id=vprofile_id RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST
    LOOP
      hist.synchronize_history('profile_rights',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;

  DELETE FROM airline_profiles WHERE profile_id=vprofile_id;
  hist.synchronize_history('airline_profiles',vprofile_id,vsetting_user,vstation);
end delete_airline_profiles;

PROCEDURE delete_roles(vrole_id       roles.role_id%TYPE,
                       vsetting_user  history_events.open_user%TYPE,
                       vstation       history_events.open_desk%TYPE)
IS
TYPE TIdsTable IS TABLE OF NUMBER(9);
ids               TIdsTable;
i                 BINARY_INTEGER;
BEGIN

  DELETE FROM trip_list_days WHERE role_id=vrole_id;
  hist.synchronize_history('trip_list_days',vrole_id,vsetting_user,vstation);

  DELETE FROM role_rights WHERE role_id=vrole_id RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST
    LOOP
      hist.synchronize_history('role_rights',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;

  DELETE FROM role_assign_rights WHERE role_id=vrole_id RETURNING id BULK COLLECT INTO ids;
  IF SQL%ROWCOUNT>0 THEN
    FOR i IN ids.FIRST..ids.LAST
    LOOP
      hist.synchronize_history('role_assign_rights',ids(i),vsetting_user,vstation);
    END LOOP;
  END IF;

  DELETE FROM roles WHERE role_id=vrole_id;
  hist.synchronize_history('roles',vrole_id,vsetting_user,vstation);
END delete_roles;

END adm;
/
