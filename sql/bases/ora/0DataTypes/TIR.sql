create or replace TYPE             "TIR"                                          as object(code number(2));
create type tid as table of tir;

select * from (tid(tir(53), tir(52), tir(68), tir(83), tir(84), tir(87), tir(69), tir(94), tir(73), tir(20), tir(21), tir(36), tir(19), tir(11), 0, tir(77), tir(64), tir(85), tir(86)));


--select code from bag_types where code in(53, 52, 68, 83, 84, 87, 69, 94, 73, 20, 21, 36, 19, 11, 0, 77, 64, 85, 86) order by code;
/
