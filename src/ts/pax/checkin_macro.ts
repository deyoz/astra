$(defmacro LOAD_PAX_BY_GRP_ID
    point_dep
    grp_id
    lang=RU
    capture=off
{
!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <grp_id>$(grp_id)</grp_id>
    </TCkinLoadPax>
  </query>
</term>

}) #end-of-macro

#########################################################################################

$(defmacro LOAD_PAX_BY_REG_NO
    point_dep
    reg_no
    lang=RU
    capture=off
{
!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <reg_no>$(reg_no)</reg_no>
    </TCkinLoadPax>
  </query>
</term>

}) #end-of-macro

#########################################################################################

$(defmacro LOAD_PAX_BY_PAX_ID
    point_dep
    pax_id
    lang=RU
    capture=off
{
!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinLoadPax>
      <point_id>$(point_dep)</point_id>
      <pax_id>$(pax_id)</pax_id>
    </TCkinLoadPax>
  </query>
</term>

}) #end-of-macro

#########################################################################################

$(defmacro NEW_CHECKIN_SEGMENT
  point_dep
  point_arv
  airp_dep
  airp_arv
  passengers
  class=Э
  status=K
{        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class>$(get_elem_id etClass $(class))</class>
          <status>$(status)</status>
          <wl_type/>
$(passengers)
        </segment>}
)

$(defmacro CHANGE_CHECKIN_SEGMENT
  point_dep
  point_arv
  airp_dep
  airp_arv
  grp_id
  grp_tid
  passengers
  class=Э
{        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class>$(get_elem_id etClass $(class))</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(grp_tid)</tid>
$(passengers)
        </segment>}
)

$(defmacro TRANSFER_SEGMENT
  airline
  flt_no
  suffix
  local_date
  airp_dep
  airp_arv
{        <segment>
          <airline>$(get_elem_id etAirline $(airline))</airline>
          <flt_no>$(flt_no)</flt_no>\
$(if $(eq $(suffix) "") {
          <suffix/>} {
          <suffix>$(suffix)</suffix>})
          <local_date>$(local_date)</local_date>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
        </segment>}
)

#########################################################################################
### возможность регистрации группы из любого кол-ва участников в секции passengers

$(defmacro NEW_CHECKIN_REQUEST
  point_dep
  point_arv
  airp_dep
  airp_arv
  passengers
  hall=777
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
$(NEW_CHECKIN_SEGMENT $(point_dep) $(point_arv) $(airp_dep) $(airp_arv) $(passengers))
      </segments>
      <hall>$(hall)</hall>
    </TCkinSavePax>
  </query>
</term>}

}) #end defmacro NEW_CHECKIN_REQUEST

$(defmacro NEW_TCHECKIN_REQUEST
  transfer
  segments
  hall=777
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>\
$(if $(eq $(transfer) "") {
    <transfer/>} {
    <transfer>
$(transfer)
    </transfer>})
      <segments>
$(segments)
      </segments>
      <hall>$(hall)</hall>
    </TCkinSavePax>
  </query>
</term>

})

#########################################################################################
### возможность записи изменений по любому кол-ву участников в секции passengers

$(defmacro CHANGE_CHECKIN_REQUEST
  point_dep
  point_arv
  airp_dep
  airp_arv
  grp_id
  grp_tid
  passengers
  hall=777
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
$(CHANGE_CHECKIN_SEGMENT $(point_dep) $(point_arv) $(airp_dep) $(airp_arv) $(grp_id) $(grp_tid) $(passengers))
      </segments>
      <hall>$(hall)</hall>
      <bag_refuse/>
    </TCkinSavePax>
  </query>
</term>}

}) #end defmacro CHANGE_CHECKIN_REQUEST

$(defmacro CHANGE_TCHECKIN_REQUEST
  segments
  bags_tags_etc
  hall=777
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
$(segments)
      </segments>
      <hall>$(hall)</hall>
      <bag_refuse/>
$(bags_tags_etc)
    </TCkinSavePax>
  </query>
</term>

})

$(defmacro NEW_CHECKIN_2982425618100
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>ДМИТРИЕВА</surname>
    <name>ЮЛИЯ АЛЕКСАНДРОВНА</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ДМИТРИЕВА</surname>
      <first_name>ЮЛИЯ АЛЕКСАНДРОВНА</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618100
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>ДМИТРИЕВА</surname>
    <name>ЮЛИЯ АЛЕКСАНДРОВНА</name>
    <pers_type>ВЗ</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no>2982425618100</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0310526187</no>
      <nationality>RUS</nationality>
      <birth_date>23.08.1985 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>ДМИТРИЕВА</surname>
      <first_name>ЮЛИЯ АЛЕКСАНДРОВНА</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(pax_tid)</tid>}
)

