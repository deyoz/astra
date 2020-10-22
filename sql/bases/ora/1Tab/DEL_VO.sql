create table del_vo(
    point_id number(9) not null,
    full_name varchar2(129) not null,
    pers_type varchar2(2) not null,
    reg_no number(3) not null,
    ticket_no varchar2(15),
    coupon_no number(1),
    rem_codes varchar2(500),
    voucher varchar2(2) not null,
    total number(9) not null
);

