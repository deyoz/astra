create or replace PROCEDURE clear_typeb_in
IS
min_id typeb_in.id%TYPE;
max_id typeb_in.id%TYPE;
CURSOR cur(curr_id typeb_in.id%TYPE) IS
  SELECT id
  FROM typeb_in
  WHERE id>=curr_id AND id<curr_id+10000 AND NOT EXISTS (SELECT * FROM tlgs_in WHERE tlgs_in.id=typeb_in.id AND rownum<2);
BEGIN
  SELECT MIN(id) INTO min_id FROM typeb_in;
  SELECT MAX(id) INTO max_id FROM typeb_in;
  WHILE min_id<=max_id LOOP
    FOR curRow IN cur(min_id) LOOP
      INSERT INTO typeb_in_ids(id) VALUES(curRow.id);
    END LOOP;
    COMMIT;
    min_id:=min_id+10000;
  END LOOP;

END clear_typeb_in;
/
