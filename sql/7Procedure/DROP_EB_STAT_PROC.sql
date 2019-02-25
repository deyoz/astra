create or replace PROCEDURE drop_eb_stat_proc
IS
vmin_time DATE;
vmax_time DATE;
vrow_nums NUMBER(9);
low_time DATE;
high_time DATE;
BEGIN
  SELECT TO_DATE('01.07.16','DD.MM.YY') INTO vmin_time FROM dual;
  SELECT TO_DATE('22.08.16','DD.MM.YY') INTO vmax_time FROM dual;
  WHILE vmin_time<vmax_time LOOP
    low_time:=vmin_time;
    high_time:=vmin_time+0.1;
    SELECT COUNT(*) INTO vrow_nums
    FROM events_bilingual
    WHERE time>=low_time AND time<high_time;
    INSERT INTO drop_eb_stat(curr_time,row_nums) VALUES(vmin_time,vrow_nums);
    COMMIT;
    vmin_time:=vmin_time+1.0;
  END LOOP;
END drop_eb_stat_proc;
/
