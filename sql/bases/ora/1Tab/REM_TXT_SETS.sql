create table rem_txt_sets (
    id number(9) not null,
    airline varchar2(3) not null,
    rfisc varchar2(15),
    tag_index number(1) not null,
    text_length number(2) not null,
    text varchar2(100) not null,
    pr_lat number(1) not null,
    brand_airline varchar2(3),
    brand_code varchar2(10),
    fqt_airline varchar2(3),
    fqt_tier_level varchar2(50)
);
