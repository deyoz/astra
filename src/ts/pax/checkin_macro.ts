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

$(defmacro LOAD_GRP_BAG_TYPES_OUTDATED
  num
  grp_id
  lang=RU
{
!! capture=on err=ignore
$(cache PIKE $(lang) GRP_BAG_TYPES$(num) "" ""
        $(cache_sql_param bag_types_id integer $(grp_id))
)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>01</col>
          <col>Негабарит по размерам</col>
          <col>Oversize</col>
          <col>Вещи пассажира, независимо от их наименования и назначения, габариты которых в упакованном виде превышают установленные размеры</col>
          <col>Passengers properties irrespective of their naming and purpose, packed dimensions of which exceed allowances</col>
          <col/>
          <col>1</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>02</col>
          <col>Негабарит по весу</col>
          <col>Overweight</col>
          <col>Вещи пассажира, независимо от их наименования и назначения массой одного места более установленного веса</col>
          <col>Passengers properties irrespective of their naming and purpose, exceeding weight allowance per piece</col>
          <col/>
          <col>2</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>03</col>
          <col>Аппаратура</col>
          <col>Equipment</col>
          <col>Теле-, радио-, видео-, аудио-, кино-, фотоаппаратура, электробытовая техника, компьютеры, массой одного места свыше установленного веса</col>
          <col>Tele-, radio, video, audio, cinema, photo equipment, consumer electronics, computers, exceeding weight allowance per piece</col>
          <col/>
          <col>3</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04</col>
          <col>Живность</col>
          <col>Living creatures</col>
          <col>Животные (домашние и дикие), птицы, рыбы, пчелы и другая живность (за исключением собак-поводырей, сопровождающих слепых).</col>
          <col>Animals (domestic and wild), birds, fish, bees and other living creatures (except for guide-dogs accompanying blind persons).</col>
          <col/>
          <col>4</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>05</col>
          <col>Цветы, саженцы, зелень</col>
          <col>Flowers, saplings, greenery</col>
          <col>Цветы, саженцы растений, пищевая зелень (зеленый лук, петрушка, щавель, укроп, сельдерей и т.д.) массой свыше установленного веса</col>
          <col>Flowers, plant saplings, edible greens (spring onion, parsley, sorrel, dill, celery etc.) exceeding weight allowance</col>
          <col/>
          <col>5</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>06</col>
          <col>Суш. растения, ветви</col>
          <col>Dried plants, branches</col>
          <col>Сушеные растения, ветви деревьев и кустарника массой свыше установленного веса</col>
          <col>Dried plants, tree and bush branches exceeding weight allowance</col>
          <col/>
          <col>6</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>07</col>
          <col>Овощи, ягода</col>
          <col>Vegetables, berries</col>
          <col>Овощи, ягода массой свыше установленного веса</col>
          <col>Vegetables, berries exceeding weight allowance</col>
          <col/>
          <col>7</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>08</col>
          <col>Служебная почта</col>
          <col>Official mail</col>
          <col>Фельдъегерская корреспонденция и корреспонденция спецсвязи.</col>
          <col>Courier service and special communication correspondence</col>
          <col/>
          <col>8</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>09</col>
          <col>Меховые изделия</col>
          <col>Furs</col>
          <col>Меховые изделия.</col>
          <col>Furs</col>
          <col/>
          <col>9</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>10</col>
          <col>Муз. инструменты</col>
          <col>Mus. instruments</col>
          <col>Музыкальные инструменты массой одного места свыше установленного веса</col>
          <col>Musical instruments exceeding weight allowance per piece</col>
          <col/>
          <col>10</col>
          <col>$(grp_id)</col>
        </row>
      </rows>

})