$(defmacro NEW_CHECKIN_2982425618102
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>ЧАРКОВ</surname>
    <name>МИХАИЛ ГЕННАДЬЕВИЧ</name>
    <pers_type>РБ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ЧАРКОВ</surname>
      <first_name>МИХАИЛ ГЕННАДЬЕВИЧ</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618102
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>ЧАРКОВ</surname>
    <name>МИХАИЛ ГЕННАДЬЕВИЧ</name>
    <pers_type>РБ</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no>2982425618102</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ЧАРКОВ</surname>
      <first_name>МИХАИЛ ГЕННАДЬЕВИЧ</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(pax_tid)</tid>}
)

$(defmacro NEW_CHECKIN_2982425618101
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>ЧАРКОВ</surname>
    <name>НИКОЛАЙ</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ЧАРКОВ</surname>
      <first_name>МИХАИЛ ГЕННАДЬЕВИЧ</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618101
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>ЧАРКОВ</surname>
    <name>НИКОЛАЙ</name>
    <pers_type>РМ</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no>2982425618101</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>ЧАРКОВ</surname>
      <first_name>МИХАИЛ ГЕННАДЬЕВИЧ</first_name>
    </document>
    <subclass>Ф</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(pax_tid)</tid>}
)

$(defmacro NEW_CHECKIN_2986145143703
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>VASILIADI</surname>
    <name>KSENIYA VALEREVNA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no>6Г</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145143703</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RUS</issue_country>
      <no>0307611933</no>
      <nationality>RUS</nationality>
      <birth_date>13.09.1984 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>VASILIADI</surname>
      <first_name>KSENIYA VALEREVNA</first_name>
    </document>
    <subclass>Л</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145143704
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>CHEKMAREV</surname>
    <name>RONALD</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145143704</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RUS</issue_country>
      <no>VАГ815247</no>
      <nationality>RUS</nationality>
      <birth_date>29.01.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>CHEKMAREV</surname>
      <first_name>RONALD KONSTANTINOVICH</first_name>
    </document>
    <subclass>Л</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145134262
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>STIPIDI</surname>
    <name>ANGELINA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no>11Б</seat_no>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145134262</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>0305555064</no>
      <nationality>RU</nationality>
      <birth_date>23.07.1982 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>STIPIDI</surname>
      <first_name>ANGELINA</first_name>
    </document>
    <subclass>В</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145134263
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>AKOPOVA</surname>
    <name>OLIVIIA</name>
    <pers_type>РМ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>0</seats>
    <ticket_no>2986145134263</ticket_no>
    <coupon_no>2</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>F</type>
      <issue_country>RU</issue_country>
      <no>VIAG519994</no>
      <nationality>RU</nationality>
      <birth_date>22.08.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>AKOPOVA</surname>
      <first_name>OLIVIIA</first_name>
    </document>
    <subclass>В</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145212943
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>VERGUNOV</surname>
    <name>VASILII LEONIDOVICH</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2986145212943</ticket_no>
    <coupon_no>1</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>0305984920</no>
      <nationality>RU</nationality>
      <birth_date>04.11.1960 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>VERGUNOV</surname>
      <first_name>VASILII LEONIDOVICH</first_name>
    </document>
    <subclass>Ю</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_NOREC
  surname
  name
  pers_type=ВЗ
  seats=1
  subclass=Э
  document
{   <pax_id/>
   <surname>$(surname)</surname>
   <name>$(name)</name>
   <pers_type>$(pers_type)</pers_type>
   <seat_no/>
   <preseat_no/>
   <seat_type/>
   <seats>$(seats)</seats>
   <ticket_no/>
   <coupon_no/>
   <ticket_rem/>
   <ticket_confirm>0</ticket_confirm>
$(if $(eq $(document) "") <document/> $(document))
   <doco/>
   <addresses/>
   <subclass>$(subclass)</subclass>
   <bag_pool_num/>
   <transfer/>
   <rems/>
   <fqt_rems/>
   <norms/>}
)

$(defmacro NEW_CHECKIN_2982410821479
  pax_id
  coupon_no
  transfer_subclass
{    <pax_id>$(pax_id)</pax_id>
    <surname>KOTOVA</surname>
    <name>IRINA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982410821479</ticket_no>
    <coupon_no>$(coupon_no)</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>7774441110</no>
      <nationality>RU</nationality>
      <birth_date>01.05.1976 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KOTOVA</surname>
      <first_name>IRINA</first_name>
    </document>
    <subclass>Э</subclass>
    <bag_pool_num/>\
$(if $(eq $(transfer_subclass) "") {
    <transfer/>} {
    <transfer>
      <segment>
         <subclass>$(get_elem_id etSubcls $(transfer_subclass))</subclass>
      </segment>
    </transfer>})}
)

$(defmacro CHANGE_CHECKIN_2982410821479
  pax_id
  pax_tid
  coupon_no
  ticket_confirm=1
  bag_pool_num
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>KOTOVA</surname>
    <name>IRINA</name>
    <pers_type>ВЗ</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no>2982410821479</ticket_no>
    <coupon_no>$(coupon_no)</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>7774441110</no>
      <nationality>RU</nationality>
      <birth_date>01.05.1976 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>KOTOVA</surname>
      <first_name>IRINA</first_name>
    </document>\
$(if $(eq $(bag_pool_num) "") {
    <bag_pool_num/>} {
    <bag_pool_num>$(bag_pool_num)</bag_pool_num>})
    <subclass>Э</subclass>
    <tid>$(pax_tid)</tid>}
)


