$(defmacro OPEN_CHECKIN
  point_id
  act="$(date_format %d.%m.%Y) $(date_format %H:%M)"
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>10</stage_id>
              <act>$(act):00</act>
            </stage>
            <stage>
              <stage_id>20</stage_id>
              <act>$(act):00</act>
            </stage>
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>...</message>
    </command>

}) #end-of-macro

#########################################################################################

$(defmacro CLOSE_CHECKIN
  point_id
  act="$(date_format %d.%m.%Y) $(date_format %H:%M)"
{
!! capture=on
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteTrips>
      <trips>
        <trip>
          <point_id>$(point_id)</point_id>
          <tripstages>
            <stage>
              <stage_id>30</stage_id>
              <act>$(act):00</act>
            </stage>
          </tripstages>
        </trip>
      </trips>
    </WriteTrips>
  </query>
</term>}

>> lines=auto
    <command>
      <message lexema_id='MSG.DATA_SAVED' code='0'>...</message>
    </command>

}) #end-of-macro

#########################################################################################