$(defmacro LOAD_GRP_BAG_TYPES
  grp_id
  lang=RU
{
!! capture=on err=ignore
$(cache PIKE $(lang) GRP_BAG_TYPES "" ""
        $(cache_sql_param grp_id integer $(grp_id))
)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col> </col>
          <col>Обычный багаж или р/кладь</col>
          <col>Regular or hand bagg.</col>
          <col/>
          <col/>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>01</col>
          <col>Негабарит по размерам</col>
          <col>Oversize</col>
          <col>Вещи пассажира, независимо от их наименования и назначения, габариты которых в упакованном виде превышают установленные размеры</col>
          <col>Passengers properties irrespective of their naming and purpose, packed dimensions of which exceed allowances</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>02</col>
          <col>Негабарит по весу</col>
          <col>Overweight</col>
          <col>Вещи пассажира, независимо от их наименования и назначения массой одного места более установленного веса</col>
          <col>Passengers properties irrespective of their naming and purpose, exceeding weight allowance per piece</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>03</col>
          <col>Аппаратура</col>
          <col>Equipment</col>
          <col>Теле-, радио-, видео-, аудио-, кино-, фотоаппаратура, электробытовая техника, компьютеры, массой одного места свыше установленного веса</col>
          <col>Tele-, radio, video, audio, cinema, photo equipment, consumer electronics, computers, exceeding weight allowance per piece</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>04</col>
          <col>Живность</col>
          <col>Living creatures</col>
          <col>Животные (домашние и дикие), птицы, рыбы, пчелы и другая живность (за исключением собак-поводырей, сопровождающих слепых).</col>
          <col>Animals (domestic and wild), birds, fish, bees and other living creatures (except for guide-dogs accompanying blind persons).</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>05</col>
          <col>Цветы, саженцы, зелень</col>
          <col>Flowers, saplings, greenery</col>
          <col>Цветы, саженцы растений, пищевая зелень (зеленый лук, петрушка, щавель, укроп, сельдерей и т.д.) массой свыше установленного веса</col>
          <col>Flowers, plant saplings, edible greens (spring onion, parsley, sorrel, dill, celery etc.) exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>06</col>
          <col>Суш. растения, ветви</col>
          <col>Dried plants, branches</col>
          <col>Сушеные растения, ветви деревьев и кустарника массой свыше установленного веса</col>
          <col>Dried plants, tree and bush branches exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>07</col>
          <col>Овощи, ягода</col>
          <col>Vegetables, berries</col>
          <col>Овощи, ягода массой свыше установленного веса</col>
          <col>Vegetables, berries exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>08</col>
          <col>Служебная почта</col>
          <col>Official mail</col>
          <col>Фельдъегерская корреспонденция и корреспонденция спецсвязи.</col>
          <col>Courier service and special communication correspondence</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>09</col>
          <col>Меховые изделия</col>
          <col>Furs</col>
          <col>Меховые изделия.</col>
          <col>Furs</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>10</col>
          <col>Муз. инструменты</col>
          <col>Mus. instruments</col>
          <col>Музыкальные инструменты массой одного места свыше установленного веса</col>
          <col>Musical instruments exceeding weight allowance per piece</col>
        </row>
      </rows>

})

