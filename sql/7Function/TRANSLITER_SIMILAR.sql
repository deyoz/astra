create or replace FUNCTION transliter_similar(str1 IN VARCHAR2,
                            str2 IN VARCHAR2,
                            fmt  IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
s1 VARCHAR2(4000);
s2 VARCHAR2(4000);
min_len NUMBER;
BEGIN
  IF str1 IS NULL OR str2 IS NULL THEN RETURN 0; END IF;
  FOR ifmt IN 1..3 LOOP
    IF fmt IS NULL OR
       fmt IS NOT NULL AND fmt=ifmt THEN
      s1:=system.transliter(str1,ifmt);
      s2:=system.transliter(str2,ifmt);
      min_len:=LEAST(LENGTH(s1), LENGTH(s2));
--      IF min_len IS NULL THEN RETURN 0; END IF;
      IF SUBSTR(s1,1,min_len)=SUBSTR(s2,1,min_len) THEN RETURN 1; END IF;
    END IF;
  END LOOP;
  RETURN 0;
END transliter_similar;
/
