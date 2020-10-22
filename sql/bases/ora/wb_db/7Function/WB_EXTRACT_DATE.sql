create or replace FUNCTION WB_EXTRACT_DATE
(INPUT_DATE date)
RETURN date
is
begin
   return to_date((to_char(EXTRACT(DAY FROM INPUT_DATE))||'.'||
                     to_char(EXTRACT(MONTH FROM INPUT_DATE))||'.'||
                       to_char(EXTRACT(YEAR FROM INPUT_DATE))), 'dd.mm.yyyy');
  end;
/
