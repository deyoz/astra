create or replace trigger TR_REG
AFTER
INSERT OR UPDATE OR DELETE
ON REG
FOR EACH ROW
DECLARE
  str  arc_reg%ROWTYPE;
  hh   arc_head.h_arc;
  code VARCHAR2(1);
  card VARCHAR2(16);
  Id_a NUMBER(10);
BEGIN
  SELECT seq_arc.nextval INTO str.ida FROM dual;
  card:= 'ARC_REG';
  str.rcoder := NULL;
  str.rcodern:= NULL;
  str.lcoder := NULL;
  str.codeg  := NULL;
  str.codef  := NULL;
  str.rname  := NULL;
  str.lname  := NULL;
  Id_a := :OLD.ida;
  IF DELETING THEN
     code := 'D';
     str.rcoder := :OLD.rcoder;
     str.lcoder := :OLD.lcoder;
     str.codeg  := :OLD.codeg;
     str.codef  := :OLD.codef;
     str.rname  := :OLD.rname;
     str.lname  := :OLD.lname;
  ELSIF INSERTING THEN
     code := 'I';
     str.rcoder := :NEW.rcoder;
     str.lcoder := :NEW.lcoder;
     str.codeg  := :NEW.codeg;
     str.codef  := :NEW.codef;
     str.rname  := :NEW.rname;
     str.lname  := :NEW.lname;
     Id_a := :NEW.ida;
  ELSE
     code:= 'U';
     IF :OLD.rcoder IS NULL OR :NEW.rcoder != :OLD.rcoder THEN
      str.rcodern := :NEW.rcoder;
      str.rcoder  := :OLD.rcoder;
     ELSE str.rcoder := :OLD.rcoder; END IF;
     IF :OLD.lcoder IS NULL OR :NEW.lcoder != :OLD.lcoder THEN str.lcoder := :NEW.lcoder;
     ELSE str.lcoder := :OLD.lcoder; END IF;
     IF :OLD.codeg  IS NULL OR :NEW.codeg != :OLD.codeg THEN str.codeg := :NEW.codeg; END IF;
     IF :OLD.codef  IS NULL OR :NEW.codef != :OLD.codef THEN str.codef := :NEW.codef; END IF;
     IF :OLD.rname  IS NULL OR :NEW.rname  != :OLD.rname  THEN str.rname  := :NEW.rname;  END IF;
     IF :OLD.lname  IS NULL OR :NEW.lname  != :OLD.lname  THEN str.lname  := :NEW.lname;  END IF;
  END IF;
  arc_head.get( hh, Id_a, code, card);
  INSERT INTO arc_ref ( ID, T_EXEC, O_EXEC, AGN, AWK, NO, CODE, GRP, TBL, IDA)
         VALUES ( hh.h_id, hh.h_dat, hh.h_pul, hh.h_agn, hh.h_awk, hh.h_no, hh.h_code,
                  arc_head.group_0, hh.h_card, str.ida);
  INSERT INTO arc_reg ( rcoder, rcodern, lcoder, codeg, codef, rname, lname, IDA)
         VALUES ( str.rcoder, str.rcodern, str.lcoder, str.codeg, str.codef,
                  str.rname, str.lname, str.ida);
EXCEPTION
  WHEN OTHERS THEN
    DBMS_OUTPUT.PUT_LINE('reg: '||SQLERRM(SQLCODE));
END;
/
