create or replace FUNCTION normalize_pax_doc(vdocument      IN     pax.drop_document%TYPE,
                                             vtype          OUT    pax_doc.type%TYPE,
                                             vissue_country OUT    pax_doc.issue_country%TYPE,
                                             vno            OUT    pax_doc.no%TYPE
                                           ) RETURN BOOLEAN
IS
TYPE TList IS TABLE OF pax_doc%ROWTYPE INDEX BY BINARY_INTEGER;
suffixes TList;
prefixes TList;
i        BINARY_INTEGER;
c        VARCHAR2(1);
mask     pax.drop_document%TYPE;
result   pax.drop_document%TYPE;
BEGIN
  prefixes(0).no:='‘ ';          prefixes(0).type:='P';     prefixes(0).issue_country:=NULL;
  prefixes(1).no:='PSP ';          prefixes(1).type:='P';     prefixes(1).issue_country:=NULL;
  prefixes(2).no:='DOCS/';         prefixes(2).type:=NULL;    prefixes(2).issue_country:=NULL;
  prefixes(3).no:='PSPT';          prefixes(3).type:=NULL;    prefixes(3).issue_country:=NULL;
  prefixes(4).no:='HK*PSPT';       prefixes(4).type:=NULL;    prefixes(4).issue_country:=NULL;
  prefixes(5).no:='PS RUS ';       prefixes(5).type:='P';     prefixes(5).issue_country:='”';
  prefixes(6).no:='PS RU ';        prefixes(6).type:='P';     prefixes(6).issue_country:='”';
  prefixes(7).no:='PS RF ';        prefixes(7).type:='P';     prefixes(7).issue_country:='”';
  prefixes(8).no:='PS RUS ';       prefixes(8).type:='P';     prefixes(8).issue_country:='”';
  prefixes(9).no:='RUS ';          prefixes(9).type:=NULL;    prefixes(9).issue_country:='”';
  prefixes(10).no:='FOID HK*/';    prefixes(10).type:=NULL;   prefixes(10).issue_country:=NULL;
  prefixes(11).no:='‘ ';          prefixes(11).type:='P';    prefixes(11).issue_country:=NULL;
  prefixes(12).no:='PS ';          prefixes(12).type:='P';    prefixes(12).issue_country:=NULL;

  suffixes(0).no:='ƒŽ‘„“Œ€” RUS'; suffixes(0).issue_country:='”';
  suffixes(1).no:='ƒŽ‘„“Œ€”';     suffixes(1).issue_country:=NULL;
  suffixes(2).no:=' ” M';         suffixes(2).issue_country:='”';
  suffixes(3).no:=' ” F';         suffixes(3).issue_country:='”';
  suffixes(4).no:=' RF';           suffixes(4).issue_country:='”';
  suffixes(5).no:=' RU';           suffixes(5).issue_country:='”';
  suffixes(6).no:=' RUS';          suffixes(6).issue_country:='”';
  suffixes(7).no:=' ”';           suffixes(7).issue_country:='”';

  --ã¡¨à ¥¬ ¤¢®©­ë¥ ¯à®¡¥«ë
  result:=NULL;
  c:=NULL;
  FOR i IN 1..LENGTH(vdocument) LOOP
    c:=SUBSTR(vdocument,i,1);
    IF c=' ' AND result IS NOT NULL AND SUBSTR(result,LENGTH(result),1)=' ' THEN
      NULL;
    ELSE
      result:=result||c;
    END IF;
  END LOOP;

  result:=TRIM(result);
  mask:=TRANSLATE(result,'0123456789', '**********');

  IF result IS NULL THEN
    RETURN FALSE;
  END IF;

  FOR i IN 0..prefixes.count-1 LOOP
    IF LENGTH(mask)>=LENGTH(prefixes(i).no) AND
       SUBSTR(mask,1,LENGTH(prefixes(i).no))=prefixes(i).no THEN
      result:=SUBSTR(result,LENGTH(prefixes(i).no)+1);
      vtype:=prefixes(i).type;
      vissue_country:=prefixes(i).issue_country;
      EXIT;
    END IF;
  END LOOP;

  result:=TRIM(result);
  mask:=TRANSLATE(result,'0123456789', '**********');

  FOR i IN 0..suffixes.count-1 LOOP
    IF LENGTH(mask)>=LENGTH(suffixes(i).no) AND
       SUBSTR(mask,LENGTH(mask)-LENGTH(suffixes(i).no)+1)=suffixes(i).no THEN
      result:=SUBSTR(result,1,LENGTH(result)-LENGTH(suffixes(i).no));
      IF suffixes(i).no='ƒŽ‘„“Œ€” RUS' OR
         suffixes(i).no='ƒŽ‘„“Œ€”' THEN
        result:=result||'ƒŽ‘„“Œ€';
      END IF;
      IF suffixes(i).issue_country IS NOT NULL THEN
        vissue_country:=suffixes(i).issue_country;
      END IF;
      EXIT;
    END IF;
  END LOOP;

  result:=TRIM(result);
  mask:=TRANSLATE(result,'0123456789', '**********');

  IF result IS NOT NULL AND LENGTH(result)>15 AND SUBSTR(result,1,3)='BC ' THEN
    result:=SUBSTR(result,4);
  END IF;

  IF result IS NOT NULL AND LENGTH(result)<=15 THEN
    vno:=result;
    RETURN TRUE;
  ELSE
    --§ ¯¨á âì ¢ «®£
    vtype:=NULL;
    vissue_country:=NULL;
    vno:=NULL;
    RETURN FALSE;
  END IF;
END;

/