$(defmacro NEW_CHECKIN_2982410821480
  pax_id
  coupon_no
  transfer_subclass
{    <pax_id>$(pax_id)</pax_id>
    <surname>MOTOVA</surname>
    <name>IRINA</name>
    <pers_type>ВЗ</pers_type>
    <seat_no/>
    <preseat_no/>
    <seat_type/>
    <seats>1</seats>
    <ticket_no>2982410821480</ticket_no>
    <coupon_no>$(coupon_no)</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>0</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>7774441110</no>
      <nationality>RU</nationality>
      <birth_date>01.05.1976 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>MOTOVA</surname>
      <first_name>IRINA</first_name>
    </document>
    <subclass>Э</subclass>
    <bag_pool_num/>\
$(if $(eq $(transfer_subclass) "") {
    <transfer/>} {
    <transfer>
      <segment>
         <subclass>$(get_elem_id etSubcls $(transfer_subclass))</subclass>
      </segment>
    </transfer>})}
)

$(defmacro CHANGE_CHECKIN_2982410821480
  pax_id
  pax_tid
  coupon_no
  ticket_confirm=1
  bag_pool_num
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>MOTOVA</surname>
    <name>IRINA</name>
    <pers_type>ВЗ</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no>2982410821480</ticket_no>
    <coupon_no>$(coupon_no)</coupon_no>
    <ticket_rem>TKNE</ticket_rem>
    <ticket_confirm>$(ticket_confirm)</ticket_confirm>
    <document>
      <type>P</type>
      <issue_country>RU</issue_country>
      <no>7774441110</no>
      <nationality>RU</nationality>
      <birth_date>01.05.1976 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>F</gender>
      <surname>MOTOVA</surname>
      <first_name>IRINA</first_name>
    </document>\
$(if $(eq $(bag_pool_num) "") {
    <bag_pool_num/>} {
    <bag_pool_num>$(bag_pool_num)</bag_pool_num>})
    <subclass>Э</subclass>
    <tid>$(pax_tid)</tid>}
)

$(defmacro SVC_REQUEST_2982410821479
  pax_id
  display_id
  segment1_props
  segment2_props
  segment2_props2
{    <passenger id=\"$(pax_id)\" surname=\"КОТОВА\" name=\"ИРИНА\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"female\">
      <document number=\"7774441110\" expiration_date=\"$(date_format %Y-%m-%d +1y)\" country=\"RUS\"/>
      <segment id=\"0\" $(segment1_props) subclass=\"Y\">
        <ticket number=\"2982410821479\" coupon_num=\"1\" display_id=\"$(display_id)\"/>
        <recloc crs=\"DT\">04VSFC</recloc>
        <recloc crs=\"UT\">054C82</recloc>
      </segment>\
$(if $(eq $(segment2_props) "") "" {
      <segment id=\"1\" $(segment2_props) subclass=\"Y\">
        <ticket number=\"2982410821479\" coupon_num=\"2\" display_id=\"$(display_id)\"/>
        <recloc crs=\"DT\">04VSFC</recloc>
        <recloc crs=\"UT\">054C82</recloc>
      </segment>})\
$(if $(eq $(segment2_props2) "") "" {
      <segment id=\"1\" $(segment2_props2) subclass=\"Y\">
        <ticket number=\"2982410821479\" coupon_num=\"2\" display_id=\"$(display_id)\"/>
      </segment>})
    </passenger>}
)

$(defmacro SVC_REQUEST_2982410821480
  pax_id
  display_id
  segment1_props
  segment2_props
  segment2_props2
{    <passenger id=\"$(pax_id)\" surname=\"МОТОВА\" name=\"ИРИНА\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"female\">
      <document number=\"7774441110\" expiration_date=\"$(date_format %Y-%m-%d +1y)\" country=\"RUS\"/>
      <segment id=\"0\" $(segment1_props) subclass=\"Y\">
        <ticket number=\"2982410821480\" coupon_num=\"1\" display_id=\"$(display_id)\"/>
        <recloc crs=\"DT\">04VSFC</recloc>
        <recloc crs=\"UT\">054C82</recloc>
      </segment>\
$(if $(eq $(segment2_props) "") "" {
      <segment id=\"1\" $(segment2_props) subclass=\"Y\">
        <ticket number=\"2982410821480\" coupon_num=\"2\" display_id=\"$(display_id)\"/>
        <recloc crs=\"DT\">04VSFC</recloc>
        <recloc crs=\"UT\">054C82</recloc>
      </segment>})\
$(if $(eq $(segment2_props2) "") "" {
      <segment id=\"1\" $(segment2_props2) subclass=\"Y\">
        <ticket number=\"2982410821480\" coupon_num=\"2\" display_id=\"$(display_id)\"/>
      </segment>})
    </passenger>}
)


