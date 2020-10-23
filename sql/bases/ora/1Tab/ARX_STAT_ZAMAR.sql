create table arx_stat_zamar (
    time date not null,
    airline varchar2(3) not null,
    airp varchar2(3) not null,
    amount_ok number(9) not null,
    amount_fault number(9) not null,
    part_key date not null
);
