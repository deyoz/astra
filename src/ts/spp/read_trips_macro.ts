######################################################################################################

$(defmacro READ_TRIPS
  date=$(date_format %d.%m.%Y)
  lang=RU
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <ReadTrips>
      <flight_date>$(date) 00:00:00</flight_date>
    </ReadTrips>
  </query>
</term>}
) #end_of_macro

######################################################################################################

$(defmacro READ_ARX_TRIPS
    arx_date=$(date_format %d.%m.%Y)

{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='STAT.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <ReadTrips>
      <arx_date>$(arx_date) 00:00:00</arx_date>
    </ReadTrips>
  </query>
</term>}
) #end_of_macro

######################################################################################################

$(defmacro CHECK_TRIP_ALARMS
  date
  alarm1
  alarm2
  alarm3
  alarm4

{

!! capture=on
$(READ_TRIPS $(date) EN)

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

$(defmacro GET_PAX_TRANSFER_REQUEST
  point_id
  pr_out
  pr_tlg
  lang=RU
  capture=off
{
!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetPaxTransfer>
      <point_id>$(point_id)</point_id>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <pr_out>$(pr_out)</pr_out>
      <pr_tlg>$(pr_tlg)</pr_tlg>
    </GetPaxTransfer>
  </query>
</term>

})

$(defmacro GET_BAG_TRANSFER_REQUEST
  point_id
  pr_out
  pr_tlg
  lang=RU
  capture=off
{
!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetBagTransfer>
      <point_id>$(point_id)</point_id>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <pr_out>$(pr_out)</pr_out>
      <pr_tlg>$(pr_tlg)</pr_tlg>
    </GetBagTransfer>
  </query>
</term>

})

$(defmacro GET_ADV_TRIP_LIST_CHECKIN_REQUEST
  date #%d.%m.%Y
  lang=RU
  capture=off
{
!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetAdvTripList>
      <date>$(date) 00:00:00</date>
      <filter>
        <pr_takeoff>1</pr_takeoff>
      </filter>
      <view>
        <codes_fmt>5</codes_fmt>
      </view>
    </GetAdvTripList>
  </query>
</term>

})

$(defmacro GET_ADV_TRIP_LIST_PREP_CHECKIN_REQUEST
  date #%d.%m.%Y
  lang=RU
  capture=off
{
!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='PREPREG.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetAdvTripList>
      <date>$(date) 00:00:00</date>
      <filter/>
      <view>
        <codes_fmt>5</codes_fmt>
      </view>
    </GetAdvTripList>
  </query>
</term>

})

$(defmacro GET_ADV_TRIP_LIST_BOARDING_REQUEST
  date #%d.%m.%Y
  lang=RU
  capture=off
{
!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='trips' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <GetAdvTripList>
      <date>$(date) 00:00:00</date>
      <filter/>
      <view>
        <codes_fmt>5</codes_fmt>
      </view>
    </GetAdvTripList>
  </query>
</term>

})

$(defmacro GET_ADV_TRIP_LIST_RESPONSE
  date #%d.%m.%Y
  trips
{

>>
<?xml version='1.0' encoding='CP866'?>
<term>
  <answer ...>
    <date>$(date) 00:00:00</date>\
$(if $(eq $(trips) "") {
    <trips/>} {
    <trips>
$(trips)
    </trips>})
  </answer>
</term>

})

$(defmacro adv_trip_list_item
  point_id
  name
  date #dd
  airp
  name_sort_order
  date_sort_order
  airp_sort_order
{      <trip>
        <point_id>$(point_id)</point_id>
        <name>$(name)</name>
        <date>$(date)...</date>
        <airp>$(airp)</airp>
        <name_sort_order>$(name_sort_order)</name_sort_order>\
$(if $(eq $(date_sort_order) "") "" {
        <date_sort_order>$(date_sort_order)</date_sort_order>})\
$(if $(eq $(airp_sort_order) "") "" {
        <airp_sort_order>$(airp_sort_order)</airp_sort_order>})
      </trip>})
