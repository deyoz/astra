
#########################################################################################
### ����������� ॣ����樨 ��㯯� �� ��� ���-�� ���⭨��� � ᥪ樨 passengers

$(defmacro NEW_CHECKIN_REQUEST
  point_dep
  point_arv
  airp_dep
  airp_arv
  passengers
  hall=777
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <transfer/>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class>�</class>
          <status>K</status>
          <wl_type/>
$(passengers)
        </segment>
      </segments>
      <hall>$(hall)</hall>
    </TCkinSavePax>
  </query>
</term>}

}) #end defmacro NEW_CHECKIN_REQUEST

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
  capture=off
{

!! capture=$(capture) err=ignore
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='CheckIn' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='EN' term_id='2479792165'>
    <TCkinSavePax>
      <agent_stat_period>3</agent_stat_period>
      <segments>
        <segment>
          <point_dep>$(point_dep)</point_dep>
          <point_arv>$(point_arv)</point_arv>
          <airp_dep>$(get_elem_id etAirp $(airp_dep))</airp_dep>
          <airp_arv>$(get_elem_id etAirp $(airp_arv))</airp_arv>
          <class>�</class>
          <grp_id>$(grp_id)</grp_id>
          <tid>$(grp_tid)</tid>
$(passengers)
        </segment>
      </segments>
      <hall>$(hall)</hall>
      <bag_refuse/>
    </TCkinSavePax>
  </query>
</term>}

}) #end defmacro CHANGE_CHECKIN_REQUEST

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
    <ticket_confirm>0</ticket_confirm>
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
    <ticket_confirm>0</ticket_confirm>
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
    <ticket_confirm>0</ticket_confirm>
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





