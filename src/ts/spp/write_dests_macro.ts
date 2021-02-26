$(defmacro new_spp_point
  airline
  flt_no
  craft
  bort
  scd_in   #формат даты: dd.mm.yyyy hh:nn
  airp
  scd_out  #формат даты: dd.mm.yyyy hh:nn
  trip_type=п
{          <dest>
            <modify/>
            <airp>$(airp)</airp>
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>\
$(if $(eq $(craft) "") "" {
            <craft>$(craft)</craft>})\
$(if $(eq $(bort) "") "" {
            <bort>$(bort)</bort>})\
$(if $(eq $(scd_in) "") "" {
            <scd_in>$(scd_in):00</scd_in>})\
$(if $(eq $(scd_out) "") "" {
            <scd_out>$(scd_out):00</scd_out>})
            <trip_type>$(trip_type)</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>})

$(defmacro change_spp_point
  point_id
  airline
  flt_no
  craft
  bort
  scd_in   #формат даты: dd.mm.yyyy hh:nn
  est_in   #формат даты: dd.mm.yyyy hh:nn
  act_in   #формат даты: dd.mm.yyyy hh:nn
  airp
  scd_out  #формат даты: dd.mm.yyyy hh:nn
  est_out  #формат даты: dd.mm.yyyy hh:nn
  act_out  #формат даты: dd.mm.yyyy hh:nn
  trip_type=п
  pr_tranzit=0
  pr_del=0
{          <dest>
            <modify/>
            <point_id>$(point_id)</point_id>
            <airp>$(airp)</airp>
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>
            <craft>$(craft)</craft>\
$(if $(eq $(bort) "") "" {
            <bort>$(bort)</bort>})\
$(if $(eq $(scd_in) "") "" {
            <scd_in>$(scd_in):00</scd_in>})\
$(if $(eq $(est_in) "") "" {
            <est_in>$(est_in):00</est_in>})\
$(if $(eq $(act_in) "") "" {
            <act_in>$(act_in):00</act_in>})\
$(if $(eq $(scd_out) "") "" {
            <scd_out>$(scd_out):00</scd_out>})\
$(if $(eq $(est_out) "") "" {
            <est_out>$(est_out):00</est_out>})\
$(if $(eq $(act_out) "") "" {
            <act_out>$(act_out):00</act_out>})
            <trip_type>$(trip_type)</trip_type>
            <pr_tranzit>$(pr_tranzit)</pr_tranzit>
            <pr_reg>1</pr_reg>
            <pr_del>$(pr_del)</pr_del>
          </dest>})

$(defmacro new_spp_point_last
  scd_in  #формат даты: dd.mm.yyyy hh:nn
  airp
{          <dest>
            <modify/>
            <airp>$(airp)</airp>\
$(if $(eq $(scd_in) "") "" {
            <scd_in>$(scd_in):00</scd_in>})
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>})

$(defmacro change_spp_point_last
  point_id
  scd_in   #формат даты: dd.mm.yyyy hh:nn
  est_in   #формат даты: dd.mm.yyyy hh:nn
  act_in   #формат даты: dd.mm.yyyy hh:nn
  airp
  pr_del=0
{          <dest>
            <modify/>
            <point_id>$(point_id)</point_id>
            <airp>$(airp)</airp>\
$(if $(eq $(scd_in) "") "" {
            <scd_in>$(scd_in):00</scd_in>})\
$(if $(eq $(est_in) "") "" {
            <est_in>$(est_in):00</est_in>})\
$(if $(eq $(act_in) "") "" {
            <act_in>$(act_in):00</act_in>})
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
            <pr_del>$(pr_del)</pr_del>
          </dest>})

$(defmacro NEW_SPP_FLIGHT_REQUEST
  dests
  lang=RU
  capture=off
{

!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <WriteDests>
      <data>
        <move_id>-2147483648</move_id>
        <canexcept>0</canexcept>
        <dests>
$(dests)
        </dests>
      </data>
    </WriteDests>
  </query>
</term>

})

