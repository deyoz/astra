create or replace FUNCTION check_locale_row(vid      locale_messages.id%TYPE,
                          vlang    locale_messages.lang%TYPE,
                          vtext    locale_messages.text%TYPE,
                          vpr_term locale_messages.pr_term%TYPE) RETURN VARCHAR2
IS
i BINARY_INTEGER;
BEGIN
  IF system.is_lat(vid) THEN
    --�����䨪��� ��⨭᪨�
    IF vlang='RU' THEN
      --�஢�ਬ ����
      SELECT COUNT(*) INTO i FROM locale_messages WHERE id=vid AND lang='EN';
      IF i=0 THEN
        RETURN '��� ���� �� ������᪮�';
      END IF;

      IF system.is_name(vid,NULL,'._')=0 THEN
        RETURN '��࠭�� ᨬ���� � ��.';
      END IF;
    END IF;
    IF (vlang='EN') AND UPPER(vid)!=vid THEN
      RETURN '��⨭᪨� ⥪�� ��� ��⨭᪮�� ��.';
    END IF;
  ELSE
    --�����䨪��� ���᪨� (�� ��ॢ�� �ନ����)
    IF vlang='RU' THEN
      RETURN '���᪨� ⥪�� ��� ���᪮�� ��.';
    END IF;
    /*
    IF vpr_term=0 OR vpr_term IS NULL THEN
      SELECT COUNT(*) INTO i FROM
        (SELECT title FROM cache_tables WHERE title=vid
         UNION
         SELECT title FROM cache_fields WHERE title=vid);
      IF i=0 THEN
        RETURN '���᪨� ��. ��� �ࢥ୮� ���';
      END IF;
    END IF;
    */
  END IF;
  IF system.is_lat(vtext) THEN
    IF vlang='RU' THEN
      RETURN '��⨭᪨� ⥪�� ��� RU';
    END IF;
  ELSE
    IF vlang='EN' THEN
      RETURN '���᪨� ⥪�� ��� EN';
    END IF;
  END IF;
  SELECT COUNT(*) INTO i FROM (SELECT DISTINCT pr_term FROM locale_messages WHERE id=vid);
  IF i>1 THEN
    RETURN '����� pr_term ��� ������ ��.';
  END IF;
  RETURN NULL;
END;
/
