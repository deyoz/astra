######################################################################################################

$(defmacro READ_TRIPS
  date=$(date_format %d.%m.%Y)
{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <ReadTrips>
      <flight_date>$(date) 00:00:00</flight_date>
    </ReadTrips>
  </query>
</term>}
}) #end_of_macro

######################################################################################################

$(defmacro READ_ARX_TRIPS
    arx_date=$(date_format %d.%m.%Y)

{{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='STAT.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <ReadTrips>
      <arx_date>$(arx_date) 00:00:00</arx_date>
    </ReadTrips>
  </query>
</term>}
}) #end_of_macro

######################################################################################################

$(defmacro CHECK_TRIP_ALARMS
  date
  alarm1
  alarm2
  alarm3
  alarm4

{

!! capture=on
$(READ_TRIPS $(date))

>> lines=auto
          <alarms>
            <alarm text='...'>$(alarm1)</alarm>\
$(if $(eq $(alarm2) "") "" {
            <alarm text='...'>$(alarm2)</alarm>})\
$(if $(eq $(alarm3) "") "" {
            <alarm text='...'>$(alarm3)</alarm>})\
$(if $(eq $(alarm4) "") "" {
            <alarm text='...'>$(alarm4)</alarm>})
          </alarms>

})

$(defmacro EXISTS_TRIP_ALARM
  date
  alarm
{

!! capture=on
$(READ_TRIPS $(date))

>> mode=regex
.*
            <alarm text='.*'>$(alarm)</alarm>
.*

})

$(defmacro NOT_EXISTS_TRIP_ALARM
  date
  alarm
{

!! capture=on
$(READ_TRIPS $(date))

>> mode=!regex
.*
            <alarm text='.*'>$(alarm)</alarm>
.*

})

