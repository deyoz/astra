truncate table COMP_ELEM_TYPES;
insert into COMP_ELEM_TYPES (CODE, NAME, PR_SEAT, TIME_CREATE, IMAGE, PR_DEL, NAME_LAT, FILENAME) values ('Д', 'Диван двойной', 1, sysdate, utl_raw.cast_to_raw('image'), 0, 'Double sofa', 'dsofa');
insert into COMP_ELEM_TYPES (CODE, NAME, PR_SEAT, TIME_CREATE, IMAGE, PR_DEL, NAME_LAT, FILENAME) values ('А', 'У аварийного выхода', 1, sysdate, utl_raw.cast_to_raw('image'), 0, 'Seat near emergency exit', 'eexit');
insert into COMP_ELEM_TYPES (CODE, NAME, PR_SEAT, TIME_CREATE, IMAGE, PR_DEL, NAME_LAT, FILENAME) values ('К', 'Кресло', 1, sysdate, utl_raw.cast_to_raw('image'), 0, 'Seat', 'seat');
insert into COMP_ELEM_TYPES (CODE, NAME, PR_SEAT, TIME_CREATE, IMAGE, PR_DEL, NAME_LAT, FILENAME) values ('П', 'Перегородка', 0, sysdate, utl_raw.cast_to_raw('image'), 0, 'Partition', 'part');