$(defmacro LOAD_GRP_RFISC_OUTDATED
  num
  grp_id
  lang=RU
{

$(cache PIKE RU RFISC_BAG_PROPS $(cache_iface_ver RFISC_BAG_PROPS) ""
  insert airline:$(get_elem_id etAirline UT)
         code:0GP
         rem_code_lci:
         rem_code_ldm:
         priority:2
         min_weight:1
         max_weight:23
  insert airline:$(get_elem_id etAirline UT)
         code:0DD
         rem_code_lci:
         rem_code_ldm:
         priority:1
         min_weight:1
         max_weight:10)

$(dump_table RFISC_BAG_PROPS)

!! capture=on err=ignore
$(cache PIKE $(lang) GRP_RFISC$(num) "" ""
        $(cache_sql_param bag_types_id integer $(grp_id))
)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>0DD</col>
          <col>1е лыжи сноуборд до 20кг</col>
          <col>1st ski snowboard upto 20kg</col>
          <col>0</col>
          <col>1</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0GP</col>
          <col>до 50ф 23кг и до 80д 203см</col>
          <col>upto50lb 23kg and80li 203lcm</col>
          <col>0</col>
          <col>2</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>03C</col>
          <col>место баг рм до10кг 55х40х20см</col>
          <col>pc of bag for inf upto 10kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04J</col>
          <col>ноутбук или сумка до 85 см</col>
          <col>laptop or handbag up to 85 lcm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04U</col>
          <col>спорт огнестроруж до 50ф23кг</col>
          <col>firearms up to 23kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04V</col>
          <col>огнестрельное оружие до 32кг</col>
          <col>firearms up to 32kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>07F</col>
          <col>1й компл лыжного снаряж до23кг</col>
          <col>1st ski equipment upto 23kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>08A</col>
          <col>ручная кладь</col>
          <col>carry on baggage</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0AA</col>
          <col>предоплаченный багаж</col>
          <col>pre paid baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0BS</col>
          <col>животное в багаж отдел до 23кг</col>
          <col>pet in hold</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0BT</col>
          <col>животное в салоне до10кг 115см</col>
          <col>pet in cabin weight upto 10kg</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C2</col>
          <col>багаж не более 20кг 203см</col>
          <col>upto50lb 23kg and62li 158lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C4</col>
          <col>багаж до 55ф 25кг</col>
          <col>upto55lb 25kg baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C5</col>
          <col>багаж 21 30кг не более 203см</col>
          <col>bag 21 30kg upto 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0CC</col>
          <col>первое зарегистрир место</col>
          <col>checked bag first</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0CZ</col>
          <col>багаж до 22ф 10кг</col>
          <col>upto 22lb 10kg baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0DG</col>
          <col>плата за сверх нормативн.багаж</col>
          <col>weight system charge</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0EB</col>
          <col>лыж ботин шлем с лыжами сноубд</col>
          <col>skibootshelmet with skisnowbrd</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0F4</col>
          <col>детская коляска</col>
          <col>stroller or pushchair</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0FB</col>
          <col>до 50ф 23кг и более 80д 203см</col>
          <col>upto50lb 23kg over80li 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0FN</col>
          <col>место багажа 30кг 203см</col>
          <col>pc of xbag upto 30kg 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0GO</col>
          <col>до 50ф 23кг и до 62д 158см</col>
          <col>upto50lb 23kg and62li 158lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0H3</col>
          <col>доп место вес 33-50кг</col>
          <col>excess pc and weight 33-50kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IA</col>
          <col>специальный багажный сбор</col>
          <col>baggage special charge</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IF</col>
          <col>превыш по весу и кол мест</col>
          <col>excess weight and piece</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IJ</col>
          <col>багаж не более 10кг 55x40x25cm</col>
          <col>bag upto 10 kg 55x40x25cm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0KJ</col>
          <col>1й чехол с 2 хок клюшками 20кг</col>
          <col>1stbagwith2icehockeystick 20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0KN</col>
          <col>1е хоккейное снаряжен до 20кг</col>
          <col>1st hockey eqpmt upto 20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0L1</col>
          <col>рыболовные снасти до 44ф 20кг</col>
          <col>fishing equipment upto44lb20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0L5</col>
          <col>ручная кладь до 5кг 40х30х20см</col>
          <col>cabin bag upto 5kg 40x30x20cm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0M5</col>
          <col>руч кладь 10кг 22ф до 39д100см</col>
          <col>carry10kg 22lb upto39li 100lcm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0M6</col>
          <col>место багажа вес от24кг до32кг</col>
          <col>pc of bag from 23kg upto 32kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0MJ</col>
          <col>ручная кладь до10кг 55х40х20см</col>
          <col>cabin bag upto 10kg 55x40x20cm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0MN</col>
          <col>домашнее животное в салоне</col>
          <col>pet in cabin</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAC</col>
          <col>одно место багажа в бизнес кл</col>
          <col>pc of bag in business class</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAM</col>
          <col>1 км баг в эконом комфорт кл</col>
          <col>pc of bag in economy comfort</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAY</col>
          <col>одно место багажа в эконом кл</col>
          <col>pc of bag in economy class</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AMM</col>
          <col>патроны до5кг с 1ед оруж беспл</col>
          <col>ammun upto 5kg with arms free</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AMP</col>
          <col>патроны до 5кг</col>
          <col>ammunition upto 5kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>PLB</col>
          <col>обьедин баг в 1pnr до30кг203см</col>
          <col>pool of bag in 1 pnr 30kg203cm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
      </rows>

})

