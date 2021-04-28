$(defmacro get_svc_availability_resp
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_availability>
    <svc_list passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\">
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
        <name language=\"ru\">›‹‚›… ‘€‘’ „ 44” 20ƒ</name>
        <description>FI</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRYY 10KG 22LB UPTO39LI 100LCM</name>
        <name language=\"ru\">“— ‹€„ 10ƒ 22” „ 39„100‘</name>
        <description>10</description>
        <description>4Q</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MN\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">PET IN CABIN</name>
        <name language=\"ru\">„€…… †‚’… ‚ ‘€‹…</name>
        <description>PE</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0B5\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <free_baggage_norm company=\"UT\" type=\"piece\">
        <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         FIREARMS UP TO 23KG (04U)
Free baggage exception:  PIECE OF BAG UPTO23KG 203LCM , PIECE OF BAG UPTO 23KG
                           203LCM (0GP)
                     and PC OF BAG FROM 23KG UPTO 32KG (0M6)
        </text>
        <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1
1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :  ‘’ ƒ…‘’“† „ 50”23ƒ (04U)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ­®ΰ¬γ:   ‹: 1 … ‹…… 23ƒ/203‘ ,  ‹  1 … ‹……
                                 23ƒ 203‘ (0GP)
                             ¨ …‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ (0M6)
        </text>
      </free_baggage_norm>
      <free_carry_on_norm company=\"UT\" type=\"piece\">
        <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CABIN BAG UPTO 10KG 55X40X20CM (0MJ)
        </text>
        <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1
1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               “—€ ‹€„ „10ƒ 55•40•20‘ (0MJ)
        </text>
      </free_carry_on_norm>
    </svc_list>
  </svc_availability>
</answer>})
}
)


$(defmacro get_svc_payment_status_resp
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_payment_status>
    <free_baggage_norm passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" company=\"UT\" type=\"piece\">
      <text language=\"en\">Free baggage allowance 1PC 1ST checked bag:         PIECE OF BAG UPTO23KG 203LCM , PIECE OF BAG UPTO 23KG 203LCM (0GP) or FIREARMS UP TO 23KG (04U)</text>
      <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1 1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :   ‹: 1 … ‹…… 23ƒ/203‘ ,  ‹  1 … ‹…… 23ƒ 203‘ (0GP) ¨«¨ ‘’ ƒ…‘’“† „ 50”23ƒ (04U)</text>
    </free_baggage_norm>
    <free_carry_on_norm passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" company=\"UT\" type=\"piece\">
      <text language=\"en\">Carry-on bag 1PC 1ST carry bag:       CABIN BAG UPTO 10KG 55X40X20CM (0MJ)</text>
      <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1 1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               “—€ ‹€„ „10ƒ 55•40•20‘ (0MJ)</text>
    </free_carry_on_norm>
    <svc passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" rfisc=\"0L1\" service_type=\"C\" payment_status=\"free\" company=\"UT\"/>
  </svc_payment_status>
</answer>})
}
)


$(defmacro get_svc_availability_invalid_resp
{
$(utf8 invalid_answer)
}
)


$(defmacro get_svc_payment_status_invalid_resp
{
$(utf8 invalid_answer)
}
)

