create or replace trigger TR_SFE
AFTER
INSERT OR UPDATE OR DELETE
ON SFE
FOR EACH ROW
DECLARE
  str  arc_sfe%ROWTYPE;
  hh   arc_head.h_arc;
  code VARCHAR2(1);
  card VARCHAR2(16);
  Id_a NUMBER(10);
  nt   NUMBER(1);
BEGIN

  SELECT seq_arc.nextval INTO str.ida FROM dual;
  card:= 'ARC_SFE';
  str.rcodec := NULL;
  str.rcodecn:= NULL;
  str.lcodec := NULL;
  str.coder  := NULL;
  str.codeg  := NULL;
  str.codef  := NULL;
  str.rname  := NULL;
  str.lname  := NULL;
  str.teladr := NULL;
  str.tmport := NULL;
  str.flprt  := NULL;
  str.latit  := NULL;
  str.longit := NULL;
  str.in_use := NULL;
  str.large_city := NULL;
  str.not_real_city := NULL;
  str.timezone := NULL;
  Id_a := :OLD.ida;

  IF DELETING THEN
     code := 'D';
     str.rcodec := :OLD.rcodec;
     str.lcodec := :OLD.lcodec;
     str.coder  := :OLD.coder;
     str.codeg  := :OLD.codeg;
     str.codef  := :OLD.codef;
     str.rname  := :OLD.rname;
     str.lname  := :OLD.lname;
     str.teladr := :OLD.teladr;
     str.tmport := :OLD.tmport;
     str.flprt  := :OLD.flprt;
     str.latit  := :OLD.latit;
     str.longit := :OLD.longit;
     str.in_use := :OLD.in_use;
     str.large_city := :OLD.large_city;
     str.not_real_city := :OLD.not_real_city;
     str.timezone := :OLD.timezone;
  ELSIF INSERTING THEN
     code := 'I';
     str.rcodec := :NEW.rcodec;
     str.lcodec := :NEW.lcodec;
     str.coder  := :NEW.coder;
     str.codeg  := :NEW.codeg;
     str.codef  := :NEW.codef;
     str.rname  := :NEW.rname;
     str.lname  := :NEW.lname;
     str.teladr := :NEW.teladr;
     str.tmport := :NEW.tmport;
     str.flprt  := :NEW.flprt;
     str.latit  := :NEW.latit;
     str.longit := :NEW.longit;
     str.in_use := :NEW.in_use;
     str.large_city := :NEW.large_city;
     str.not_real_city := :NEW.not_real_city;
     str.timezone := :NEW.timezone;
     Id_a := :NEW.ida;
  ELSE
    IF :NEW.IDA<0 THEN
      SELECT COUNT(*) INTO nt FROM DUAL WHERE
      EXISTS (SELECT 1 FROM JSON_RSM R,PAR WHERE (PAR.CITY1=:OLD.IDA OR PAR.CITY2=:OLD.IDA)
      AND PAR.RID=R.IDA AND ROWNUM<2) OR
      EXISTS (SELECT 1 FROM RSD,PAR WHERE (PAR.CITY1=:OLD.IDA OR PAR.CITY2=:OLD.IDA)
      AND PAR.RID=RSD.IDA AND ROWNUM<2) OR
      EXISTS (SELECT 1 FROM PUL WHERE CODEC=:OLD.IDA AND ROWNUM<2);
        IF nt!=0 THEN raise_application_error(-20111,'RECORD ALIVE'); END IF;
    END IF;
     code:= 'U';
     IF :OLD.rcodec IS NULL OR :NEW.rcodec != :OLD.rcodec THEN
      str.rcodecn := :NEW.rcodec;
      str.rcodec:= :OLD.rcodec;
     ELSE str.rcodec := :OLD.rcodec; END IF;
     IF :OLD.lcodec IS NULL OR :NEW.lcodec != :OLD.lcodec THEN str.lcodec := :NEW.lcodec;
     ELSE str.lcodec := :OLD.lcodec; END IF;
     IF :OLD.coder  IS NULL OR :NEW.coder  != :OLD.coder  THEN str.coder  := :NEW.coder;  END IF;
     IF :OLD.codeg  IS NULL OR :NEW.codeg  != :OLD.codeg  THEN str.codeg  := :NEW.codeg;  END IF;
     IF :OLD.codef  IS NULL OR :NEW.codef  != :OLD.codef  THEN str.codef  := :NEW.codef;  END IF;
     IF :OLD.rname  IS NULL OR :NEW.rname  != :OLD.rname  THEN str.rname  := :NEW.rname;  END IF;
     IF :OLD.lname  IS NULL OR :NEW.lname  != :OLD.lname  THEN str.lname  := :NEW.lname;  END IF;
     IF :OLD.teladr IS NULL OR :NEW.teladr != :OLD.teladr THEN str.teladr := :NEW.teladr;  END IF;
     IF :OLD.tmport IS NULL OR :NEW.tmport != :OLD.tmport THEN str.tmport := :NEW.tmport;  END IF;
     IF :OLD.flprt  IS NULL OR :NEW.flprt  != :OLD.flprt  THEN str.flprt  := :NEW.flprt;  END IF;
     IF :OLD.latit  IS NULL OR :NEW.latit  != :OLD.latit  THEN str.latit  := :NEW.latit;  END IF;
     IF :OLD.longit IS NULL OR :NEW.longit != :OLD.longit THEN str.longit := :NEW.longit; END IF;
     IF :OLD.in_use IS NULL OR :NEW.in_use != :OLD.in_use THEN str.in_use := :NEW.in_use; END IF;
     IF :OLD.large_city IS NULL OR :NEW.large_city != :OLD.large_city THEN str.large_city := :NEW.large_city; END IF;
     IF :OLD.not_real_city IS NULL OR :NEW.not_real_city != :OLD.not_real_city THEN str.not_real_city := :NEW.not_real_city; END IF;
     IF :OLD.timezone IS NULL OR :NEW.timezone != :OLD.timezone THEN str.timezone := :NEW.timezone; END IF;
  END IF;

  arc_head.get( hh, Id_a, code, card);

  INSERT INTO arc_ref ( ID, T_EXEC, O_EXEC, AGN, AWK, NO, CODE, GRP, TBL, IDA)
         VALUES ( hh.h_id, hh.h_dat, hh.h_pul, hh.h_agn, hh.h_awk, hh.h_no, hh.h_code,
                  arc_head.group_0, hh.h_card, str.ida);

  INSERT INTO arc_sfe (rcodec, rcodecn, lcodec, codeg, codef, tmport, teladr/*, tmreg*/, coder,
                       flprt, rname, lname, latit, longit, IDA, in_use, LARGE_CITY, timezone, not_real_city)
         VALUES ( str.rcodec, str.rcodecn, str.lcodec, str.codeg, str.codef, str.tmport, str.teladr,
                  /*str.tmreg, */str.coder, str.flprt, str.rname,
                  str.lname, str.latit, str.longit, str.ida, str.in_use, str.large_city, str.timezone, str.not_real_city);

  SFEAERPACK.pushsfe(:OLD.ida);
  SFEAERPACK.pushsfe(:NEW.ida);
/*
EXCEPTION
  WHEN OTHERS THEN
    DBMS_OUTPUT.PUT_LINE('sfe: '||SQLERRM(SQLCODE));
*/
END;

/
