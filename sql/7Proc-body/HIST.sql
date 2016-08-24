create or replace PACKAGE BODY hist
AS

PROCEDURE synchronize_history(vtable_name   history_tables.code%TYPE,
                              vrow_ident    history_events.row_ident%TYPE,
                              vuser         history_events.open_user%TYPE,
                              vdesk         history_events.open_desk%TYPE)
IS
vtable_id             history_tables.id%TYPE;
vident_field          history_tables.ident_field%TYPE;
vhistory_fields       history_tables.history_fields%TYPE;
vtime                 history_events.open_time%TYPE;
vorder                history_events.hist_order%TYPE;
vhist_time            history_events.open_time%TYPE;
vhist_order           history_events.hist_order%TYPE;
sql_query             VARCHAR2(4000);
vdefault_time         history_events.close_time%TYPE;
BEGIN
  vdefault_time := TO_DATE('01.01.99 00:00:00', 'DD.MM.YY HH24:MI:SS');
  BEGIN
    SELECT id, ident_field, history_fields
    INTO vtable_id, vident_field, vhistory_fields
    FROM history_tables
    WHERE code=lower(vtable_name);
  EXCEPTION
    WHEN NO_DATA_FOUND THEN raise_application_error(-20000,'Incorrect table name ('||vtable_name||')');
  END;
  sql_query := 'UPDATE ' || vtable_name || ' SET ' || vident_field || '=' || vident_field || ' WHERE ' || vident_field || '=:vrow_ident';
  EXECUTE IMMEDIATE sql_query USING vrow_ident;
  IF SQL%ROWCOUNT=0 THEN
    UPDATE history_events SET close_time=system.UTCSYSDATE, close_user=vuser, close_desk=vdesk
    WHERE row_ident=vrow_ident AND table_id=vtable_id AND close_time = vdefault_time;
    RETURN;
  END IF;
  SELECT system.UTCSYSDATE, events__seq.nextval INTO vtime, vorder FROM dual;
  BEGIN
    SELECT hist_order, open_time INTO vhist_order, vhist_time FROM history_events
    WHERE row_ident=vrow_ident AND table_id=vtable_id AND close_time = vdefault_time;
  EXCEPTION
    WHEN TOO_MANY_ROWS THEN
      raise_application_error(-20000,'Database is corrupted (table name is "'||vtable_name||'", row_ident = '|| vrow_ident ||')!!');
    WHEN NO_DATA_FOUND THEN
      NULL;
  END;
  sql_query := 'INSERT INTO hist_' || vtable_name || '(' || vident_field || ', ' || vhistory_fields || ', hist_time, hist_order) '||
               '(SELECT :vrow_ident, ' || vhistory_fields || ', :vtime, :vorder FROM ' || vtable_name || ' WHERE ' || vident_field || '=:vrow_ident '||
               ' MINUS '||
               ' SELECT :vrow_ident, ' || vhistory_fields || ', :vtime, :vorder FROM hist_' || vtable_name ||
               ' WHERE ' || vident_field || '=:vrow_ident  AND hist_order=:vhist_order AND hist_time=:vhist_time)';
  EXECUTE IMMEDIATE sql_query USING vrow_ident, vtime, vorder, vrow_ident, vrow_ident, vtime, vorder, vrow_ident, vhist_order, vhist_time;
  IF SQL%ROWCOUNT=0 THEN
    RETURN;
  END IF;

  UPDATE history_events SET close_time=vtime, close_user=vuser, close_desk=vdesk
  WHERE row_ident=vrow_ident AND table_id=vtable_id AND close_time = vdefault_time;

  INSERT INTO history_events(table_id,row_ident,open_time,open_user,open_desk,hist_order,close_time)
  VALUES (vtable_id,vrow_ident,vtime,vuser,vdesk,vorder,vdefault_time);
END synchronize_history;

END hist;
/