$(defmacro SVC_LIST_UT_1PAX_SEG0
  pax_id
{    <svc_list passenger-id=\"$(pax_id)\" segment-id=\"0\">
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
        <name language=\"ru\">›‹‚›… ‘€‘’ „ 44” 20ƒ</name>
        <description>FI</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0GP\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND80LI 203LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  „ 80„ 203‘</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRY10KG 22LB UPTO39LI 100LCM</name>
        <name language=\"ru\">“— ‹€„ 10ƒ 22” „ 39„100‘</name>
        <description>10</description>
        <description>4Q</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MN\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">PET IN CABIN</name>
        <name language=\"ru\">„€…… †‚’… ‚ ‘€‹…</name>
        <description>PE</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04J\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">LAPTOP OR HANDBAG UP TO 85 LCM</name>
        <name language=\"ru\">“’“ ‹ ‘“€ „ 85 ‘</name>
        <description>3S</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAC\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN BUSINESS CLASS</name>
        <name language=\"ru\">„ …‘’ €ƒ€†€ ‚ ‡…‘ ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAM\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN ECONOMY COMFORT</name>
        <name language=\"ru\">1  €ƒ ‚  ”’ ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAY\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN ECONOMY CLASS</name>
        <name language=\"ru\">„ …‘’ €ƒ€†€ ‚  ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04U\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 23KG</name>
        <name language=\"ru\">‘’ ƒ…‘’“† „ 50”23ƒ</name>
        <description>FA</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04V\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 32KG</name>
        <name language=\"ru\">ƒ…‘’…‹… “†… „ 32ƒ</name>
        <description>FA</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0F4\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"IN\">
        <name language=\"en\">STROLLER OR PUSHCHAIR</name>
        <name language=\"ru\">„…’‘€ ‹‘€</name>
        <description>ST</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0H3\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">EXCESS PC AND WEIGHT 33-50KG</name>
        <name language=\"ru\">„ …‘’ ‚…‘ 33-50ƒ</name>
        <description>X2</description>
        <description>X0</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C4\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO55LB 25KG BAGGAGE</name>
        <name language=\"ru\">€ƒ€† „ 55” 25ƒ</name>
        <description>25</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MJ\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CABIN BAG UPTO 10KG 55X40X20CM</name>
        <name language=\"ru\">“—€ ‹€„ „10ƒ 55•40•20‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AMP\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">AMMUNITION UPTO 5KG</name>
        <name language=\"ru\">€’› „ 5ƒ</name>
        <description>05</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">EXCESS WEIGHT AND PIECE</name>
        <name language=\"ru\">…‚›  ‚…‘“  ‹ …‘’</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"07F\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST SKI EQUIPMENT UPTO 23KG</name>
        <name language=\"ru\">1‰ ‹ ‹›†ƒ ‘€† „23ƒ</name>
        <description>SK</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0KN\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST HOCKEY EQPMT UPTO 20KG</name>
        <name language=\"ru\">1… •…‰… ‘€†… „ 20ƒ</name>
        <description>HE</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0CC\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">CHECKED BAG FIRST</name>
        <name language=\"ru\">…‚… ‡€…ƒ‘’ …‘’</name>
        <description>B1</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0KJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1STBAGWITH2ICEHOCKEYSTICK 20KG</name>
        <name language=\"ru\">1‰ —…•‹ ‘ 2 • ‹€ 20ƒ</name>
        <description>23</description>
        <description>6U</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AMM\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">AMMUN UPTO 5KG WITH ARMS FREE</name>
        <name language=\"ru\">€’› „5ƒ ‘ 1…„ “† …‘‹</name>
        <description>05</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C2\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND62LI 158LCM</name>
        <name language=\"ru\">€ƒ€† … ‹…… 20ƒ 203‘</name>
        <description>20</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C5\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAG 21 30KG UPTO 203LCM</name>
        <name language=\"ru\">€ƒ€† 21 30ƒ … ‹…… 203‘</name>
        <description>30</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0EB\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">SKIBOOTSHELMET WITH SKISNOWBRD</name>
        <name language=\"ru\">‹›† ’ ‹… ‘ ‹›†€ ‘“„</name>
        <description>RK</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"PLB\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">POOL OF BAG IN 1 PNR 30KG203CM</name>
        <name language=\"ru\">…„ €ƒ ‚ 1PNR „30ƒ203‘</name>
        <description>30</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M6\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FROM 23KG UPTO 32KG</name>
        <name language=\"ru\">…‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ</name>
        <description>2V</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0GO\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND62LI 158LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  „ 62„ 158‘</name>
        <description>23</description>
        <description>6U</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0BS\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"PT\" subgroup=\"PH\">
        <name language=\"en\">PET IN HOLD</name>
        <name language=\"ru\">†‚’… ‚ €ƒ€† ’„…‹ „ 23ƒ</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"03C\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FOR INF UPTO 10KG</name>
        <name language=\"ru\">…‘’ €ƒ  „10ƒ 55•40•20‘</name>
        <description>10</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0FN\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF XBAG UPTO 30KG 203LCM</name>
        <name language=\"ru\">…‘’ €ƒ€†€ 30ƒ 203‘</name>
        <description>32</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0AA\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PRE PAID BAGGAGE</name>
        <name language=\"ru\">…„‹€—…›‰ €ƒ€†</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IA\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAGGAGE SPECIAL CHARGE</name>
        <name language=\"ru\">‘…–€‹›‰ €ƒ€†›‰ ‘</name>
        <description>X0</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0BT\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"PT\" subgroup=\"PC\">
        <name language=\"en\">PET IN CABIN WEIGHT UPTO 10KG</name>
        <name language=\"ru\">†‚’… ‚ ‘€‹… „10ƒ 115‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"08A\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRY ON BAGGAGE</name>
        <name language=\"ru\">“—€ ‹€„</name>
        <description>10</description>
        <description>55</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CABIN BAG UPTO 5KG 40X30X20CM</name>
        <name language=\"ru\">“—€ ‹€„ „ 5ƒ 40•30•20‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAG UPTO 10 KG 55X40X25CM</name>
        <name language=\"ru\">€ƒ€† … ‹…… 10ƒ 55X40X25CM</name>
        <description>X7</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0DG\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"XBAG\" carry_on=\"false\" group=\"BG\" subgroup=\"XS\">
        <name language=\"en\">WEIGHT SYSTEM CHARGE</name>
        <name language=\"ru\">‹€’€ ‡€ ‘‚…• €’‚.€ƒ€†</name>
        <description>WT</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0CZ\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"XBGX\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO 22LB 10KG BAGGAGE</name>
        <name language=\"ru\">€ƒ€† „ 22” 10ƒ</name>
        <description>10</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0FB\" booking=\"SSR\" ssr=\"XBAG\" carry_on=\"false\">
        <name language=\"en\">UPTO50LB 23KG OVER80LI 203LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  ‹…… 80„ 203‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0DD\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST SKI SNOWBOARD UPTO 20KG</name>
        <name language=\"ru\">1… ‹›† ‘“„ „ 20ƒ</name>
        <description>SK</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0F4\" emd_type=\"OTHR\" carry_on=\"false\" group=\"BG\" subgroup=\"IN\">
        <name language=\"en\">STROLLER OR PUSHCHAIR</name>
        <name language=\"ru\">„…’‘€ ‹‘€</name>
        <description>ST</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"LD1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">GRILLED CHICKEN</name>
        <name language=\"ru\">€‹› “›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"F\" rfisc=\"0BB\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT SMALL</name>
        <name language=\"ru\">“€€ ‹ ‚‡ €‹</name>
        <description>AD</description>
        <description>SM</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AI\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\" subgroup=\"BR\">
        <name language=\"en\">BREAKFAST</name>
        <name language=\"ru\">‡€‚’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LQ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL 4</name>
        <name language=\"ru\">’€… 4</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"03P\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"PO\">
        <name language=\"en\">PRIORITY CHECK IN</name>
        <name language=\"ru\">’…’€ …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"CMF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0L8\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">BUNDLE “‘‹“ƒ€</name>
        <description>YY</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0BJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BAS\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"062\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">BUSINESS TO FIRST</name>
        <name language=\"ru\">‡…‘ € …‚›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BBG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0M6\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FROM 23KG UPTO 32KG</name>
        <name language=\"ru\">…‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ</name>
        <description>2V</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0JB\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\">
        <name language=\"en\">“†… ‘ €’€</name>
        <name language=\"ru\">“†… ‘ €’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"F\" rfisc=\"0BD\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT LARGE</name>
        <name language=\"ru\">“€€ ‹ ‚‡ LARGE</name>
        <description>AD</description>
        <description>LG</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0CC\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ABAG\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">CHECKED BAG FIRST</name>
        <name language=\"ru\">…‚… ‡€…ƒ‘’ …‘’</name>
        <description>B1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SPF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"PN1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BLACK OR GREEN TEA</name>
        <name language=\"ru\">—€‰ —…›‰ ‹ ‡…‹…›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"04U\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 23KG</name>
        <name language=\"ru\">‘’ ƒ…‘’“† „ 50”23ƒ</name>
        <description>FA</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"029\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"FT\">
        <name language=\"en\">…‡…‚ …‘’€ „</name>
        <name language=\"ru\">…‡…‚ …‘’€ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">PANCAKES</name>
        <name language=\"ru\">‹—</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0AG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">EXECUTIVE LOUNGE</name>
        <name language=\"ru\">‚ ‡€‹</name>
        <description>EX</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"SW1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BEEF SANDWICH</name>
        <name language=\"ru\">‘„‚— ‘ ƒ‚„‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AN\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"VGML\" carry_on=\"false\" group=\"ML\" subgroup=\"DI\">
        <name language=\"en\">VEGETARIAN DINNER</name>
        <name language=\"ru\">‚…ƒ…’€€‘‰ …„</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0MF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">LOW FAT MEAL</name>
        <name language=\"ru\">…‡†…€ ™€</name>
        <description>LF</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0BT\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"AVIH\" carry_on=\"true\" group=\"PT\" subgroup=\"PC\">
        <name language=\"en\">PET IN CABIN WEIGHT UPTO 10KG</name>
        <name language=\"ru\">†‚’… ‚ ‘€‹… „10ƒ 115‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LE\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"SFML\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">SEAFOOD MEAL 2</name>
        <name language=\"ru\">…„“’› 2</name>
        <description>SO</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF2\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">CURD CAKES</name>
        <name language=\"ru\">‘›</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BH\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"UMNR\" carry_on=\"false\" group=\"UN\" subgroup=\"MR\">
        <name language=\"en\">UNACCOMPANIED MINOR</name>
        <name language=\"ru\">…‘‚†„€…›‰ ……</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SP1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">CHANGE</name>
        <name language=\"ru\">‘…€ …‘’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"060\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"997\" emd_type=\"EMD-A\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"07E\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">TIME TO DECIDE FEE</name>
        <name language=\"ru\">”‘€– ‘’‘’ ……‚‡</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0B3\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL</name>
        <name language=\"ru\">’€…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"DPS\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"061\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE OF ACCOMP INFANT</name>
        <name language=\"ru\">€ƒ…‰„ ‘‚†„ ‹€„…–€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"CRD\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SF\">
        <name language=\"en\">CRADLE</name>
        <name language=\"ru\">‹‹€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STR\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€  …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"042\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST1\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
        <name language=\"ru\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0G9\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTCH\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <name language=\"ru\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <description>CN</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STA\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
        <name language=\"ru\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST2\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‘ 2  4 „</name>
        <name language=\"ru\">…‘’ ‘ 2  4 „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STW\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ “ € ‘ 5 „€</name>
        <name language=\"ru\">…‘’ “ € ‘ 5 „€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A2\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"31\">
        <name language=\"en\">REISSUE OVERRIDE</name>
        <name language=\"ru\">ƒ‚ €‚‹ ……”</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A3\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"33\">
        <name language=\"en\">REFUND OVERRIDE</name>
        <name language=\"ru\">ƒ‚€… €‚‹ ‚‡‚€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A4\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"3A\">
        <name language=\"en\">REISSUE AND REFUND OVERRIDE</name>
        <name language=\"ru\">REISSUE AND REFUND OVERRIDE</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"02O\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"IE\" subgroup=\"PE\">
        <name language=\"en\">PERSONAL ENTERTAINMENT</name>
        <name language=\"ru\">…‘€‹›… €‡‚‹…—…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"RFP\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">CONFIRMATION DOCS ISSUANCE FEE</name>
        <name language=\"ru\">‘ ‡€ ‚›„€—“ „’‚…†„ „“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD4\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 4</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 4</name>
        <description>Y4</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD1\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">€…’ “‘‹“ƒ</name>
        <description>Y1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0B5\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"YYY\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">TICKET NOTICE</name>
        <name language=\"ru\">‘€‚€  ‹…’“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0CL\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">INTERNET ACCESS</name>
        <name language=\"ru\">„‘’“ ‚ ’……’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AR\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTTR\" carry_on=\"false\" group=\"ML\" subgroup=\"LU\">
        <name language=\"en\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <name language=\"ru\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0BK\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\" subgroup=\"PD\">
        <name language=\"en\">VOUCHER FOR TRAVEL</name>
        <name language=\"ru\">‚€“—… € ……‚‡“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ATX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …„—’’…‹ƒ …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD2\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 2</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 2</name>
        <description>Y2</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD3\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 3</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 3</name>
        <description>Y3</description>
      </svc>
      <free_baggage_norm company=\"UT\" type=\"piece\" rfiscs=\"0C2,0C4,0DD\">
        <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         UPTO50LB 23KG AND62LI 158LCM , PIECE OF BAG UPTO20KG
                           203LCM (0C2)
Free baggage exception:  UPTO55LB 25KG BAGGAGE (0C4)
                     and 1ST SKI SNOWBOARD UPTO 20KG (0DD)
Embargoes
  1ST SKI SNOWBOARD UPTO 20KG (0DD) 2PC and more</text>
        <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1
1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :  €ƒ€† … ‹…… 20ƒ 203‘ (0C2)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ­®ΰ¬γ:  €ƒ€† „ 55” 25ƒ (0C4)
                             ¨ 1… ‹›† ‘“„ „ 20ƒ (0DD)
‡ ―ΰ¥ι¥­® ª ―¥ΰ¥Ά®§ª¥
  1… ‹›† ‘“„ „ 20ƒ (0DD) 2 ¨ ΅®«¥¥</text>
      </free_baggage_norm>
      <free_carry_on_norm company=\"UT\" type=\"unknown\" rfiscs=\"08A,0L5\">
        <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CARRY ON BAGGAGE CHARGES , CABIN BAG UPTO 5KG 40X30X20CM
                       (0L5)
Carry-on exception:  CARRY ON BAGGAGE (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
        <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1
1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               ‹€’€ “—€ ‹€„ , “—€ ‹€„ „
                                        5ƒ 40•30•20‘ (0L5)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ΰγη­γξ ª« ¤μ:  “—€ ‹€„ (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
      </free_carry_on_norm>
    </svc_list>})

$(defmacro SVC_LIST_UT_1PAX_SEG1
  pax_id
{    <svc_list passenger-id=\"$(pax_id)\" segment-id=\"1\">
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
        <name language=\"ru\">›‹‚›… ‘€‘’ „ 44” 20ƒ</name>
        <description>FI</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0GP\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND80LI 203LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  „ 80„ 203‘</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRY10KG 22LB UPTO39LI 100LCM</name>
        <name language=\"ru\">“— ‹€„ 10ƒ 22” „ 39„100‘</name>
        <description>10</description>
        <description>4Q</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MN\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">PET IN CABIN</name>
        <name language=\"ru\">„€…… †‚’… ‚ ‘€‹…</name>
        <description>PE</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04J\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">LAPTOP OR HANDBAG UP TO 85 LCM</name>
        <name language=\"ru\">“’“ ‹ ‘“€ „ 85 ‘</name>
        <description>3S</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAC\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN BUSINESS CLASS</name>
        <name language=\"ru\">„ …‘’ €ƒ€†€ ‚ ‡…‘ ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAM\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN ECONOMY COMFORT</name>
        <name language=\"ru\">1  €ƒ ‚  ”’ ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AAY\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG IN ECONOMY CLASS</name>
        <name language=\"ru\">„ …‘’ €ƒ€†€ ‚  ‹</name>
        <description>23</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04U\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 23KG</name>
        <name language=\"ru\">‘’ ƒ…‘’“† „ 50”23ƒ</name>
        <description>FA</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"04V\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 32KG</name>
        <name language=\"ru\">ƒ…‘’…‹… “†… „ 32ƒ</name>
        <description>FA</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0F4\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"IN\">
        <name language=\"en\">STROLLER OR PUSHCHAIR</name>
        <name language=\"ru\">„…’‘€ ‹‘€</name>
        <description>ST</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0H3\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">EXCESS PC AND WEIGHT 33-50KG</name>
        <name language=\"ru\">„ …‘’ ‚…‘ 33-50ƒ</name>
        <description>X2</description>
        <description>X0</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C4\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO55LB 25KG BAGGAGE</name>
        <name language=\"ru\">€ƒ€† „ 55” 25ƒ</name>
        <description>25</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MJ\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CABIN BAG UPTO 10KG 55X40X20CM</name>
        <name language=\"ru\">“—€ ‹€„ „10ƒ 55•40•20‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AMP\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">AMMUNITION UPTO 5KG</name>
        <name language=\"ru\">€’› „ 5ƒ</name>
        <description>05</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">EXCESS WEIGHT AND PIECE</name>
        <name language=\"ru\">…‚›  ‚…‘“  ‹ …‘’</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"07F\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST SKI EQUIPMENT UPTO 23KG</name>
        <name language=\"ru\">1‰ ‹ ‹›†ƒ ‘€† „23ƒ</name>
        <description>SK</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0KN\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST HOCKEY EQPMT UPTO 20KG</name>
        <name language=\"ru\">1… •…‰… ‘€†… „ 20ƒ</name>
        <description>HE</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0CC\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">CHECKED BAG FIRST</name>
        <name language=\"ru\">…‚… ‡€…ƒ‘’ …‘’</name>
        <description>B1</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0KJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1STBAGWITH2ICEHOCKEYSTICK 20KG</name>
        <name language=\"ru\">1‰ —…•‹ ‘ 2 • ‹€ 20ƒ</name>
        <description>23</description>
        <description>6U</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"AMM\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">AMMUN UPTO 5KG WITH ARMS FREE</name>
        <name language=\"ru\">€’› „5ƒ ‘ 1…„ “† …‘‹</name>
        <description>05</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C2\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND62LI 158LCM</name>
        <name language=\"ru\">€ƒ€† … ‹…… 20ƒ 203‘</name>
        <description>20</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0C5\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAG 21 30KG UPTO 203LCM</name>
        <name language=\"ru\">€ƒ€† 21 30ƒ … ‹…… 203‘</name>
        <description>30</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0EB\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">SKIBOOTSHELMET WITH SKISNOWBRD</name>
        <name language=\"ru\">‹›† ’ ‹… ‘ ‹›†€ ‘“„</name>
        <description>RK</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"PLB\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">POOL OF BAG IN 1 PNR 30KG203CM</name>
        <name language=\"ru\">…„ €ƒ ‚ 1PNR „30ƒ203‘</name>
        <description>30</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M6\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FROM 23KG UPTO 32KG</name>
        <name language=\"ru\">…‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ</name>
        <description>2V</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0GO\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO50LB 23KG AND62LI 158LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  „ 62„ 158‘</name>
        <description>23</description>
        <description>6U</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0BS\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"PT\" subgroup=\"PH\">
        <name language=\"en\">PET IN HOLD</name>
        <name language=\"ru\">†‚’… ‚ €ƒ€† ’„…‹ „ 23ƒ</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"03C\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FOR INF UPTO 10KG</name>
        <name language=\"ru\">…‘’ €ƒ  „10ƒ 55•40•20‘</name>
        <description>10</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0FN\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF XBAG UPTO 30KG 203LCM</name>
        <name language=\"ru\">…‘’ €ƒ€†€ 30ƒ 203‘</name>
        <description>32</description>
        <description>6B</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0AA\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PRE PAID BAGGAGE</name>
        <name language=\"ru\">…„‹€—…›‰ €ƒ€†</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IA\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAGGAGE SPECIAL CHARGE</name>
        <name language=\"ru\">‘…–€‹›‰ €ƒ€†›‰ ‘</name>
        <description>X0</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0BT\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"PT\" subgroup=\"PC\">
        <name language=\"en\">PET IN CABIN WEIGHT UPTO 10KG</name>
        <name language=\"ru\">†‚’… ‚ ‘€‹… „10ƒ 115‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"08A\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRY ON BAGGAGE</name>
        <name language=\"ru\">“—€ ‹€„</name>
        <description>10</description>
        <description>55</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CABIN BAG UPTO 5KG 40X30X20CM</name>
        <name language=\"ru\">“—€ ‹€„ „ 5ƒ 40•30•20‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0IJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">BAG UPTO 10 KG 55X40X25CM</name>
        <name language=\"ru\">€ƒ€† … ‹…… 10ƒ 55X40X25CM</name>
        <description>X7</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0DG\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"XBAG\" carry_on=\"false\" group=\"BG\" subgroup=\"XS\">
        <name language=\"en\">WEIGHT SYSTEM CHARGE</name>
        <name language=\"ru\">‹€’€ ‡€ ‘‚…• €’‚.€ƒ€†</name>
        <description>WT</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0CZ\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"XBGX\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">UPTO 22LB 10KG BAGGAGE</name>
        <name language=\"ru\">€ƒ€† „ 22” 10ƒ</name>
        <description>10</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0FB\" booking=\"SSR\" ssr=\"XBAG\" carry_on=\"false\">
        <name language=\"en\">UPTO50LB 23KG OVER80LI 203LCM</name>
        <name language=\"ru\">„ 50” 23ƒ  ‹…… 80„ 203‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0DD\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">1ST SKI SNOWBOARD UPTO 20KG</name>
        <name language=\"ru\">1… ‹›† ‘“„ „ 20ƒ</name>
        <description>SK</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0F4\" emd_type=\"OTHR\" carry_on=\"false\" group=\"BG\" subgroup=\"IN\">
        <name language=\"en\">STROLLER OR PUSHCHAIR</name>
        <name language=\"ru\">„…’‘€ ‹‘€</name>
        <description>ST</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"LD1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">GRILLED CHICKEN</name>
        <name language=\"ru\">€‹› “›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"F\" rfisc=\"0BB\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT SMALL</name>
        <name language=\"ru\">“€€ ‹ ‚‡ €‹</name>
        <description>AD</description>
        <description>SM</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AI\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\" subgroup=\"BR\">
        <name language=\"en\">BREAKFAST</name>
        <name language=\"ru\">‡€‚’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LQ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL 4</name>
        <name language=\"ru\">’€… 4</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"03P\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"PO\">
        <name language=\"en\">PRIORITY CHECK IN</name>
        <name language=\"ru\">’…’€ …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"CMF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0L8\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">BUNDLE “‘‹“ƒ€</name>
        <description>YY</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0BJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BAS\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"062\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">BUSINESS TO FIRST</name>
        <name language=\"ru\">‡…‘ € …‚›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BBG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0M6\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FROM 23KG UPTO 32KG</name>
        <name language=\"ru\">…‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ</name>
        <description>2V</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0JB\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\">
        <name language=\"en\">“†… ‘ €’€</name>
        <name language=\"ru\">“†… ‘ €’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"F\" rfisc=\"0BD\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT LARGE</name>
        <name language=\"ru\">“€€ ‹ ‚‡ LARGE</name>
        <description>AD</description>
        <description>LG</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0CC\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ABAG\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">CHECKED BAG FIRST</name>
        <name language=\"ru\">…‚… ‡€…ƒ‘’ …‘’</name>
        <description>B1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SPF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"PN1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BLACK OR GREEN TEA</name>
        <name language=\"ru\">—€‰ —…›‰ ‹ ‡…‹…›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"04U\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 23KG</name>
        <name language=\"ru\">‘’ ƒ…‘’“† „ 50”23ƒ</name>
        <description>FA</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"029\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"FT\">
        <name language=\"en\">…‡…‚ …‘’€ „</name>
        <name language=\"ru\">…‡…‚ …‘’€ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">PANCAKES</name>
        <name language=\"ru\">‹—</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0AG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">EXECUTIVE LOUNGE</name>
        <name language=\"ru\">‚ ‡€‹</name>
        <description>EX</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"SW1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BEEF SANDWICH</name>
        <name language=\"ru\">‘„‚— ‘ ƒ‚„‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AN\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"VGML\" carry_on=\"false\" group=\"ML\" subgroup=\"DI\">
        <name language=\"en\">VEGETARIAN DINNER</name>
        <name language=\"ru\">‚…ƒ…’€€‘‰ …„</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0MF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">LOW FAT MEAL</name>
        <name language=\"ru\">…‡†…€ ™€</name>
        <description>LF</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0BT\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"AVIH\" carry_on=\"true\" group=\"PT\" subgroup=\"PC\">
        <name language=\"en\">PET IN CABIN WEIGHT UPTO 10KG</name>
        <name language=\"ru\">†‚’… ‚ ‘€‹… „10ƒ 115‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LE\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"SFML\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">SEAFOOD MEAL 2</name>
        <name language=\"ru\">…„“’› 2</name>
        <description>SO</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF2\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">CURD CAKES</name>
        <name language=\"ru\">‘›</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BH\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"UMNR\" carry_on=\"false\" group=\"UN\" subgroup=\"MR\">
        <name language=\"en\">UNACCOMPANIED MINOR</name>
        <name language=\"ru\">…‘‚†„€…›‰ ……</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SP1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">CHANGE</name>
        <name language=\"ru\">‘…€ …‘’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"060\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"997\" emd_type=\"EMD-A\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"07E\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">TIME TO DECIDE FEE</name>
        <name language=\"ru\">”‘€– ‘’‘’ ……‚‡</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0B3\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL</name>
        <name language=\"ru\">’€…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"DPS\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"061\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE OF ACCOMP INFANT</name>
        <name language=\"ru\">€ƒ…‰„ ‘‚†„ ‹€„…–€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"CRD\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SF\">
        <name language=\"en\">CRADLE</name>
        <name language=\"ru\">‹‹€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STR\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€  …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"042\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST1\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
        <name language=\"ru\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0G9\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTCH\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <name language=\"ru\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <description>CN</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STA\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
        <name language=\"ru\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST2\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‘ 2  4 „</name>
        <name language=\"ru\">…‘’ ‘ 2  4 „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STW\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ “ € ‘ 5 „€</name>
        <name language=\"ru\">…‘’ “ € ‘ 5 „€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A2\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"31\">
        <name language=\"en\">REISSUE OVERRIDE</name>
        <name language=\"ru\">ƒ‚ €‚‹ ……”</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A3\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"33\">
        <name language=\"en\">REFUND OVERRIDE</name>
        <name language=\"ru\">ƒ‚€… €‚‹ ‚‡‚€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A4\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"3A\">
        <name language=\"en\">REISSUE AND REFUND OVERRIDE</name>
        <name language=\"ru\">REISSUE AND REFUND OVERRIDE</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"02O\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"IE\" subgroup=\"PE\">
        <name language=\"en\">PERSONAL ENTERTAINMENT</name>
        <name language=\"ru\">…‘€‹›… €‡‚‹…—…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"RFP\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">CONFIRMATION DOCS ISSUANCE FEE</name>
        <name language=\"ru\">‘ ‡€ ‚›„€—“ „’‚…†„ „“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD4\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 4</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 4</name>
        <description>Y4</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD1\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">€…’ “‘‹“ƒ</name>
        <description>Y1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0B5\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"YYY\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">TICKET NOTICE</name>
        <name language=\"ru\">‘€‚€  ‹…’“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0CL\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">INTERNET ACCESS</name>
        <name language=\"ru\">„‘’“ ‚ ’……’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AR\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTTR\" carry_on=\"false\" group=\"ML\" subgroup=\"LU\">
        <name language=\"en\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <name language=\"ru\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0BK\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\" subgroup=\"PD\">
        <name language=\"en\">VOUCHER FOR TRAVEL</name>
        <name language=\"ru\">‚€“—… € ……‚‡“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ATX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …„—’’…‹ƒ …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD2\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 2</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 2</name>
        <description>Y2</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD3\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 3</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 3</name>
        <description>Y3</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0F4\" emd_type=\"OTHR\" carry_on=\"false\" group=\"BG\" subgroup=\"IN\">
        <name language=\"en\">STROLLER OR PUSHCHAIR</name>
        <name language=\"ru\">„…’‘€ ‹‘€</name>
        <description>ST</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"LD1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">GRILLED CHICKEN</name>
        <name language=\"ru\">€‹› “›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"F\" rfisc=\"0BB\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT SMALL</name>
        <name language=\"ru\">“€€ ‹ ‚‡ €‹</name>
        <description>AD</description>
        <description>SM</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AI\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\" subgroup=\"BR\">
        <name language=\"en\">BREAKFAST</name>
        <name language=\"ru\">‡€‚’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LQ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL 4</name>
        <name language=\"ru\">’€… 4</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"03P\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"PO\">
        <name language=\"en\">PRIORITY CHECK IN</name>
        <name language=\"ru\">’…’€ …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"CMF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0L8\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">BUNDLE “‘‹“ƒ€</name>
        <description>YY</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0BJ\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BAS\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"062\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">BUSINESS TO FIRST</name>
        <name language=\"ru\">‡…‘ € …‚›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"BBG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"0BG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"TI\">
        <name language=\"en\">TRIP INSURANCE</name>
        <name language=\"ru\">„†€ ‘’€•‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0M6\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">PC OF BAG FROM 23KG UPTO 32KG</name>
        <name language=\"ru\">…‘’ €ƒ€†€ ‚…‘ ’24ƒ „32ƒ</name>
        <description>2V</description>
        <description>32</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0JB\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\">
        <name language=\"en\">“†… ‘ €’€</name>
        <name language=\"ru\">“†… ‘ €’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"F\" rfisc=\"0BD\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ST\" subgroup=\"AP\">
        <name language=\"en\">ADULT POLO SHIRT LARGE</name>
        <name language=\"ru\">“€€ ‹ ‚‡ LARGE</name>
        <description>AD</description>
        <description>LG</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0CC\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ABAG\" carry_on=\"false\" group=\"BG\">
        <name language=\"en\">CHECKED BAG FIRST</name>
        <name language=\"ru\">…‚… ‡€…ƒ‘’ …‘’</name>
        <description>B1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SPF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"PN1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BLACK OR GREEN TEA</name>
        <name language=\"ru\">—€‰ —…›‰ ‹ ‡…‹…›‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"04U\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FIREARMS UP TO 23KG</name>
        <name language=\"ru\">‘’ ƒ…‘’“† „ 50”23ƒ</name>
        <description>FA</description>
        <description>23</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"D\" rfisc=\"0L7\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">NAME CHANGE</name>
        <name language=\"ru\">‘ ‡€ ‡……… …</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"D\" rfisc=\"029\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"TS\" subgroup=\"FT\">
        <name language=\"en\">…‡…‚ …‘’€ „</name>
        <name language=\"ru\">…‡…‚ …‘’€ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF1\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"ASVC\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">PANCAKES</name>
        <name language=\"ru\">‹—</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0AG\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">EXECUTIVE LOUNGE</name>
        <name language=\"ru\">‚ ‡€‹</name>
        <description>EX</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"SW1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">BEEF SANDWICH</name>
        <name language=\"ru\">‘„‚— ‘ ƒ‚„‰</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AN\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"VGML\" carry_on=\"false\" group=\"ML\" subgroup=\"DI\">
        <name language=\"en\">VEGETARIAN DINNER</name>
        <name language=\"ru\">‚…ƒ…’€€‘‰ …„</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"T\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-ST\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0MF\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">LOW FAT MEAL</name>
        <name language=\"ru\">…‡†…€ ™€</name>
        <description>LF</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"C\" rfisc=\"0BT\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"AVIH\" carry_on=\"true\" group=\"PT\" subgroup=\"PC\">
        <name language=\"en\">PET IN CABIN WEIGHT UPTO 10KG</name>
        <name language=\"ru\">†‚’… ‚ ‘€‹… „10ƒ 115‘</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0LE\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"SFML\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">SEAFOOD MEAL 2</name>
        <name language=\"ru\">…„“’› 2</name>
        <description>SO</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BF2\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">CURD CAKES</name>
        <name language=\"ru\">‘›</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BH\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"UMNR\" carry_on=\"false\" group=\"UN\" subgroup=\"MR\">
        <name language=\"en\">UNACCOMPANIED MINOR</name>
        <name language=\"ru\">…‘‚†„€…›‰ ……</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"SP1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">CHANGE</name>
        <name language=\"ru\">‘…€ …‘’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"060\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"997\" emd_type=\"EMD-A\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"07E\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">TIME TO DECIDE FEE</name>
        <name language=\"ru\">”‘€– ‘’‘’ ……‚‡</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0B3\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">MEAL</name>
        <name language=\"ru\">’€…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"DPS\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">DEPOSITS DOWN PAYMENTS</name>
        <name language=\"ru\">…„‹€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"061\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE OF ACCOMP INFANT</name>
        <name language=\"ru\">€ƒ…‰„ ‘‚†„ ‹€„…–€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"CRD\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SF\">
        <name language=\"en\">CRADLE</name>
        <name language=\"ru\">‹‹€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STR\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …‘’€  …ƒ‘’€–</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-S\" booking=\"AUX\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"042\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"UP\">
        <name language=\"en\">UPGRADE</name>
        <name language=\"ru\">‚›…… ‹€‘‘€ ‘‹“†‚€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST1\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
        <name language=\"ru\">1›‰ ‘€‹ ‘ 2ƒ  4›‰ „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0G9\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTCH\" carry_on=\"false\" group=\"ML\">
        <name language=\"en\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <name language=\"ru\">‘„‚— ‘ —…‰ ƒ“„‰</name>
        <description>CN</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STA\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
        <name language=\"ru\">…‘’ ‚ 1 „“ “ ‚›•„‚</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ST2\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ ‘ 2  4 „</name>
        <name language=\"ru\">…‘’ ‘ 2  4 „</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"STW\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">…‘’ “ € ‘ 5 „€</name>
        <name language=\"ru\">…‘’ “ € ‘ 5 „€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A2\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"31\">
        <name language=\"en\">REISSUE OVERRIDE</name>
        <name language=\"ru\">ƒ‚ €‚‹ ……”</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A3\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"33\">
        <name language=\"en\">REFUND OVERRIDE</name>
        <name language=\"ru\">ƒ‚€… €‚‹ ‚‡‚€’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"R\" rfisc=\"0A4\" emd_type=\"ETKT\" carry_on=\"false\" group=\"RO\" subgroup=\"3A\">
        <name language=\"en\">REISSUE AND REFUND OVERRIDE</name>
        <name language=\"ru\">REISSUE AND REFUND OVERRIDE</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"02O\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"IE\" subgroup=\"PE\">
        <name language=\"en\">PERSONAL ENTERTAINMENT</name>
        <name language=\"ru\">…‘€‹›… €‡‚‹…—…</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"RFP\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\">
        <name language=\"en\">CONFIRMATION DOCS ISSUANCE FEE</name>
        <name language=\"ru\">‘ ‡€ ‚›„€—“ „’‚…†„ „“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD4\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 4</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 4</name>
        <description>Y4</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD1\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE</name>
        <name language=\"ru\">€…’ “‘‹“ƒ</name>
        <description>Y1</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"0B5\" emd_type=\"EMD-A\" ssr=\"SEAT\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">…„‚€’…‹›‰ ‚› …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"YYY\" emd_type=\"EMD-S\" carry_on=\"false\">
        <name language=\"en\">TICKET NOTICE</name>
        <name language=\"ru\">‘€‚€  ‹…’“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0CL\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">INTERNET ACCESS</name>
        <name language=\"ru\">„‘’“ ‚ ’……’</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"E\" rfisc=\"0BX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"LG\">
        <name language=\"en\">LOUNGE ACCESS</name>
        <name language=\"ru\">‡…‘ ‡€‹</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"0AR\" emd_type=\"EMD-A\" booking=\"SSR\" ssr=\"PTTR\" carry_on=\"false\" group=\"ML\" subgroup=\"LU\">
        <name language=\"en\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <name language=\"ru\">‚…ƒ…’€€‘ ‘„‚— ‘ ‘“‘</name>
        <description>VG</description>
      </svc>
      <svc company=\"UT\" service_type=\"M\" rfic=\"D\" rfisc=\"0BK\" emd_type=\"EMD-S\" carry_on=\"false\" group=\"TS\" subgroup=\"PD\">
        <name language=\"en\">VOUCHER FOR TRAVEL</name>
        <name language=\"ru\">‚€“—… € ……‚‡“</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"A\" rfisc=\"ATX\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"SA\">
        <name language=\"en\">PRE RESERVED SEAT ASSIGNMENT</name>
        <name language=\"ru\">‚› …„—’’…‹ƒ …‘’€</name>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD2\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 2</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 2</name>
        <description>Y2</description>
      </svc>
      <svc company=\"UT\" service_type=\"F\" rfic=\"G\" rfisc=\"BD3\" emd_type=\"EMD-A\" booking=\"BDL\" carry_on=\"false\" group=\"BD\" subgroup=\"FBD\">
        <name language=\"en\">BUNDLE SERVICE 3</name>
        <name language=\"ru\">€…’ “‘‹“ƒ 3</name>
        <description>Y3</description>
      </svc>
      <free_baggage_norm company=\"UT\" type=\"piece\" rfiscs=\"0C2,0C4,0DD\">
        <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         UPTO50LB 23KG AND62LI 158LCM , PIECE OF BAG UPTO20KG
                           203LCM (0C2)
Free baggage exception:  UPTO55LB 25KG BAGGAGE (0C4)
                     and 1ST SKI SNOWBOARD UPTO 20KG (0DD)
Embargoes
  1ST SKI SNOWBOARD UPTO 20KG (0DD) 2PC and more</text>
        <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1
1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :  €ƒ€† … ‹…… 20ƒ 203‘ (0C2)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ­®ΰ¬γ:  €ƒ€† „ 55” 25ƒ (0C4)
                             ¨ 1… ‹›† ‘“„ „ 20ƒ (0DD)
‡ ―ΰ¥ι¥­® ª ―¥ΰ¥Ά®§ª¥
  1… ‹›† ‘“„ „ 20ƒ (0DD) 2 ¨ ΅®«¥¥</text>
      </free_baggage_norm>
      <free_carry_on_norm company=\"UT\" type=\"unknown\" rfiscs=\"08A,0L5\">
        <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CARRY ON BAGGAGE CHARGES , CABIN BAG UPTO 5KG 40X30X20CM
                       (0L5)
Carry-on exception:  CARRY ON BAGGAGE (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
        <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1
1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               ‹€’€ “—€ ‹€„ , “—€ ‹€„ „
                                        5ƒ 40•30•20‘ (0L5)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ΰγη­γξ ª« ¤μ:  “—€ ‹€„ (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
      </free_carry_on_norm>
    </svc_list>})

$(defmacro SVC_LIST_UT_1PAX_2SEGS
  pax_id
{$(SVC_LIST_UT_1PAX_SEG0 $(pax_id))
$(SVC_LIST_UT_1PAX_SEG1 $(pax_id))})

$(defmacro FREE_NORMS_UT_1PAX_SEG0
  pax_id
{    <free_baggage_norm passenger-id=\"$(pax_id)\" segment-id=\"0\" company=\"UT\" type=\"piece\" rfiscs=\"0C2,0C4,0DD\">
      <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         UPTO50LB 23KG AND62LI 158LCM , PIECE OF BAG UPTO20KG
                           203LCM (0C2)
Free baggage exception:  UPTO55LB 25KG BAGGAGE (0C4)
                     and 1ST SKI SNOWBOARD UPTO 20KG (0DD)
Embargoes
  1ST SKI SNOWBOARD UPTO 20KG (0DD) 2PC and more</text>
      <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1
1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :  €ƒ€† … ‹…… 20ƒ 203‘ (0C2)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ­®ΰ¬γ:  €ƒ€† „ 55” 25ƒ (0C4)
                             ¨ 1… ‹›† ‘“„ „ 20ƒ (0DD)
‡ ―ΰ¥ι¥­® ª ―¥ΰ¥Ά®§ª¥
  1… ‹›† ‘“„ „ 20ƒ (0DD) 2 ¨ ΅®«¥¥</text>
    </free_baggage_norm>
    <free_carry_on_norm passenger-id=\"$(pax_id)\" segment-id=\"0\" company=\"UT\" type=\"unknown\" rfiscs=\"08A,0L5\">
      <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CARRY ON BAGGAGE CHARGES , CABIN BAG UPTO 5KG 40X30X20CM
                       (0L5)
Carry-on exception:  CARRY ON BAGGAGE (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
      <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1
1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               ‹€’€ “—€ ‹€„ , “—€ ‹€„ „
                                        5ƒ 40•30•20‘ (0L5)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ΰγη­γξ ª« ¤μ:  “—€ ‹€„ (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
    </free_carry_on_norm>})

$(defmacro FREE_NORMS_UT_1PAX_SEG1
  pax_id
{    <free_baggage_norm passenger-id=\"$(pax_id)\" segment-id=\"1\" company=\"UT\" type=\"piece\" rfiscs=\"0C2,0C4,0DD\">
      <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         UPTO50LB 23KG AND62LI 158LCM , PIECE OF BAG UPTO20KG
                           203LCM (0C2)
Free baggage exception:  UPTO55LB 25KG BAGGAGE (0C4)
                     and 1ST SKI SNOWBOARD UPTO 20KG (0DD)
Embargoes
  1ST SKI SNOWBOARD UPTO 20KG (0DD) 2PC and more</text>
      <text language=\"ru\">®ΰ¬  ΅¥α―« β­®£® ΅ £ ¦  1
1-… ¬¥αβ® ΅¥α―« β­®£® ΅ £ ¦ :  €ƒ€† … ‹…… 20ƒ 203‘ (0C2)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ­®ΰ¬γ:  €ƒ€† „ 55” 25ƒ (0C4)
                             ¨ 1… ‹›† ‘“„ „ 20ƒ (0DD)
‡ ―ΰ¥ι¥­® ª ―¥ΰ¥Ά®§ª¥
  1… ‹›† ‘“„ „ 20ƒ (0DD) 2 ¨ ΅®«¥¥</text>
    </free_baggage_norm>
    <free_carry_on_norm passenger-id=\"$(pax_id)\" segment-id=\"1\" company=\"UT\" type=\"unknown\" rfiscs=\"08A,0L5\">
      <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CARRY ON BAGGAGE CHARGES , CABIN BAG UPTO 5KG 40X30X20CM
                       (0L5)
Carry-on exception:  CARRY ON BAGGAGE (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
      <text language=\"ru\">¥α―« β­ ο ΰγη­ ο ª« ¤μ 1
1-… ¬¥αβ® ΰγη­®© ª« ¤¨:               ‹€’€ “—€ ‹€„ , “—€ ‹€„ „
                                        5ƒ 40•30•20‘ (0L5)
¥ Άε®¤¨β Ά ΅¥α―« β­γξ ΰγη­γξ ª« ¤μ:  “—€ ‹€„ (08A)
                PARAMETERS
                -WEIGHT UP TO 5 KG
                -SIZE 40X30X20CM
                PLEASE REFER TO WWW.UTAIR.RU FOR FULL DETAILS OF
                UT BAGGAGE POLICY.</text>
    </free_carry_on_norm>})


$(defmacro FREE_NORMS_UT_1PAX_2SEGS
  pax_id
{$(FREE_NORMS_UT_1PAX_SEG0 $(pax_id))
$(FREE_NORMS_UT_1PAX_SEG1 $(pax_id))})


$(defmacro SVC_AVAILABILITY_RESPONSE_UT_1PAX_1SEG
  pax_id
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_availability>
$(SVC_LIST_UT_1PAX_SEG0 $(pax_id))
  </svc_availability>
</answer>})
}
)

$(defmacro SVC_AVAILABILITY_RESPONSE_UT_1PAX_2SEGS
  pax_id
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_availability>
$(SVC_LIST_UT_1PAX_2SEGS $(pax_id))
  </svc_availability>
</answer>})
}
)

$(defmacro SVC_AVAILABILITY_RESPONSE_UT_2PAXES_2SEGS
  pax_id1
  pax_id2
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_availability>
$(SVC_LIST_UT_1PAX_2SEGS $(pax_id1))
$(SVC_LIST_UT_1PAX_2SEGS $(pax_id2))
  </svc_availability>
</answer>})
}
)

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_1SEG
  pax_id
  svc_list
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_payment_status>
$(FREE_NORMS_UT_1PAX_SEG0 $(pax_id))
$(svc_list)
  </svc_payment_status>
</answer>})
}
)

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_UT_1PAX_2SEGS
  pax_id
  svc_list
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_payment_status>
$(FREE_NORMS_UT_1PAX_2SEGS $(pax_id))
$(svc_list)
  </svc_payment_status>
</answer>})
}
)

$(defmacro SVC_PAYMENT_STATUS_RESPONSE_UT_2PAXES_2SEGS
  pax_id1
  pax_id2
  svc_list
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_payment_status>
$(FREE_NORMS_UT_1PAX_2SEGS $(pax_id1))
$(FREE_NORMS_UT_1PAX_2SEGS $(pax_id2))
$(svc_list)
  </svc_payment_status>
</answer>})
}
)
