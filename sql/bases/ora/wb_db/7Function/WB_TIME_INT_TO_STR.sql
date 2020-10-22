create or replace FUNCTION WB_TIME_INT_TO_STR
(INPUT_VAL number,
   SEPARATOR varchar2)
RETURN varchar2
is
HOUR_PART varchar2(50);
MIN_PART varchar2(50);
  begin
    if floor(abs(INPUT_VAL)/60)<10 then
      begin
        HOUR_PART:='0'||to_char(floor(abs(INPUT_VAL)/60));

      end;
    else
     begin
        HOUR_PART:=to_char(floor(abs(INPUT_VAL)/60));

      end;
    end if;

    if abs(INPUT_VAL)-60*floor(abs(INPUT_VAL)/60)<10 then
      begin
        MIN_PART:='0'||to_char(abs(INPUT_VAL)-60*floor(abs(INPUT_VAL)/60));

      end;
    else
      begin
        MIN_PART:=to_char(abs(INPUT_VAL)-60*floor(abs(INPUT_VAL)/60));

      end;
    end if;

    return HOUR_PART||SEPARATOR||MIN_PART;
  end;
/
