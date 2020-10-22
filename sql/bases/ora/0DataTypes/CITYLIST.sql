create or replace TYPE CITYLIST AS TABLE OF VARCHAR2(3);

CREATE OR REPLACE PROCEDURE CHANGE_CITY_TZ_REGION
  (cityList in CITYLIST)
AS
  cursor cr_cities is
    select distinct code from airps where city in(
      select CODE_LAT from cities where code in (select city from cityList) and code_lat is not null
      union
      select CODE from cities where code in (select city from cityList)
    );

BEGIN

  open cr_cities;
  for code in cr_cities
  loop

  end loop;
END CHANGE_CITY_TZ_REGION;
/