$(defmacro LOAD_GRP_RFISC
  grp_id
  lang=RU
{
!! capture=on err=ignore
$(cache PIKE $(lang) GRP_RFISC "" ""
        $(cache_sql_param grp_id integer $(grp_id))
)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>03C</col>
          <col>место баг рм до10кг 55х40х20см</col>
          <col>pc of bag for inf upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>04J</col>
          <col>ноутбук или сумка до 85 см</col>
          <col>laptop or handbag up to 85 lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>04U</col>
          <col>спорт огнестроруж до 50ф23кг</col>
          <col>firearms up to 23kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>04V</col>
          <col>огнестрельное оружие до 32кг</col>
          <col>firearms up to 32kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>07F</col>
          <col>1й компл лыжного снаряж до23кг</col>
          <col>1st ski equipment upto 23kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>08A</col>
          <col>ручная кладь</col>
          <col>carry on baggage</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0AA</col>
          <col>предоплаченный багаж</col>
          <col>pre paid baggage</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0BS</col>
          <col>животное в багаж отдел до 23кг</col>
          <col>pet in hold</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0BT</col>
          <col>животное в салоне до10кг 115см</col>
          <col>pet in cabin weight upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0C2</col>
          <col>багаж не более 20кг 203см</col>
          <col>upto50lb 23kg and62li 158lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0C4</col>
          <col>багаж до 55ф 25кг</col>
          <col>upto55lb 25kg baggage</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0C5</col>
          <col>багаж 21 30кг не более 203см</col>
          <col>bag 21 30kg upto 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0CC</col>
          <col>первое зарегистрир место</col>
          <col>checked bag first</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0CZ</col>
          <col>багаж до 22ф 10кг</col>
          <col>upto 22lb 10kg baggage</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0DD</col>
          <col>1е лыжи сноуборд до 20кг</col>
          <col>1st ski snowboard upto 20kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0DG</col>
          <col>плата за сверх нормативн.багаж</col>
          <col>weight system charge</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0EB</col>
          <col>лыж ботин шлем с лыжами сноубд</col>
          <col>skibootshelmet with skisnowbrd</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0F4</col>
          <col>детская коляска</col>
          <col>stroller or pushchair</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0FB</col>
          <col>до 50ф 23кг и более 80д 203см</col>
          <col>upto50lb 23kg over80li 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0FN</col>
          <col>место багажа 30кг 203см</col>
          <col>pc of xbag upto 30kg 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0GO</col>
          <col>до 50ф 23кг и до 62д 158см</col>
          <col>upto50lb 23kg and62li 158lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0GP</col>
          <col>до 50ф 23кг и до 80д 203см</col>
          <col>upto50lb 23kg and80li 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0H3</col>
          <col>доп место вес 33-50кг</col>
          <col>excess pc and weight 33-50kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0IA</col>
          <col>специальный багажный сбор</col>
          <col>baggage special charge</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0IF</col>
          <col>превыш по весу и кол мест</col>
          <col>excess weight and piece</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0IJ</col>
          <col>багаж не более 10кг 55x40x25cm</col>
          <col>bag upto 10 kg 55x40x25cm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0KJ</col>
          <col>1й чехол с 2 хок клюшками 20кг</col>
          <col>1stbagwith2icehockeystick 20kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0KN</col>
          <col>1е хоккейное снаряжен до 20кг</col>
          <col>1st hockey eqpmt upto 20kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0L1</col>
          <col>рыболовные снасти до 44ф 20кг</col>
          <col>fishing equipment upto44lb20kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0L5</col>
          <col>ручная кладь до 5кг 40х30х20см</col>
          <col>cabin bag upto 5kg 40x30x20cm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0M5</col>
          <col>руч кладь 10кг 22ф до 39д100см</col>
          <col>carry10kg 22lb upto39li 100lcm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0M6</col>
          <col>место багажа вес от24кг до32кг</col>
          <col>pc of bag from 23kg upto 32kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0MJ</col>
          <col>ручная кладь до10кг 55х40х20см</col>
          <col>cabin bag upto 10kg 55x40x20cm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>0MN</col>
          <col>домашнее животное в салоне</col>
          <col>pet in cabin</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>AAC</col>
          <col>одно место багажа в бизнес кл</col>
          <col>pc of bag in business class</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>AAM</col>
          <col>1 км баг в эконом комфорт кл</col>
          <col>pc of bag in economy comfort</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>AAY</col>
          <col>одно место багажа в эконом кл</col>
          <col>pc of bag in economy class</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>AMM</col>
          <col>патроны до5кг с 1ед оруж беспл</col>
          <col>ammun upto 5kg with arms free</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>AMP</col>
          <col>патроны до 5кг</col>
          <col>ammunition upto 5kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>C</col>
          <col>PLB</col>
          <col>обьедин баг в 1pnr до30кг203см</col>
          <col>pool of bag in 1 pnr 30kg203cm</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>029</col>
          <col>резервир места рядом</col>
          <col>резервир места рядом</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>02O</col>
          <col>персональные развлечения</col>
          <col>personal entertainment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>03P</col>
          <col>приоритетная регистрация</col>
          <col>priority check in</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>042</col>
          <col>повышение класса обслуживания</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>04U</col>
          <col>спорт огнестроруж до 50ф23кг</col>
          <col>firearms up to 23kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>060</col>
          <col>повышение класса обслуживания</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>061</col>
          <col>апгрейд сопровожд младенца</col>
          <col>upgrade of accomp infant</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>062</col>
          <col>бизнес на первый</col>
          <col>business to first</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0AG</col>
          <col>вип зал</col>
          <col>executive lounge</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0AI</col>
          <col>завтрак</col>
          <col>breakfast</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0AN</col>
          <col>вегетарианский обед</col>
          <col>vegetarian dinner</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0AR</col>
          <col>вегетарианск сэндвич с соусом</col>
          <col>вегетарианск сэндвич с соусом</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0B3</col>
          <col>питание</col>
          <col>meal</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0B5</col>
          <col>предварительный выбор места</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BD</col>
          <col>рубашка поло взр large</col>
          <col>adult polo shirt large</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BG</col>
          <col>дорожная страховка</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BH</col>
          <col>несопровождаемый ребенок</col>
          <col>unaccompanied minor</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BJ</col>
          <col>повышение класса обслуживания</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BT</col>
          <col>животное в салоне до10кг 115см</col>
          <col>pet in cabin weight upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0BX</col>
          <col>бизнес зал</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0CC</col>
          <col>первое зарегистрир место</col>
          <col>checked bag first</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0CL</col>
          <col>доступ в интернет</col>
          <col>internet access</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0F4</col>
          <col>детская коляска</col>
          <col>stroller or pushchair</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0G9</col>
          <col>сэндвич с копченой грудкой</col>
          <col>сэндвич с копченой грудкой</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0JB</col>
          <col>ружье с патронами</col>
          <col>ружье с патронами</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0L8</col>
          <col>bundle услуга</col>
          <col>bundle service</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0LE</col>
          <col>морепродукты 2</col>
          <col>seafood meal 2</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0LQ</col>
          <col>питание 4</col>
          <col>meal 4</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0M6</col>
          <col>место багажа вес от24кг до32кг</col>
          <col>pc of bag from 23kg upto 32kg</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>0MF</col>
          <col>обезжиренная пища</col>
          <col>low fat meal</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>ATX</col>
          <col>выбор предпочтительного места</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BAS</col>
          <col>предварительный выбор места</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BBG</col>
          <col>дорожная страховка</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BD1</col>
          <col>пакет услуг</col>
          <col>bundle service</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BD2</col>
          <col>пакет услуг 2</col>
          <col>bundle service 2</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BD3</col>
          <col>пакет услуг 3</col>
          <col>bundle service 3</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BD4</col>
          <col>пакет услуг 4</col>
          <col>bundle service 4</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BF1</col>
          <col>блинчики</col>
          <col>pancakes</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>BF2</col>
          <col>сырники</col>
          <col>curd cakes</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>CMF</col>
          <col>предварительный выбор места</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>CRD</col>
          <col>люлька</col>
          <col>cradle</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>LD1</col>
          <col>шашлык куриный</col>
          <col>grilled chicken</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>PN1</col>
          <col>чай черный или зеленый</col>
          <col>black or green tea</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>SP1</col>
          <col>смена мест</col>
          <col>change</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>SPF</col>
          <col>выбор места</col>
          <col>seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>ST1</col>
          <col>1ый салон с 2го по 4ый ряд</col>
          <col>1ый салон с 2го по 4ый ряд</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>ST2</col>
          <col>место с 2 по 4 ряд</col>
          <col>место с 2 по 4 ряд</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>STA</col>
          <col>место в 1ом ряду у выходов</col>
          <col>место в 1ом ряду у выходов</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>STR</col>
          <col>выбор места при регистрации</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>STW</col>
          <col>место у окна с 5 ряда</col>
          <col>место у окна с 5 ряда</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>F</col>
          <col>SW1</col>
          <col>сэндвич с говядиной</col>
          <col>beef sandwich</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>07E</col>
          <col>фиксация стоимости перевозки</col>
          <col>time to decide fee</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>0BB</col>
          <col>рубашка поло взр мал</col>
          <col>adult polo shirt small</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>0BK</col>
          <col>ваучер на перевозку</col>
          <col>voucher for travel</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>0BX</col>
          <col>бизнес зал</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>0L7</col>
          <col>сбор за изменение имени</col>
          <col>name change</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>997</col>
          <col>предоплата</col>
          <col>deposits down payments</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>DPS</col>
          <col>предоплата</col>
          <col>deposits down payments</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>RFP</col>
          <col>сбор за выдачу подтвержд докум</col>
          <col>confirmation docs issuance fee</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>M</col>
          <col>YYY</col>
          <col>справка по билету</col>
          <col>ticket notice</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>R</col>
          <col>0A2</col>
          <col>игнориров правил переоформ</col>
          <col>reissue override</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>R</col>
          <col>0A3</col>
          <col>игнорирование правил возврата</col>
          <col>refund override</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>R</col>
          <col>0A4</col>
          <col>reissue and refund override</col>
          <col>reissue and refund override</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>T</col>
          <col>0BG</col>
          <col>дорожная страховка</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>T</col>
          <col>0BX</col>
          <col>бизнес зал</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>ЮТ</col>
          <col>UT</col>
          <col>T</col>
          <col>0L7</col>
          <col>сбор за изменение имени</col>
          <col>name change</col>
        </row>
      </rows>

})
