$(defmacro CKIN_PAX_LIST_REQUEST
  point_dep
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <PaxList>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_dep)</point_id>
      <tripcounters>
        <fields>
          <class/>
        </fields>
      </tripcounters>
      <LoadForm/>
    </PaxList>
  </query>
</term>

})

$(defmacro CKIN_LIST_PAX
  pax_id
  reg_no
  surname
  name
  airp_arv
  last_trfer
  last_tckin_seg
  subclass
  seat_no
  seats
  pers_type
  bag_amount
  bag_weight
  rk_weight
  excess
  tags
  grp_id
  cl_grp_id
  hall_id=777
  point_arv
  user_id
  status_id
{      <pax>
        <pax_id>$(pax_id)</pax_id>
        <reg_no>$(reg_no)</reg_no>
        <surname>$(surname)</surname>
        <name>$(name)</name>
        <airp_arv>$(airp_arv)</airp_arv>\
$(if $(eq $(last_trfer) "") "" {
        <last_trfer>$(last_trfer)</last_trfer>})\
$(if $(eq $(last_tckin_seg) "") "" {
        <last_tckin_seg>$(last_tckin_seg)</last_tckin_seg>})
        <subclass>$(subclass)</subclass>\
$(if $(eq $(seat_no) "") {
        <seat_no/>} {
        <seat_no>$(seat_no)</seat_no>})\
$(if $(eq $(seats) "") "" {
        <seats>$(seats)</seats>})\
$(if $(eq $(pers_type) "") "" {
        <pers_type>$(pers_type)</pers_type>})\
$(if $(eq $(bag_amount) "") "" {
        <bag_amount>$(bag_amount)</bag_amount>})\
$(if $(eq $(bag_weight) "") "" {
        <bag_weight>$(bag_weight)</bag_weight>})\
$(if $(eq $(rk_weight) "") "" {
        <rk_weight>$(rk_weight)</rk_weight>})\
$(if $(eq $(excess) "") "" {
        <excess>$(excess)</excess>})\
$(if $(eq $(tags) "") "" {
        <tags>$(tags)</tags>})
        <grp_id>$(grp_id)</grp_id>
        <cl_grp_id>$(cl_grp_id)</cl_grp_id>
        <hall_id>$(hall_id)</hall_id>
        <point_arv>$(point_arv)</point_arv>
        <user_id>$(user_id)</user_id>\
$(if $(eq $(status_id) "") "" {
        <status_id>$(status_id)</status_id>})
      </pax>})

$(defmacro CKIN_LIST_BAG
  airp_arv
  last_trfer
  bag_amount
  bag_weight
  rk_weight
  excess
  tags
  grp_id
  hall_id=777
  point_arv
  user_id
{      <bag>
        <airp_arv>$(airp_arv)</airp_arv>\
$(if $(eq $(last_trfer) "") "" {
        <last_trfer>$(last_trfer)</last_trfer>})\
$(if $(eq $(bag_amount) "") "" {
        <bag_amount>$(bag_amount)</bag_amount>})\
$(if $(eq $(bag_weight) "") "" {
        <bag_weight>$(bag_weight)</bag_weight>})\
$(if $(eq $(rk_weight) "") "" {
        <rk_weight>$(rk_weight)</rk_weight>})\
$(if $(eq $(excess) "") "" {
        <excess>$(excess)</excess>})\
$(if $(eq $(tags) "") "" {
        <tags>$(tags)</tags>})
        <grp_id>$(grp_id)</grp_id>
        <hall_id>$(hall_id)</hall_id>
        <point_arv>$(point_arv)</point_arv>
        <user_id>$(user_id)</user_id>
      </bag>})

#########################################################################################

$(defmacro BRD_PAX_LIST_REQUEST
  point_dep
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='brd' ver='1' opr='PIKE' screen='BRDBUS.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <PaxList>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_dep)</point_id>
    </PaxList>
  </query>
</term>

})

#########################################################################################

$(defmacro CRS_LIST_REQUEST
  point_dep
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='prepreg' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <ViewCRSList>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_dep)</point_id>
    </ViewCRSList>
  </query>
</term>

})

#########################################################################################

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
  class=�
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

