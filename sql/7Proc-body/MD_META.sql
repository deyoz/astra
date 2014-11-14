create or replace PACKAGE BODY "MD_META" AS

    multiplier constant number  := 22695477;
    increment  constant number  := 1;
    "2^32"     constant number  := 2 ** 32;
    "2^16"     constant number  := 2 ** 16;
    "0x7fff"   constant number  := 32767;
    Seed       number := 1;
    g_curr_val NUMBER DEFAULT NULL;
/*
 * Get the next available id for objectids.
 * @return the next id that can be used in objectid fields
 * This code is taken from wwv_flows
 */
FUNCTION get_next_id RETURN NUMBER
IS
BEGIN
/*
	select  to_number(
                 to_char(dbobjectid_sequence.nextval) ||
                      lpad( substr( abs(rand), 1, 5 ),5, '0' ) ||
                   ltrim(to_char(mod(abs(hsecs),1000000),'000000')))
    into    g_curr_val
    from   sys.v_$timer;
*/
	select to_number(
				to_char(dbobjectid_sequence.nextval) ||
					lpad( substr( abs(rand), 1, 5 ), 5, '0') ||
				ltrim(to_char(mod(dbms_random.value(100000000000, 999999999999),1000000),'000000')))
    into    g_curr_val
	from dual;				

    return g_curr_val;
END get_next_id;
-- Following code taken from wwv_flows_random
 procedure srand( new_seed in number ) is
 begin
  Seed := new_seed;
 end srand;
--
  function rand return number is
    s number;
  begin
    seed := mod( multiplier * seed + increment, "2^32" );
    begin
       return bitand( seed/"2^16", "0x7fff" );
    --mhichwa
    exception when others then
       select dbobjectid_sequence.nextval into s from dual;
       return s||to_char(sysdate,'HH24MISS');
    end;
  end rand;
--
  procedure get_rand( r OUT number ) is
  begin
    r := rand;
  end get_rand;
--
  function rand_max( n IN number ) return number is
  begin
    return mod( rand, n ) + 1;
  end rand_max;
--
  procedure get_rand_max( r OUT number, n IN number )  is
  begin
    r := rand_max( n );
  end get_rand_max;
--
-- One time initialisation
begin
  select to_char(sysdate,'JSSSSS')
    into seed
    from dual;
end;
/
