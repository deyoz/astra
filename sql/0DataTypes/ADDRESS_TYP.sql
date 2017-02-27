create or replace type address_typ as object (
    code varchar(20),
    visible number(9),
    caption varchar(20),
    ctype varchar(20),
    width number(5),
    len number(5),
    isalnum number(1),
    err_lexeme varchar2(100)
);
/