$(defmacro NEW_UNACCOMP_SEGMENT
  point_dep
  point_arv
  airp_dep
  airp_arv
  status=K
{        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class/>
          <status>$(status)</status>
          <wl_type/>
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
  class=�
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

$(defmacro CHANGE_UNACCOMP_SEGMENT
  point_dep
  point_arv
  airp_dep
  airp_arv
  grp_id
  grp_tid
{        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class/>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(grp_tid)</tid>
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
### ����������� ॣ����樨 ��㯯� �� ��� ���-�� ���⭨��� � ᥪ樨 passengers

$(defmacro NEW_CHECKIN_REQUEST
  point_dep
  point_arv
  airp_dep
  airp_arv
  passengers
  class=�
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
$(NEW_CHECKIN_SEGMENT $(point_dep) $(point_arv) $(airp_dep) $(airp_arv) $(passengers) $(class))
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

$(defmacro NEW_UNACCOMP_REQUEST
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
    <TCkinSaveUnaccompBag>
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
    </TCkinSaveUnaccompBag>
  </query>
</term>

})


#########################################################################################
### ����������� ����� ��������� �� ��� ���-�� ���⭨��� � ᥪ樨 passengers

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

$(defmacro CHANGE_UNACCOMP_REQUEST
  segments
  bags_tags_etc
  hall=777
  bag_refuse
  lang=RU
  capture=off
{

!! capture=$(capture) err=ignore
<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='$(lang)' term_id='2479792165'>
    <TCkinSaveUnaccompBag>
      <agent_stat_period>3</agent_stat_period>
      <segments>
$(segments)
      </segments>
      <hall>$(hall)</hall>\
$(if $(eq $(bag_refuse) "") {
      <bag_refuse/>} {
      <bag_refuse>$(bag_refuse)</bag_refuse>})
$(bags_tags_etc)
    </TCkinSaveUnaccompBag>
  </query>
</term>

})

$(defmacro NEW_CHECKIN_2982425618100
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>���������</surname>
    <name>���� �������������</name>
    <pers_type>��</pers_type>
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
      <surname>���������</surname>
      <first_name>���� �������������</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618100
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>���������</surname>
    <name>���� �������������</name>
    <pers_type>��</pers_type>\
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
      <surname>���������</surname>
      <first_name>���� �������������</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(pax_tid)</tid>}
)

$(defmacro NEW_CHECKIN_2982425618102
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>������</surname>
    <name>������ �����������</name>
    <pers_type>��</pers_type>
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
      <no>V��568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>������</surname>
      <first_name>������ �����������</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618102
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>������</surname>
    <name>������ �����������</name>
    <pers_type>��</pers_type>\
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
      <no>V��568572</no>
      <nationality>RUS</nationality>
      <birth_date>21.11.$(date_format %Y -5y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>������</surname>
      <first_name>������ �����������</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>
    <tid>$(pax_tid)</tid>}
)

$(defmacro NEW_CHECKIN_2982425618101
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>������</surname>
    <name>�������</name>
    <pers_type>��</pers_type>
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
      <no>V��841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>������</surname>
      <first_name>������ �����������</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro CHANGE_CHECKIN_2982425618101
  pax_id
  pax_tid
  ticket_confirm=1
  refuse
{    <pax_id>$(pax_id)</pax_id>
    <surname>������</surname>
    <name>�������</name>
    <pers_type>��</pers_type>\
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
      <no>V��841650</no>
      <nationality>RUS</nationality>
      <birth_date>11.08.$(date_format %Y -2y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>������</surname>
      <first_name>������ �����������</first_name>
    </document>
    <subclass>�</subclass>
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
    <pers_type>��</pers_type>
    <seat_no>6�</seat_no>
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
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145143704
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>CHEKMAREV</surname>
    <name>RONALD</name>
    <pers_type>��</pers_type>
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
      <no>V��815247</no>
      <nationality>RUS</nationality>
      <birth_date>29.01.$(date_format %Y -1y) 00:00:00</birth_date>
      <expiry_date>$(date_format %d.%m.%Y +1y) 00:00:00</expiry_date>
      <gender>M</gender>
      <surname>CHEKMAREV</surname>
      <first_name>RONALD KONSTANTINOVICH</first_name>
    </document>
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145134262
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>STIPIDI</surname>
    <name>ANGELINA</name>
    <pers_type>��</pers_type>
    <seat_no>11�</seat_no>
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
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145134263
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>AKOPOVA</surname>
    <name>OLIVIIA</name>
    <pers_type>��</pers_type>
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
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_2986145212943
  pax_id
  ticket_confirm=0
{    <pax_id>$(pax_id)</pax_id>
    <surname>VERGUNOV</surname>
    <name>VASILII LEONIDOVICH</name>
    <pers_type>��</pers_type>
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
    <subclass>�</subclass>
    <bag_pool_num/>
    <transfer/>}
)

$(defmacro NEW_CHECKIN_NOREC
  surname
  name
  pers_type=��
  seats=1
  subclass=�
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

$(defmacro NEW_CHECKIN
  pax_id
  surname
  name
  pers_type=��
  seats=1
  subclass=�
  transfer_subclass
{  <pax>
    <pax_id>$(pax_id)</pax_id>
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
    <subclass>$(subclass)</subclass>
    <bag_pool_num/>\
$(if $(eq $(transfer_subclass) "") {
    <transfer/>} {
    <transfer>
      <segment>
         <subclass>$(get_elem_id etSubcls $(transfer_subclass))</subclass>
      </segment>
    </transfer>})
  </pax>}
)

$(defmacro CHANGE_CHECKIN
  pax_id
  surname
  name
  pers_type=��
  subclass=�
  bag_pool_num
  refuse
{  <pax>
    <pax_id>$(pax_id)</pax_id>
    <surname>$(surname)</surname>
    <name>$(name)</name>
    <pers_type>$(pers_type)</pers_type>\
$(if $(eq $(refuse) "") {
    <refuse/>} {
    <refuse>$(refuse)</refuse>})
    <ticket_no/>
    <coupon_no/>
    <ticket_rem/>
    <ticket_confirm>0</ticket_confirm>\
$(if $(eq $(bag_pool_num) "") {
    <bag_pool_num/>} {
    <bag_pool_num>$(bag_pool_num)</bag_pool_num>})
    <subclass>$(subclass)</subclass>
    <tid>$(get_single_pax_tid $(pax_id))</tid>
  </pax>}
)

$(defmacro NEW_CHECKIN_2982410821479
  pax_id
  coupon_no
  transfer_subclass
{    <pax_id>$(pax_id)</pax_id>
    <surname>KOTOVA</surname>
    <name>IRINA</name>
    <pers_type>��</pers_type>
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
    <subclass>�</subclass>
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
    <pers_type>��</pers_type>\
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
    <subclass>�</subclass>
    <tid>$(pax_tid)</tid>}
)


$(defmacro NEW_CHECKIN_2982410821480
  pax_id
  coupon_no
  transfer_subclass
{    <pax_id>$(pax_id)</pax_id>
    <surname>MOTOVA</surname>
    <name>IRINA</name>
    <pers_type>��</pers_type>
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
    <subclass>�</subclass>
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
    <pers_type>��</pers_type>\
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
    <subclass>�</subclass>
    <tid>$(pax_tid)</tid>}
)

$(defmacro SVC_REQUEST_2982410821479
  pax_id
  display_id
  segment1_props
  segment2_props
  segment2_props2
{    <passenger id=\"$(pax_id)\" surname=\"������\" name=\"�����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"female\">
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
{    <passenger id=\"$(pax_id)\" surname=\"������\" name=\"�����\" category=\"ADT\" birthdate=\"1976-05-01\" sex=\"female\">
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

$(defmacro BAG_WT
  num
  bag_type
  airline
  pr_cabin
  amount
  weight
  bag_pool_num
  id
{  <bag>\
$(if $(eq $(bag_type) "") {
    <bag_type/>} {
    <bag_type>$(bag_type)</bag_type>})
    <airline>$(airline)</airline>\
$(if $(eq $(id) "") "" {
    <id>$(id)</id>})
    <num>$(num)</num>
    <pr_cabin>$(pr_cabin)</pr_cabin>
    <amount>$(amount)</amount>
    <weight>$(weight)</weight>
    <value_bag_num/>
    <pr_liab_limit>0</pr_liab_limit>
    <to_ramp>0</to_ramp>
    <using_scales>0</using_scales>
    <is_trfer>0</is_trfer>
    <bag_pool_num>$(bag_pool_num)</bag_pool_num>
  </bag>})

$(defmacro TAG
  num
  tag_type
  no
  bag_num
  pr_print=0
  color
{  <tag>
    <num>$(num)</num>
    <tag_type>$(tag_type)</tag_type>
    <no>$(no)</no>\
$(if $(eq $(color) "") {
    <color/>} {
    <color>$(color)</color>})\
$(if $(eq $(bag_num) "") {
    <bag_num/>} {
    <bag_num>$(bag_num)</bag_num>})
    <pr_print>$(pr_print)</pr_print>
  </tag>})

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
          <col>�������� �� ࠧ��ࠬ</col>
          <col>Oversize</col>
          <col>��� ���ᠦ��, ������ᨬ� �� �� ������������ � �����祭��, ������� ������ � 㯠�������� ���� �ॢ���� ��⠭������� ࠧ����</col>
          <col>Passengers properties irrespective of their naming and purpose, packed dimensions of which exceed allowances</col>
          <col/>
          <col>1</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>02</col>
          <col>�������� �� ����</col>
          <col>Overweight</col>
          <col>��� ���ᠦ��, ������ᨬ� �� �� ������������ � �����祭�� ���ᮩ ������ ���� ����� ��⠭��������� ���</col>
          <col>Passengers properties irrespective of their naming and purpose, exceeding weight allowance per piece</col>
          <col/>
          <col>2</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>03</col>
          <col>��������</col>
          <col>Equipment</col>
          <col>����-, ࠤ��-, �����-, �㤨�-, ����-, �⮠�������, ���஡�⮢�� �孨��, ���������, ���ᮩ ������ ���� ��� ��⠭��������� ���</col>
          <col>Tele-, radio, video, audio, cinema, photo equipment, consumer electronics, computers, exceeding weight allowance per piece</col>
          <col/>
          <col>3</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04</col>
          <col>��������</col>
          <col>Living creatures</col>
          <col>������ (����譨� � �����), ����, ���, �祫� � ��㣠� �������� (�� �᪫�祭��� ᮡ��-������३, ᮯ஢������� ᫥���).</col>
          <col>Animals (domestic and wild), birds, fish, bees and other living creatures (except for guide-dogs accompanying blind persons).</col>
          <col/>
          <col>4</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>05</col>
          <col>�����, ᠦ����, ������</col>
          <col>Flowers, saplings, greenery</col>
          <col>�����, ᠦ���� ��⥭��, ��饢�� ������ (������ ��, �����誠, 頢���, �ய, ᥫ줥३ � �.�.) ���ᮩ ��� ��⠭��������� ���</col>
          <col>Flowers, plant saplings, edible greens (spring onion, parsley, sorrel, dill, celery etc.) exceeding weight allowance</col>
          <col/>
          <col>5</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>06</col>
          <col>���. ��⥭��, ��⢨</col>
          <col>Dried plants, branches</col>
          <col>��襭� ��⥭��, ��⢨ ��ॢ쥢 � ����୨�� ���ᮩ ��� ��⠭��������� ���</col>
          <col>Dried plants, tree and bush branches exceeding weight allowance</col>
          <col/>
          <col>6</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>07</col>
          <col>����, ��</col>
          <col>Vegetables, berries</col>
          <col>����, �� ���ᮩ ��� ��⠭��������� ���</col>
          <col>Vegetables, berries exceeding weight allowance</col>
          <col/>
          <col>7</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>08</col>
          <col>��㦥���� ����</col>
          <col>Official mail</col>
          <col>����ꥣ��᪠� ����ᯮ������ � ����ᯮ������ ᯥ��裡.</col>
          <col>Courier service and special communication correspondence</col>
          <col/>
          <col>8</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>09</col>
          <col>��客� �������</col>
          <col>Furs</col>
          <col>��客� �������.</col>
          <col>Furs</col>
          <col/>
          <col>9</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>10</col>
          <col>��. �����㬥���</col>
          <col>Mus. instruments</col>
          <col>��몠��� �����㬥��� ���ᮩ ������ ���� ��� ��⠭��������� ���</col>
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
          <col>��</col>
          <col>UT</col>
          <col> </col>
          <col>����� ����� ��� �/�����</col>
          <col>Regular or hand bagg.</col>
          <col/>
          <col/>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>01</col>
          <col>�������� �� ࠧ��ࠬ</col>
          <col>Oversize</col>
          <col>��� ���ᠦ��, ������ᨬ� �� �� ������������ � �����祭��, ������� ������ � 㯠�������� ���� �ॢ���� ��⠭������� ࠧ����</col>
          <col>Passengers properties irrespective of their naming and purpose, packed dimensions of which exceed allowances</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>02</col>
          <col>�������� �� ����</col>
          <col>Overweight</col>
          <col>��� ���ᠦ��, ������ᨬ� �� �� ������������ � �����祭�� ���ᮩ ������ ���� ����� ��⠭��������� ���</col>
          <col>Passengers properties irrespective of their naming and purpose, exceeding weight allowance per piece</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>03</col>
          <col>��������</col>
          <col>Equipment</col>
          <col>����-, ࠤ��-, �����-, �㤨�-, ����-, �⮠�������, ���஡�⮢�� �孨��, ���������, ���ᮩ ������ ���� ��� ��⠭��������� ���</col>
          <col>Tele-, radio, video, audio, cinema, photo equipment, consumer electronics, computers, exceeding weight allowance per piece</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>04</col>
          <col>��������</col>
          <col>Living creatures</col>
          <col>������ (����譨� � �����), ����, ���, �祫� � ��㣠� �������� (�� �᪫�祭��� ᮡ��-������३, ᮯ஢������� ᫥���).</col>
          <col>Animals (domestic and wild), birds, fish, bees and other living creatures (except for guide-dogs accompanying blind persons).</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>05</col>
          <col>�����, ᠦ����, ������</col>
          <col>Flowers, saplings, greenery</col>
          <col>�����, ᠦ���� ��⥭��, ��饢�� ������ (������ ��, �����誠, 頢���, �ய, ᥫ줥३ � �.�.) ���ᮩ ��� ��⠭��������� ���</col>
          <col>Flowers, plant saplings, edible greens (spring onion, parsley, sorrel, dill, celery etc.) exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>06</col>
          <col>���. ��⥭��, ��⢨</col>
          <col>Dried plants, branches</col>
          <col>��襭� ��⥭��, ��⢨ ��ॢ쥢 � ����୨�� ���ᮩ ��� ��⠭��������� ���</col>
          <col>Dried plants, tree and bush branches exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>07</col>
          <col>����, ��</col>
          <col>Vegetables, berries</col>
          <col>����, �� ���ᮩ ��� ��⠭��������� ���</col>
          <col>Vegetables, berries exceeding weight allowance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>08</col>
          <col>��㦥���� ����</col>
          <col>Official mail</col>
          <col>����ꥣ��᪠� ����ᯮ������ � ����ᯮ������ ᯥ��裡.</col>
          <col>Courier service and special communication correspondence</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>09</col>
          <col>��客� �������</col>
          <col>Furs</col>
          <col>��客� �������.</col>
          <col>Furs</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>10</col>
          <col>��. �����㬥���</col>
          <col>Mus. instruments</col>
          <col>��몠��� �����㬥��� ���ᮩ ������ ���� ��� ��⠭��������� ���</col>
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

$(db_dump_table RFISC_BAG_PROPS)

!! capture=on err=ignore
$(cache PIKE $(lang) GRP_RFISC$(num) "" ""
        $(cache_sql_param bag_types_id integer $(grp_id))
)

>> lines=auto
      <rows tid='-1'>
        <row pr_del='0'>
          <col>0DD</col>
          <col>1� �릨 ᭮㡮� �� 20��</col>
          <col>1st ski snowboard upto 20kg</col>
          <col>0</col>
          <col>1</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0GP</col>
          <col>�� 50� 23�� � �� 80� 203�</col>
          <col>upto50lb 23kg and80li 203lcm</col>
          <col>0</col>
          <col>2</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>03C</col>
          <col>���� ��� � ��10�� 55�40�20�</col>
          <col>pc of bag for inf upto 10kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04J</col>
          <col>����� ��� �㬪� �� 85 �</col>
          <col>laptop or handbag up to 85 lcm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04U</col>
          <col>ᯮ�� ��������� �� 50�23��</col>
          <col>firearms up to 23kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>04V</col>
          <col>������५쭮� ��㦨� �� 32��</col>
          <col>firearms up to 32kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>07F</col>
          <col>1� ����� �릭��� ᭠�� ��23��</col>
          <col>1st ski equipment upto 23kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>08A</col>
          <col>��筠� �����</col>
          <col>carry on baggage</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0AA</col>
          <col>�।����祭�� �����</col>
          <col>pre paid baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0BS</col>
          <col>����⭮� � ����� �⤥� �� 23��</col>
          <col>pet in hold</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0BT</col>
          <col>����⭮� � ᠫ��� ��10�� 115�</col>
          <col>pet in cabin weight upto 10kg</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C2</col>
          <col>����� �� ����� 20�� 203�</col>
          <col>upto50lb 23kg and62li 158lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C4</col>
          <col>����� �� 55� 25��</col>
          <col>upto55lb 25kg baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0C5</col>
          <col>����� 21 30�� �� ����� 203�</col>
          <col>bag 21 30kg upto 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0CC</col>
          <col>��ࢮ� ��ॣ����� ����</col>
          <col>checked bag first</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0CZ</col>
          <col>����� �� 22� 10��</col>
          <col>upto 22lb 10kg baggage</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0DG</col>
          <col>���� �� ᢥ�� ��ଠ⨢�.�����</col>
          <col>weight system charge</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0EB</col>
          <col>�� ��⨭ 諥� � �릠�� ᭮㡤</col>
          <col>skibootshelmet with skisnowbrd</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0F4</col>
          <col>���᪠� ����᪠</col>
          <col>stroller or pushchair</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0FB</col>
          <col>�� 50� 23�� � ����� 80� 203�</col>
          <col>upto50lb 23kg over80li 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0FN</col>
          <col>���� ������ 30�� 203�</col>
          <col>pc of xbag upto 30kg 203lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0GO</col>
          <col>�� 50� 23�� � �� 62� 158�</col>
          <col>upto50lb 23kg and62li 158lcm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0H3</col>
          <col>��� ���� ��� 33-50��</col>
          <col>excess pc and weight 33-50kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IA</col>
          <col>ᯥ樠��� ������� ᡮ�</col>
          <col>baggage special charge</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IF</col>
          <col>�ॢ�� �� ���� � ��� ����</col>
          <col>excess weight and piece</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0IJ</col>
          <col>����� �� ����� 10�� 55x40x25cm</col>
          <col>bag upto 10 kg 55x40x25cm</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0KJ</col>
          <col>1� �宫 � 2 宪 ���誠�� 20��</col>
          <col>1stbagwith2icehockeystick 20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0KN</col>
          <col>1� 宪������ ᭠�殮� �� 20��</col>
          <col>1st hockey eqpmt upto 20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0L1</col>
          <col>�롮����� ᭠�� �� 44� 20��</col>
          <col>fishing equipment upto44lb20kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0L5</col>
          <col>��筠� ����� �� 5�� 40�30�20�</col>
          <col>cabin bag upto 5kg 40x30x20cm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0M5</col>
          <col>��� ����� 10�� 22� �� 39�100�</col>
          <col>carry10kg 22lb upto39li 100lcm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0M6</col>
          <col>���� ������ ��� ��24�� ��32��</col>
          <col>pc of bag from 23kg upto 32kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0MJ</col>
          <col>��筠� ����� ��10�� 55�40�20�</col>
          <col>cabin bag upto 10kg 55x40x20cm</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>0MN</col>
          <col>����譥� ����⭮� � ᠫ���</col>
          <col>pet in cabin</col>
          <col>1</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAC</col>
          <col>���� ���� ������ � ������ ��</col>
          <col>pc of bag in business class</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAM</col>
          <col>1 �� ��� � ���� ������ ��</col>
          <col>pc of bag in economy comfort</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AAY</col>
          <col>���� ���� ������ � ���� ��</col>
          <col>pc of bag in economy class</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AMM</col>
          <col>���஭� ��5�� � 1�� ��� ��ᯫ</col>
          <col>ammun upto 5kg with arms free</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>AMP</col>
          <col>���஭� �� 5��</col>
          <col>ammunition upto 5kg</col>
          <col>0</col>
          <col>100000</col>
          <col>$(grp_id)</col>
        </row>
        <row pr_del='0'>
          <col>PLB</col>
          <col>��쥤�� ��� � 1pnr ��30��203�</col>
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
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>03C</col>
          <col>���� ��� � ��10�� 55�40�20�</col>
          <col>pc of bag for inf upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>04J</col>
          <col>����� ��� �㬪� �� 85 �</col>
          <col>laptop or handbag up to 85 lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>04U</col>
          <col>ᯮ�� ��������� �� 50�23��</col>
          <col>firearms up to 23kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>04V</col>
          <col>������५쭮� ��㦨� �� 32��</col>
          <col>firearms up to 32kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>07F</col>
          <col>1� ����� �릭��� ᭠�� ��23��</col>
          <col>1st ski equipment upto 23kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>08A</col>
          <col>��筠� �����</col>
          <col>carry on baggage</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0AA</col>
          <col>�।����祭�� �����</col>
          <col>pre paid baggage</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0BS</col>
          <col>����⭮� � ����� �⤥� �� 23��</col>
          <col>pet in hold</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0BT</col>
          <col>����⭮� � ᠫ��� ��10�� 115�</col>
          <col>pet in cabin weight upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0C2</col>
          <col>����� �� ����� 20�� 203�</col>
          <col>upto50lb 23kg and62li 158lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0C4</col>
          <col>����� �� 55� 25��</col>
          <col>upto55lb 25kg baggage</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0C5</col>
          <col>����� 21 30�� �� ����� 203�</col>
          <col>bag 21 30kg upto 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0CC</col>
          <col>��ࢮ� ��ॣ����� ����</col>
          <col>checked bag first</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0CZ</col>
          <col>����� �� 22� 10��</col>
          <col>upto 22lb 10kg baggage</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0DD</col>
          <col>1� �릨 ᭮㡮� �� 20��</col>
          <col>1st ski snowboard upto 20kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0DG</col>
          <col>���� �� ᢥ�� ��ଠ⨢�.�����</col>
          <col>weight system charge</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0EB</col>
          <col>�� ��⨭ 諥� � �릠�� ᭮㡤</col>
          <col>skibootshelmet with skisnowbrd</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0F4</col>
          <col>���᪠� ����᪠</col>
          <col>stroller or pushchair</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0FB</col>
          <col>�� 50� 23�� � ����� 80� 203�</col>
          <col>upto50lb 23kg over80li 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0FN</col>
          <col>���� ������ 30�� 203�</col>
          <col>pc of xbag upto 30kg 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0GO</col>
          <col>�� 50� 23�� � �� 62� 158�</col>
          <col>upto50lb 23kg and62li 158lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0GP</col>
          <col>�� 50� 23�� � �� 80� 203�</col>
          <col>upto50lb 23kg and80li 203lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0H3</col>
          <col>��� ���� ��� 33-50��</col>
          <col>excess pc and weight 33-50kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0IA</col>
          <col>ᯥ樠��� ������� ᡮ�</col>
          <col>baggage special charge</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0IF</col>
          <col>�ॢ�� �� ���� � ��� ����</col>
          <col>excess weight and piece</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0IJ</col>
          <col>����� �� ����� 10�� 55x40x25cm</col>
          <col>bag upto 10 kg 55x40x25cm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0KJ</col>
          <col>1� �宫 � 2 宪 ���誠�� 20��</col>
          <col>1stbagwith2icehockeystick 20kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0KN</col>
          <col>1� 宪������ ᭠�殮� �� 20��</col>
          <col>1st hockey eqpmt upto 20kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0L1</col>
          <col>�롮����� ᭠�� �� 44� 20��</col>
          <col>fishing equipment upto44lb20kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0L5</col>
          <col>��筠� ����� �� 5�� 40�30�20�</col>
          <col>cabin bag upto 5kg 40x30x20cm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0M5</col>
          <col>��� ����� 10�� 22� �� 39�100�</col>
          <col>carry10kg 22lb upto39li 100lcm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0M6</col>
          <col>���� ������ ��� ��24�� ��32��</col>
          <col>pc of bag from 23kg upto 32kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0MJ</col>
          <col>��筠� ����� ��10�� 55�40�20�</col>
          <col>cabin bag upto 10kg 55x40x20cm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>0MN</col>
          <col>����譥� ����⭮� � ᠫ���</col>
          <col>pet in cabin</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>AAC</col>
          <col>���� ���� ������ � ������ ��</col>
          <col>pc of bag in business class</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>AAM</col>
          <col>1 �� ��� � ���� ������ ��</col>
          <col>pc of bag in economy comfort</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>AAY</col>
          <col>���� ���� ������ � ���� ��</col>
          <col>pc of bag in economy class</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>AMM</col>
          <col>���஭� ��5�� � 1�� ��� ��ᯫ</col>
          <col>ammun upto 5kg with arms free</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>AMP</col>
          <col>���஭� �� 5��</col>
          <col>ammunition upto 5kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>C</col>
          <col>PLB</col>
          <col>��쥤�� ��� � 1pnr ��30��203�</col>
          <col>pool of bag in 1 pnr 30kg203cm</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>029</col>
          <col>१�ࢨ� ���� �冷�</col>
          <col>१�ࢨ� ���� �冷�</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>02O</col>
          <col>���ᮭ���� ࠧ���祭��</col>
          <col>personal entertainment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>03P</col>
          <col>�ਮ��⭠� ॣ������</col>
          <col>priority check in</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>042</col>
          <col>����襭�� ����� ���㦨�����</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>04U</col>
          <col>ᯮ�� ��������� �� 50�23��</col>
          <col>firearms up to 23kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>060</col>
          <col>����襭�� ����� ���㦨�����</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>061</col>
          <col>���३� ᮯ஢��� �������</col>
          <col>upgrade of accomp infant</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>062</col>
          <col>������ �� ����</col>
          <col>business to first</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0AG</col>
          <col>��� ���</col>
          <col>executive lounge</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0AI</col>
          <col>����ࠪ</col>
          <col>breakfast</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0AN</col>
          <col>�����ਠ�᪨� ����</col>
          <col>vegetarian dinner</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0AR</col>
          <col>�����ਠ�� ����� � ��ᮬ</col>
          <col>�����ਠ�� ����� � ��ᮬ</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0B3</col>
          <col>��⠭��</col>
          <col>meal</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0B5</col>
          <col>�।���⥫�� �롮� ����</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BD</col>
          <col>�㡠誠 ���� ��� large</col>
          <col>adult polo shirt large</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BG</col>
          <col>��஦��� ���客��</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BH</col>
          <col>��ᮯ஢������� ॡ����</col>
          <col>unaccompanied minor</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BJ</col>
          <col>����襭�� ����� ���㦨�����</col>
          <col>upgrade</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BT</col>
          <col>����⭮� � ᠫ��� ��10�� 115�</col>
          <col>pet in cabin weight upto 10kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0BX</col>
          <col>������ ���</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0CC</col>
          <col>��ࢮ� ��ॣ����� ����</col>
          <col>checked bag first</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0CL</col>
          <col>����� � ���୥�</col>
          <col>internet access</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0F4</col>
          <col>���᪠� ����᪠</col>
          <col>stroller or pushchair</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0G9</col>
          <col>����� � ���祭�� ��㤪��</col>
          <col>����� � ���祭�� ��㤪��</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0JB</col>
          <col>��� � ���஭���</col>
          <col>��� � ���஭���</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0L8</col>
          <col>bundle ��㣠</col>
          <col>bundle service</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0LE</col>
          <col>��९த��� 2</col>
          <col>seafood meal 2</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0LQ</col>
          <col>��⠭�� 4</col>
          <col>meal 4</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0M6</col>
          <col>���� ������ ��� ��24�� ��32��</col>
          <col>pc of bag from 23kg upto 32kg</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>0MF</col>
          <col>������७��� ���</col>
          <col>low fat meal</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>ATX</col>
          <col>�롮� �।����⥫쭮�� ����</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BAS</col>
          <col>�।���⥫�� �롮� ����</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BBG</col>
          <col>��஦��� ���客��</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BD1</col>
          <col>����� ���</col>
          <col>bundle service</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BD2</col>
          <col>����� ��� 2</col>
          <col>bundle service 2</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BD3</col>
          <col>����� ��� 3</col>
          <col>bundle service 3</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BD4</col>
          <col>����� ��� 4</col>
          <col>bundle service 4</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BF1</col>
          <col>����稪�</col>
          <col>pancakes</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>BF2</col>
          <col>��୨��</col>
          <col>curd cakes</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>CMF</col>
          <col>�।���⥫�� �롮� ����</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>CRD</col>
          <col>��쪠</col>
          <col>cradle</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>LD1</col>
          <col>��� ��ਭ�</col>
          <col>grilled chicken</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>PN1</col>
          <col>砩 ��� ��� ������</col>
          <col>black or green tea</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>SP1</col>
          <col>ᬥ�� ����</col>
          <col>change</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>SPF</col>
          <col>�롮� ����</col>
          <col>seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>ST1</col>
          <col>1� ᠫ�� � 2�� �� 4� ��</col>
          <col>1� ᠫ�� � 2�� �� 4� ��</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>ST2</col>
          <col>���� � 2 �� 4 ��</col>
          <col>���� � 2 �� 4 ��</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>STA</col>
          <col>���� � 1�� ��� � ��室��</col>
          <col>���� � 1�� ��� � ��室��</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>STR</col>
          <col>�롮� ���� �� ॣ����樨</col>
          <col>pre reserved seat assignment</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>STW</col>
          <col>���� � ���� � 5 �鸞</col>
          <col>���� � ���� � 5 �鸞</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>F</col>
          <col>SW1</col>
          <col>����� � ���廊���</col>
          <col>beef sandwich</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>07E</col>
          <col>䨪��� �⮨���� ��ॢ����</col>
          <col>time to decide fee</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>0BB</col>
          <col>�㡠誠 ���� ��� ���</col>
          <col>adult polo shirt small</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>0BK</col>
          <col>����� �� ��ॢ����</col>
          <col>voucher for travel</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>0BX</col>
          <col>������ ���</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>0L7</col>
          <col>ᡮ� �� ��������� �����</col>
          <col>name change</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>997</col>
          <col>�।�����</col>
          <col>deposits down payments</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>DPS</col>
          <col>�।�����</col>
          <col>deposits down payments</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>RFP</col>
          <col>ᡮ� �� �뤠�� ���⢥ত ����</col>
          <col>confirmation docs issuance fee</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>M</col>
          <col>YYY</col>
          <col>�ࠢ�� �� ������</col>
          <col>ticket notice</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>R</col>
          <col>0A2</col>
          <col>�����஢ �ࠢ�� ��८��</col>
          <col>reissue override</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>R</col>
          <col>0A3</col>
          <col>�����஢���� �ࠢ�� ������</col>
          <col>refund override</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>R</col>
          <col>0A4</col>
          <col>reissue and refund override</col>
          <col>reissue and refund override</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>T</col>
          <col>0BG</col>
          <col>��஦��� ���客��</col>
          <col>trip insurance</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>T</col>
          <col>0BX</col>
          <col>������ ���</col>
          <col>lounge access</col>
        </row>
        <row pr_del='0'>
          <col>��</col>
          <col>UT</col>
          <col>T</col>
          <col>0L7</col>
          <col>ᡮ� �� ��������� �����</col>
          <col>name change</col>
        </row>
      </rows>

})
