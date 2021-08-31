include(ts/macro.ts)

$(defmacro PREPARE_HALLS_FOR_BOARDING
  airp_dep
{
$(set airp_other_ $(if $(eq $(get_elem_id etAirp $(airp_dep)) "ДМД") "VKO" "ДМД"))

$(db_sql HALLS2 {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
                 VALUES(776, '$(get_elem_id etAirp $(airp_dep))', NULL, '$(airp_dep)', NULL, NULL, 0)})
$(db_sql HALLS2 {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
                 VALUES(777, '$(get_elem_id etAirp $(airp_dep))', NULL, '$(airp_dep)', NULL, NULL, 0)})
$(db_sql HALLS2 {INSERT INTO halls2(id, airp, terminal, name, name_lat, rpt_grp, pr_vip)
                 VALUES(778, '$(get_elem_id etAirp $(get airp_other_))', NULL, '$(get airp_other_)', NULL, NULL, 0)})
})

$(defmacro DEPLANE_ALL_REQUEST
  point_dep
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <DeplaneAll>
      <point_id>$(point_dep)</point_id>
      <boarding>0</boarding>
    </DeplaneAll>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST
  point_dep
  pax_id
  hall
{

!! capture=on err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByPaxId>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <pax_id>$(pax_id)</pax_id>
      <boarding>1</boarding>
      <dev_model/>
      <fmt_type/>
    </PaxByPaxId>
  </query>
</term>}

})

$(defmacro OK_TO_BOARD
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

>> lines=auto
      <updated>
        <pax_id>$(pax_id)</pax_id>
      </updated>

})

$(defmacro NO_BOARD
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

$(USER_ERROR_RESPONSE MSG.PASSENGER.APPS_PROBLEM)

})

$(defmacro APIS_INCOMPLETE_ON_BOARDING
  point_dep
  pax_id
  hall=777
{

$(BOARDING_REQUEST $(point_dep) $(pax_id) $(hall))

>> lines=auto
$(USER_ERROR_MESSAGE_TAG MSG.PASSENGER.APIS_INCOMPLETE 128)

})

$(defmacro BOARDING_REQUEST_BY_PAX_ID
  point_dep
  pax_id
  hall
  pax_tid
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByPaxId>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <pax_id>$(pax_id)</pax_id>\
$(if $(eq $(pax_tid) "") "" {
      <tid>$(pax_tid)</tid>})
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByPaxId>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST_BY_REG_NO
  point_dep
  reg_no
  hall
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByRegNo>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <reg_no>$(reg_no)</reg_no>
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByRegNo>
  </query>
</term>}

})

$(defmacro BOARDING_REQUEST_BY_SCAN_DATA
  point_dep
  scan_data
  hall
  boarding
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <PaxByScanData>
      <col_excess_type>0</col_excess_type>
      <point_id>$(point_dep)</point_id>
      <hall>$(hall)</hall>
      <scan_data hex='0'>$(scan_data)</scan_data>
      <boarding>$(boarding)</boarding>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
    </PaxByScanData>
  </query>
</term>}

})

$(defmacro LOAD_APIS_REQUEST
  point_dep
  pax_id
  pax_tid
  capture=off
{

!! capture=$(capture)
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <LoadPaxAPIS>
      <point_id>$(point_dep)</point_id>
      <pax_id>$(pax_id)</pax_id>
      <tid>$(pax_tid)</tid>
    </LoadPaxAPIS>
  </query>
</term>}

}) #end-of-macro LOAD_APIS_REQUEST

$(defmacro SAVE_APIS_REQUEST
  point_dep
  pax_id
  pax_tid
  apis
  capture=off
{

!! capture=$(capture)
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <SavePaxAPIS>
      <point_id>$(point_dep)</point_id>
      <pax_id>$(pax_id)</pax_id>
      <tid>$(pax_tid)</tid>
$(apis)
    </SavePaxAPIS>
  </query>
</term>}

}) #end-of-macro SAVE_APIS_REQUEST

