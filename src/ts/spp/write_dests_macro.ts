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
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>
            <craft>$(craft)</craft>\
$(if $(eq $(bort) "") "" {
            <bort>$(bort)</bort>})
            <scd_out>$(scd_out1):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp2)</airp>
            <scd_in>$(scd_in2):00</scd_in>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}

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

{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='sopp' ver='1' opr='PIKE' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <WriteDests>
      <data>
        <move_id>$(get_move_id $(point_dep))</move_id>
        <canexcept>0</canexcept>
        <dests>
          <dest>
            <modify/>
            <point_id>$(point_dep)</point_id>
            <point_num>0</point_num>
            <airp>$(airp1)</airp>
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>
            <craft>$(craft)</craft>\
$(if $(eq $(bort) "") "" {
            <bort>$(bort)</bort>})
            <scd_out>$(scd_out1):00</scd_out>\
$(if $(eq $(act_out1) "") "" {
            <act_out>$(act_out1):00</act_out>})
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>1</pr_reg>
            <pr_del>$(pr_del)</pr_del>
          </dest>
          <dest>
            <modify/>
            <point_id>$(get_next_trip_point_id $(point_dep))</point_id>
            <point_num>1</point_num>
            <first_point>$(point_dep)</first_point>
            <airp>$(airp2)</airp>
            <scd_in>$(scd_in2):00</scd_in>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
            <pr_del>$(pr_del)</pr_del>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}

})

$(defmacro NEW_SPP_FLIGHT_TWO_LEGS
  airline
  flt_no
  craft
  airp1
  scd_out1 #формат даты: dd.mm.yyyy hh:nn
  scd_in2  #формат даты: dd.mm.yyyy hh:nn
  airp2
  scd_out2 #формат даты: dd.mm.yyyy hh:nn
  scd_in3  #формат даты: dd.mm.yyyy hh:nn
  airp3
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
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>
            <craft>$(craft)</craft>
            <scd_out>$(scd_out1):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp2)</airp>
            <airline>$(airline)</airline>
            <flt_no>$(flt_no)</flt_no>
            <craft>$(craft)</craft>
            <scd_in>$(scd_in2):00</scd_in>
            <scd_out>$(scd_out2):00</scd_out>
            <trip_type>п</trip_type>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
          <dest>
            <modify/>
            <airp>$(airp3)</airp>
            <scd_in>$(scd_in3):00</scd_in>
            <pr_tranzit>0</pr_tranzit>
            <pr_reg>0</pr_reg>
          </dest>
        </dests>
      </data>
    </WriteDests>
  </query>
</term>}

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

