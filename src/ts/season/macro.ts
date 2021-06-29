$(defmacro SEASON_READ
    user
{{<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='season' ver='1' opr='$(user)' screen='SEASON.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <season_read>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <LoadForm/>
    </season_read>
  </query>
</term>}
}) #end-of-defmarot SEASON_READ

#########################################################################################

$(defmacro GET_SPP
    user
    ondate
{{<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='season' ver='1' opr='$(user)' screen='SEASON.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <get_spp>
      <date>$(ondate)</date>
    </get_spp>
  </query>
</term>}
}) #end-of-macro GET_SPP

#########################################################################################

$(defmacro EDIT_SEASON_TRIP
    user
    trip_id
{{<?xml version='1.0' encoding='UTF-8'?>
<term>
  <query handle='0' id='season' ver='1' opr='$(user)' screen='SEASON.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <edit>
      <filter>
        <season>2</season>
      </filter>
      <trip_id>$(trip_id)</trip_id>
    </edit>
  </query>
</term>}
}) #end-of-defmarot EDIT_SEASON_TRIP
