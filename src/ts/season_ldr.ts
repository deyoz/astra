include(ts/macro.ts)
include(ts/adm_macro.ts)

# Файл запускается в bin/createdb.sh
# не участвует в тестах

# версия макроса для createdb

$(defmacro PREPARE_SEASON_SCD_LDR
  airl
  depp
  arrp
  fltno
  moveid=-1
  craft=TU5
  first_date=$(date_format %d.%m.%Y)
  last_date=$(date_format %d.%m.%Y)
  takeoff=30.12.1899
  land=30.12.1899
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
  <query handle='0' id='season' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <write>
      <filter>
        <season>2</season>
      </filter>
      <SubrangeList>
        <subrange>
          <modify>insert</modify>
          <move_id>$(moveid)</move_id>
          <first>$(first_date) 00:00:00</first>
          <last>$(last_date) 00:00:00</last>
          <days>1234567</days>
          <dests>
            <dest>
              <cod>$(depp)</cod>
              <company>$(airl)</company>
              <trip>$(fltno)</trip>
              <bc>$(craft)</bc>
              <takeoff>$(takeoff) 15:00:00</takeoff>
              <y>-1</y>
            </dest>
            <dest>
              <cod>$(arrp)</cod>
              <land>$(land) 17:30:00</land>
            </dest>
          </dests>
        </subrange>
      </SubrangeList>
    </write>
  </query>
</term>}
}) # end-of-macro PREPARE_SEASON_SCD_LDR


$(init_term 201707-0195750)

#############################################

$(PREPARE_SEASON_SCD_LDR СУ ШРМ ПЛК 9999 -1 737 01.01.2021 31.12.2025)

#############################################

$(sql "commit")
