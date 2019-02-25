create or replace function julian_date(val number) return date is
begin
    return trunc(sysdate, 'year') - 1 + val;
end;
/
