create table stat_zamar (
    time date not null,
    airline varchar2(3) not null,
    airp varchar2(3) not null,
    amount_ok number(9) not null,
    amount_fault number(9) not null,
    sbdo_type varchar2(10) not null
);