$(defmacro CHANGE_SPP_FLIGHT_REQUEST
  any_point_id
  dests
  lang=RU
  capture=off
{

!! capture=$(capture)
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
   <WriteDests>
      <data>
        <move_id>$(get_move_id $(any_point_id))</move_id>
        <canexcept>0</canexcept>
        <dests>
$(dests)
        </dests>
      </data>
    </WriteDests>
  </query>
</term>

})

$(defmacro NEW_SPP_FLIGHT_ONE_LEG
  airline
  flt_no
  craft
  airp1
  scd_out1 #формат даты: dd.mm.yyyy hh:nn
  scd_in2  #формат даты: dd.mm.yyyy hh:nn
  airp2
  bort
{

$(NEW_SPP_FLIGHT_REQUEST
{ $(new_spp_point $(airline) $(flt_no) $(craft) $(bort) "" $(airp1) $(scd_out1))
  $(new_spp_point_last $(scd_in2) $(airp2)) })
})

$(defmacro CHANGE_SPP_FLIGHT_ONE_LEG
  point_dep
  act_out1 #формат даты: dd.mm.yyyy hh:nn
  pr_del
  airline
  flt_no
  craft
  airp1
  scd_out1 #формат даты: dd.mm.yyyy hh:nn
  scd_in2  #формат даты: dd.mm.yyyy hh:nn
  airp2
  bort
{

$(CHANGE_SPP_FLIGHT_REQUEST $(point_dep)
{ $(change_spp_point $(point_dep) $(airline) $(flt_no) $(craft) $(bort) "" "" "" $(airp1) $(scd_out1) "" $(act_out1) pr_del=$(pr_del))
  $(change_spp_point_last $(get_next_trip_point_id $(point_dep)) $(scd_in2) "" "" $(airp2)) })
})

$(defmacro NEW_SPP_FLIGHT_THREE_LEGS
  airline1
  flt_no1
  craft1
  bort1
  airp1
  scd_out1 #формат даты: dd.mm.yyyy hh:nn
  scd_in2  #формат даты: dd.mm.yyyy hh:nn
  airline2
  flt_no2
  craft2
  bort2
  airp2
  scd_out2 #формат даты: dd.mm.yyyy hh:nn
  scd_in3  #формат даты: dd.mm.yyyy hh:nn
  airline3
  flt_no3
  craft3
  bort3
  airp3
  scd_out3 #формат даты: dd.mm.yyyy hh:nn
  scd_in4  #формат даты: dd.mm.yyyy hh:nn
  airp4
{

{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteDests>
      <data>
        <move_id>-2147483648</move_id>
        <canexcept>0</canexcept>
        <dests>
          <dest>
            <modify/>
            <airp>$(airp1)</airp>
            <airline>$(airline1)</airline>
            <flt_no>$(flt_no1)</flt_no>
            <craft>$(craft1)</craft>\
$(if $(eq $(bort1) "") "" {
            <bort>$(bort1)</bort>})
            <scd_out>$(scd_out1):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp2)</airp>
            <airline>$(airline2)</airline>
            <flt_no>$(flt_no2)</flt_no>
            <craft>$(craft2)</craft>\
$(if $(eq $(bort2) "") "" {
            <bort>$(bort2)</bort>})
            <scd_in>$(scd_in2):00</scd_in>
            <scd_out>$(scd_out2):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp3)</airp>
            <airline>$(airline3)</airline>
            <flt_no>$(flt_no3)</flt_no>
            <craft>$(craft3)</craft>\
$(if $(eq $(bort3) "") "" {
            <bort>$(bort3)</bort>})
            <scd_in>$(scd_in3):00</scd_in>
            <scd_out>$(scd_out3):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp4)</airp>
            <scd_in>$(scd_in4):00</scd_in>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}

})

