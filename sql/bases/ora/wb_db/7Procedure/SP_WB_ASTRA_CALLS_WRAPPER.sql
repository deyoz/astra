create or replace function SP_WB_ASTRA_CALLS_WRAPPER(CLOB_IN in clob) return clob
is
    CLOB_OUT clob;
begin
    SP_WB_ASTRA_CALLS(CLOB_IN, CLOB_OUT);
    return CLOB_OUT;
end SP_WB_ASTRA_CALLS_WRAPPER;
/
