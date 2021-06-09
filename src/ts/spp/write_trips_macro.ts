$(defmacro SET_STAGE
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
  stage_id1
  stage_id2
{

!! capture=on
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>$(stage_id1)</stage_id>
              <act>$(act):00</act>
            </stage>\
$(if $(eq $(stage_id2) "") "" {
            <stage>
              <stage_id>$(stage_id2)</stage_id>
              <act>$(act):00</act>
            </stage>})
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>...</message>
    </command>

}) #end-of-macro

#########################################################################################

$(defmacro CANCEL_STAGE
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
  stage_id1
  stage_id2
{

!! capture=on
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>$(stage_id1)</stage_id>
              <old_act>$(act):00</old_act>
            </stage>\
$(if $(eq $(stage_id2) "") "" {
            <stage>
              <stage_id>$(stage_id2)</stage_id>
              <old_act>$(act):00</old_act>
            </stage>})
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>...</message>
    </command>

}) #end-of-macro

#########################################################################################


$(defmacro PREP_CHECKIN
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 10)

}) #end-of-macro

#########################################################################################

$(defmacro OPEN_CHECKIN
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 10 20)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_CHECKIN
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 30)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_CHECKIN_CANCEL
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(CANCEL_STAGE $(point_id) $(act) 30)

}) #end-of-macro

#########################################################################################

$(defmacro OPEN_WEB_CHECKIN
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 25)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_WEB_CHECKIN
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 35)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_WEB_CHECKIN_CANCEL
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(CANCEL_STAGE $(point_id) $(act) 35)

}) #end-of-macro

#########################################################################################

$(defmacro OPEN_BOARDING
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 40)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_BOARDING
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 50)

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_BOARDING_CANCEL
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(CANCEL_STAGE $(point_id) $(act) 50)

}) #end-of-macro

#########################################################################################

$(defmacro REMOVE_GANGWAY
  point_id
  act=$(date_format {%d.%m.%Y %H:%M})
{

$(SET_STAGE $(point_id) $(act) 70)

}) #end-of-macro

#########################################################################################


