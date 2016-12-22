#include "collect_data.h"

using namespace std;

const string req_save_pax =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinSavePax>"
"      <agent_stat_period>54</agent_stat_period>"
"      <transfer/>"
"      <segments>"
"        <segment>"
"          <point_dep>2944842</point_dep>"
"          <point_arv>2944843</point_arv>"
"          <airp_dep></airp_dep>"
"          <airp_arv></airp_arv>"
"          <class></class>"
"          <status>K</status>"
"          <wl_type/>"
"          <mark_flight>"
"            <airline></airline>"
"            <flt_no>2009</flt_no>"
"            <suffix/>"
"            <scd>01.03.2016 00:00:00</scd>"
"            <airp_dep></airp_dep>"
"            <pr_mark_norms>0</pr_mark_norms>"
"          </mark_flight>"
"          <passengers>"
"            <pax>"
"              <pax_id>29401268</pax_id>"
"              <surname></surname>"
"              <name></name>"
"              <pers_type></pers_type>"
"              <seat_no/>"
"              <preseat_no/>"
"              <seat_type/>"
"              <seats>2</seats>"
"              <ticket_no>2982408012592</ticket_no>"
"              <coupon_no/>"
"              <ticket_rem>TKNA</ticket_rem>"
"              <ticket_confirm>0</ticket_confirm>"
"              <document>"
"                <type>P</type>"
"                <issue_country>RUS</issue_country>"
"                <no>7774441110</no>"
"                <nationality>RUS</nationality>"
"                <birth_date>01.05.1976 00:00:00</birth_date>"
"                <gender>F</gender>"
"                <surname></surname>"
"                <first_name></first_name>"
"              </document>"
"              <doco/>"
"              <addresses/>"
"              <subclass></subclass>"
"              <bag_pool_num/>"
"              <transfer/>"
"              <rems>"
"                <rem>"
"                  <rem_code>FOID</rem_code>"
"                  <rem_text>FOID PP7774441110</rem_text>"
"                </rem>"
"                <rem>"
"                  <rem_code>PRSA</rem_code>"
"                  <rem_text>PRSA</rem_text>"
"                </rem>"
"                <rem>"
"                  <rem_code>OTHS</rem_code>"
"                  <rem_text>OTHS HK1 DOCS/7774441110/PS</rem_text>"
"                </rem>"
"              </rems>"
"              <norms/>"
"            </pax>"
"          </passengers>"
"          <paid_bag_emd/>"
"        </segment>"
"      </segments>"
"      <hall>1</hall>"
"      <paid_bags/>"
"    </TCkinSavePax>"
"  </query>"
"</term>";

const string req_save_pax0 =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <TCkinSavePax> "
"      <agent_stat_period>23</agent_stat_period> "
"      <transfer/> "
"      <segments> "
"        <segment> "
"          <point_dep>2943595</point_dep> "
"          <point_arv>2943596</point_arv> "
"          <airp_dep></airp_dep> "
"          <airp_arv></airp_arv> "
"          <class></class> "
"          <status>K</status> "
"          <wl_type/> "
"          <mark_flight> "
"            <airline></airline> "
"            <flt_no>969</flt_no> "
"            <suffix/> "
"            <scd>25.02.2016 00:00:00</scd> "
"            <airp_dep></airp_dep> "
"            <pr_mark_norms>0</pr_mark_norms> "
"          </mark_flight> "
"          <passengers> "
"            <pax> "
"              <pax_id>29398829</pax_id> "
"              <surname></surname> "
"              <name></name> "
"              <pers_type></pers_type> "
"              <seat_no/> "
"              <preseat_no/> "
"              <seat_type/> "
"              <seats>2</seats> "
"              <ticket_no>2982408012596</ticket_no> "
"              <coupon_no>1</coupon_no> "
"              <ticket_rem>TKNE</ticket_rem> "
"              <ticket_confirm>0</ticket_confirm> "
"              <document> "
"                <type>P</type> "
"                <issue_country>RUS</issue_country> "
"                <no>7774441110</no> "
"                <nationality>RUS</nationality> "
"                <birth_date>01.05.1976 00:00:00</birth_date> "
"                <gender>F</gender> "
"                <surname></surname> "
"                <first_name></first_name> "
"              </document> "
"              <doco/> "
"              <addresses/> "
"              <subclass></subclass> "
"              <bag_pool_num/> "
"              <transfer/> "
"              <rems> "
"                <rem> "
"                  <rem_code>FOID</rem_code> "
"                  <rem_text>FOID PP7774441110</rem_text> "
"                </rem> "
"                <rem> "
"                  <rem_code>OTHS</rem_code> "
"                  <rem_text>OTHS HK1 DOCS/7774441110/PS</rem_text> "
"                </rem> "
"              </rems> "
"              <norms/> "
"            </pax> "
"          </passengers> "
"          <paid_bag_emd/> "
"        </segment> "
"      </segments> "
"      <hall>2</hall> "
"      <paid_bags/> "
"    </TCkinSavePax> "
"  </query> "
"</term> ";

const string req_print_bp =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='print' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <GetGRPPrintData>"
"      <op_type>PRINT_BP</op_type>"
"      <grp_id>32593143</grp_id>"
"      <pr_all>1</pr_all>"
"      <dev_model>ML390</dev_model>"
"      <fmt_type>EPSON</fmt_type>"
"      <prnParams>"
"        <pr_lat>0</pr_lat>"
"        <encoding>CP866</encoding>"
"        <offset>20</offset>"
"        <top>0</top>"
"      </prnParams>"
"      <clientData>"
"        <gate/>"
"      </clientData>"
"    </GetGRPPrintData>"
"  </query>"
"</term>";

const string req_logon =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='MainDCS' ver='1' opr='' screen='MAINDCS.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <UserLogon> "
"      <term_version>201602-0177911</term_version> "
"      <lang dictionary_lang='RU' dictionary_checksum='-1973420663'>RU</lang> "
"      <userr>DEN</userr> "
"      <passwd>DEN</passwd> "
"      <airlines/> "
"      <devices> "
"        <operation type='PRINT_BP'> "
"          <dev_model_code>ML390</dev_model_code> "
"          <sess_params type='LPT'> "
"            <addr>LPT3</addr> "
"          </sess_params> "
"          <fmt_params type='EPSON'> "
"            <encoding>CP866</encoding> "
"            <left>20</left> "
"            <pr_lat>0</pr_lat> "
"            <top>0</top> "
"          </fmt_params> "
"          <model_params type='ML390'> "
"            <pr_stock>0</pr_stock> "
"          </model_params> "
"          <events/> "
"        </operation> "
"        <operation type='PRINT_BT'> "
"          <dev_model_code>ML390</dev_model_code> "
"          <sess_params type='LPT'> "
"            <addr>LPT3</addr> "
"          </sess_params> "
"          <fmt_params type='EPSON'> "
"            <encoding>CP866</encoding> "
"            <left>20</left> "
"            <pr_lat>0</pr_lat> "
"            <top>0</top> "
"          </fmt_params> "
"          <model_params type='ML390'> "
"            <pr_stock>0</pr_stock> "
"          </model_params> "
"          <events/> "
"        </operation> "
"        <operation type='PRINT_BR'> "
"          <dev_model_code>ML390</dev_model_code> "
"          <sess_params type='LPT'> "
"            <addr>LPT3</addr> "
"          </sess_params> "
"          <fmt_params type='EPSON'> "
"            <encoding>CP866</encoding> "
"            <left>20</left> "
"            <pr_lat>0</pr_lat> "
"            <top>0</top> "
"          </fmt_params> "
"          <model_params type='ML390'> "
"            <pr_stock>0</pr_stock> "
"          </model_params> "
"          <events/> "
"        </operation> "
"        <operation type='PRINT_FLT'> "
"          <dev_model_code>DRV PRINT</dev_model_code> "
"          <sess_params type='WDP'> "
"            <addr/> "
"          </sess_params> "
"          <fmt_params type='FRX'> "
"            <export_bmp>0</export_bmp> "
"            <paper_height/> "
"            <pr_lat>0</pr_lat> "
"          </fmt_params> "
"          <events/> "
"        </operation> "
"        <operation type='PRINT_TLG'> "
"          <dev_model_code>DRV PRINT</dev_model_code> "
"          <sess_params type='WDP'> "
"            <addr/> "
"          </sess_params> "
"          <fmt_params type='FRX'> "
"            <export_bmp>0</export_bmp> "
"            <paper_height/> "
"            <pr_lat>0</pr_lat> "
"          </fmt_params> "
"          <events/> "
"        </operation> "
"      </devices> "
"      <command_line_params> "
"        <param>STAND</param> "
"      </command_line_params> "
"    </UserLogon> "
"  </query> "
"</term> ";

const string req_create_flt =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='sopp' ver='1' opr='DEN' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <WriteDests> "
"      <data> "
"        <move_id>-2147483648</move_id> "
"        <canexcept>1</canexcept> "
"        <reference/> "
"        <dests> "
"          <dest> "
"            <modify/> "
"            <airp></airp> "
"            <airline></airline> "
"            <flt_no>2001</flt_no> "
"            <craft>5</craft> "
"            <bort>TEST</bort> "
"            <scd_out>29.02.2016 11:00:00</scd_out> "
"            <trip_type>ฏ</trip_type> "
"            <pr_tranzit>0</pr_tranzit> "
"            <pr_reg>0</pr_reg> "
"          </dest> "
"          <dest> "
"            <modify/> "
"            <airp></airp> "
"            <scd_in>29.02.2016 12:00:00</scd_in> "
"            <pr_tranzit>0</pr_tranzit> "
"            <pr_reg>0</pr_reg> "
"          </dest> "
"        </dests> "
"      </data> "
"    </WriteDests> "
"  </query> "
"</term> ";

// create flt answer
//001456750304+86 12 @@@@@@ 0029865
//<term>
//  <answer execute_time="84" lang="RU" handle="0">
//    <command>
//        <message lexema_id="MSG.DATA_SAVED" code="0"> ญญ๋ฅ ใแฏฅ่ญฎ แฎๅเ ญฅญ๋</message>
//    </command>
//    <data>
//        <move_id>1086098</move_id>
//        <dests>
//            <dest>
//                <point_id>2944539</point_id>
//                <point_num>0</point_num>
//                <airp></airp>
//                <airpId></airpId>
//                <airline></airline>
//                <flt_no>2001</flt_no>
//                <craft>5</craft>
//                <bort>TEST</bort>
//                <scd_out>29.02.2016 11:00:00</scd_out>
//                <trip_type>ฏ</trip_type>
//                <pr_tranzit>0</pr_tranzit>
//                <pr_reg>1</pr_reg>
//            </dest>
//            <dest>
//                <point_id>2944540</point_id>
//                <point_num>1</point_num>
//                <first_point>2944539</first_point>
//                <airp></airp>
//                <airpId></airpId>
//                <scd_in>29.02.2016 12:00:00</scd_in>
//                <pr_tranzit>0</pr_tranzit>
//                <pr_reg>0</pr_reg>
//            </dest>
//        </dests>
//    </data>
//  </answer>
//</term>

const string req_create_salon =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='salonform' ver='1' opr='DEN' screen='CENT.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <Write>"
"      <trip_id>2945517</trip_id>"
"      <comp_id>507275</comp_id>"
"      <initcomp>1</initcomp>"
"      <refcompon>"
"        <ref>      172  COLLECTOR</ref>"
"        <pr_lat>1</pr_lat>"
"        <classes>172</classes>"
"        <ctype>"
"          <type>cBase</type>"
"        </ctype>"
"      </refcompon>"
"      <salons pr_lat_seat='1'>"
"        <placelist num='0'>"
"          <place>"
"            <x>0</x>"
"            <y>0</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>1</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>0</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>1</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>0</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>1</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>0</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>1</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>1</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>2</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>1</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>2</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>1</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>2</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>1</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>2</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>2</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>3</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>2</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>3</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>2</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>3</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>2</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>3</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>3</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>4</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>3</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>4</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>3</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>4</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>3</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>4</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>4</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>5</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>4</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>5</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>4</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>5</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>4</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>5</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>5</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>6</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>5</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>6</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>5</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>6</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>5</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>6</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>6</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>7</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>6</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>7</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>6</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>7</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>6</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>7</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>7</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>8</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>7</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>8</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>7</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>8</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>7</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>8</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>8</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>9</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>8</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>9</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>8</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>9</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>8</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>9</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>9</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>10</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>9</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>10</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>9</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>10</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>9</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>10</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>10</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>11</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>10</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>11</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>10</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>11</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>10</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>11</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>11</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>12</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>11</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>12</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>11</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>12</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>11</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>12</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>12</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>13</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>12</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>13</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>12</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>13</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>12</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>13</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>13</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>14</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>13</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>14</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>13</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>14</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>13</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>14</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>14</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>15</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>14</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>15</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>14</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>15</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>14</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>15</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>15</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>16</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>15</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>16</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>15</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>16</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>15</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>16</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>16</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>17</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>16</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>17</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>16</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>17</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>16</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>17</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>17</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>18</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>17</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>18</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>17</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>18</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>17</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>18</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>18</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>19</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>18</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>19</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>18</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>19</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>18</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>19</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>19</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>20</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>19</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>20</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>19</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>20</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>19</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>20</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>20</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>21</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>20</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>21</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>20</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>21</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>20</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>21</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>21</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>22</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>21</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>22</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>21</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>22</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>21</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>22</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>22</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>23</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>22</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>23</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>22</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>23</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>22</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>23</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>23</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>24</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>23</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>24</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>23</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>24</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>23</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>24</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>24</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>25</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>24</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>25</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>24</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>25</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>24</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>25</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>25</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>26</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>25</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>26</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>25</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>26</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>25</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>26</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>26</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>27</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>26</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>27</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>26</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>27</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>26</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>27</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>27</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>28</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>27</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>28</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>27</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>28</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>27</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>28</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>28</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>29</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>28</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>29</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>28</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>29</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>28</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>29</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>29</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>30</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>29</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>30</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>29</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>30</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>29</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>30</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>30</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>31</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>30</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>31</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>30</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>31</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>30</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>31</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>31</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>32</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>31</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>32</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>31</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>32</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>31</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>32</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>32</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>33</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>32</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>33</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>32</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>33</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>32</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>33</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>33</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>34</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>33</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>34</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>33</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>34</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>33</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>34</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>34</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>35</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>34</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>35</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>34</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>35</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>34</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>35</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>35</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>36</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>35</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>36</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>35</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>36</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>35</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>36</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>36</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>37</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>36</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>37</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>36</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>37</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>36</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>37</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>37</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>38</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>37</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>38</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>37</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>38</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>37</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>38</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>38</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>39</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>38</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>39</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>38</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>39</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>38</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>39</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>39</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>40</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>39</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>40</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>39</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>40</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>39</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>40</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>40</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>41</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>40</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>41</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>40</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>41</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>40</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>41</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>41</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>42</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>41</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>42</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>41</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>42</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>41</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>42</yname>"
"          </place>"
"          <place>"
"            <x>0</x>"
"            <y>42</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>A</xname>"
"            <yname>43</yname>"
"          </place>"
"          <place>"
"            <x>1</x>"
"            <y>42</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>B</xname>"
"            <yname>43</yname>"
"          </place>"
"          <place>"
"            <x>2</x>"
"            <y>42</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>C</xname>"
"            <yname>43</yname>"
"          </place>"
"          <place>"
"            <x>3</x>"
"            <y>42</y>"
"            <elem_type></elem_type>"
"            <class></class>"
"            <xname>D</xname>"
"            <yname>43</yname>"
"          </place>"
"        </placelist>"
"      </salons>"
"    </Write>"
"  </query>"
"</term>";

const string req_load_pnl =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='Telegram' ver='1' opr='DEN' screen='TLG.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <LoadTlg> "
"      <tlg_text>MOWDENI\n"
".MOWDENI 180519\n"
"PNL\n"
"UT2003/29FEB DME PART1\n"
"CFG/008C114Y\n"
"RBD C/CJZIDA Y/YRSTEQGNBXWUOVHLK\n"
"AVAIL\n"
" DME  AER\n"
"C006\n"
"Y033\n"
"-VKO000C\n"
"-VKO000J\n"
"-VKO000Z\n"
"-VKO000I\n"
"-VKO000D\n"
"-VKO002A\n"
"1/ \n"
".L/0M71MG/UT\n"
".L/T0TBK6/1H\n"
".R/TKNE HK1 2986145372512/1-1/ \n"
".R/OTHS HK1 FQTSTATUS SILVER-1/ \n"
".R/FQTV UT 3000001768263-1/ \n"
".R/DOCS HK1/P/RUS/7400177695/RUS/20OCT72/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7400177695/RUS/20OCT72// \n"
".RN//M-1/ \n"
".R/FOID PP7400177695-1/ \n"
"1/ \n"
".L/0M68F0/UT\n"
".L/T0SFNZ/1H\n"
".R/TKNE HK1 2986145367246/1-1/ \n"
".R/FQTV UT 0003000000063535-1/ \n"
".R/DOCS HK1/P/RUS/7106500403/RUS/14JUL80/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7106500403/RUS/14JUL80// \n"
".RN//M-1/ \n"
".R/FOID PP7106500403-1/ \n"
"-VKO001Y\n"
"1/ \n"
".L/0D25BB/UT\n"
".L/SZBC2P/1H\n"
".R/TKNE HK1 2986144661291/2-1/ \n"
".R/DOCS HK1/P/RUS/4611547922/RUS/20DEC91/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4611547922/RUS/20DEC91// /M\n"
".RN/-1/ \n"
".R/FOID PP4611547922-1/ \n"
"-VKO000R\n"
"-VKO000S\n"
"-VKO000T\n"
"-VKO000E\n"
"-VKO012Q\n"
"1/ \n"
".C/\n"
".L/0M7CGD/UT\n"
".L/T0TXD2/1H\n"
".R/TKNE HK1 2982415357639/1-1/ \n"
".R/DOCS HK1/P/RUS/2505433154/RUS/11FEB61/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 2505433154/RUS/11FEB61// /M\n"
".RN/-1/ \n"
".R/FOID PP2505433154-1/ \n"
"1/ \n"
".L/0G1KF6/UT\n"
".L/SW8N01/1H\n"
".O/UT333Q19VKONNM1205HK\n"
".R/TKNE HK1 2982415326937/3-1/ \n"
".R/DOCS HK1/P/RUS/5506047872/RUS/01JAN87/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 5506047872/RUS/01JAN87// /F\n"
".RN/-1/ \n"
".R/FOID PP5506047872-1/ \n"
"1KHACHATUROV/GEORGY\n"
".L/0G1KGD/UT\n"
".L/SW8N39/1H\n"
".R/TKNE HK1 2982415326941/2-1KHACHATUROV/GEORGY\n"
".R/DOCS HK1/P/RUS/515723940/RUS/23MAR84/M/10OCT20/KHACHATUROV\n"
".RN//GEORGY-1KHACHATUROV/GEORGY\n"
".R/PSPT HK1 515723940/RUS/23MAR84/KHACHATUROV/GEORGY/M\n"
".RN/-1KHACHATUROV/GEORGY\n"
".R/FOID PP515723940-1KHACHATUROV/GEORGY\n"
"1/ \n"
".L/0G1KCG/UT\n"
".L/SW8MTN/1H\n"
".O/UT595Q19VKOUSK1150HK\n"
".R/TKNE HK1 2982415326933/3-1/ \n"
".R/DOCS HK1/P/RUS/8700103825/RUS/29JUL80/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 8700103825/RUS/29JUL80//\n"
".RN/ /F-1/ \n"
".R/FOID PP8700103825-1/ \n"
"-VKO012Q\n"
"1/ \n"
".L/0D1M2D/UT\n"
".L/SZBMC0/1H\n"
".R/TKNE HK1 2982415306751/2-1/ \n"
".R/DOCS HK1/P/RUS/4605960711/RUS/11AUG59/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4605960711/RUS/11AUG59// \n"
".RN//M-1/ \n"
".R/FOID PP4605960711-1/ \n"
"1/ \n"
".L/0MB2D8/UT\n"
".L/T0XLPR/1H\n"
".O/UT389Q19VKOKGD1200HK\n"
".O2/UT390Q21KGDVKO1400HK\n"
".R/TKNE HK1 2982415360339/1-1/ \n"
".R/DOCS HK1/P/RUS/6704137966/RUS/26JUL72/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 6704137966/RUS/26JUL72// /F\n"
".RN/-1/ \n"
".R/FOID PP6704137966-1/ \n"
"1/ \n"
".L/0M3MB6/UT\n"
".L/T0RVFL/1H\n"
".R/TKNE HK1 2982415357299/1-1/ \n"
".R/DOCS HK1/P/RUS/8709403138/RUS/14NOV64/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 8709403138/RUS/14NOV64// /M\n"
".RN/-1/ \n"
".R/FOID PP8709403138-1/ \n"
"1/ \n"
".L/0MB31D/UT\n"
".L/T0XM14/1H\n"
".O/UT389Q19VKOKGD1200HK\n"
".O2/UT390Q21KGDVKO1400HK\n"
".R/TKNE HK1 2982415360342/1-1/ \n"
".R/DOCS HK1/P/RUS/7106437090/RUS/01AUG61/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7106437090/RUS/01AUG61// /F\n"
".RN/-1/ \n"
".R/FOID PP7106437090-1/ \n"
"1/ -A2\n"
".L/0G1K5L/UT\n"
".L/SW8M93/1H\n"
".O/UT381Q19VKOLED0925HK\n"
".R/TKNE HK1 2982415326923/1-1/ \n"
".R/DOCS HK1/P/RUS/4007033544/RUS/15DEC86/F//\n"
".RN///-1/ \n"
".R/PSPT HK1 4007033544/RUS/15DEC86//\n"
".RN/ /F-1/ \n"
".R/FOID PP4007033544-1/ \n"
"1/ \n"
".L/0M8D72/UT\n"
".L/T0FLX9/1H\n"
".R/TKNE HK1 2982415358675/1-1/ \n"
".R/DOCS HK1/P/RUS/6711121982/RUS/07DEC84/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 6711121982/RUS/07DEC84// /M\n"
".RN/-1/ \n"
".R/FOID PP6711121982-1/ \n"
"1/ -A2\n"
".L/0G1K5L/UT\n"
".L/SW8M93/1H\n"
".R/TKNE HK1 2982415326924/1-1/ \n"
".R/DOCS HK1/P/RUS/2704908606/RUS/30MAR82/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 2704908606/RUS/30MAR82// /M\n"
".RN/-1/ \n"
".R/FOID PP2704908606-1/ \n"
"-VKO000G\n"
"-VKO000N\n"
"-VKO000B\n"
"-VKO000X\n"
"-VKO000W\n"
"-VKO001U\n"
"-VKO001U\n"
"1/ \n"
".L/0LKK44/UT\n"
".L/T0L71B/1H\n"
".O/UT595U19VKOUSK1150HK\n"
".R/TKNE HK1 2986145303929/1-1/ \n"
".R/DOCS HK1/P/RUS/6511150013/RUS/14JUN85/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 6511150013/RUS/14JUN85//\n"
".RN/ /M-1/ \n"
".R/FOID PP6511150013-1/ \n"
"-VKO004O\n"
"1/ \n"
".L/0K5CL8/UT\n"
".L/SWX54F/1H\n"
".O/UT557O19VKOMCX1210HK\n"
".R/TKNE HK1 2986145094517/1-1/ \n"
".R/OTHS HK1 FQTSTATUS BASIC-1/ \n"
".R/FQTV UT 4000000006177-1/ \n"
".R/DOCS HK1/P/RUS/8202921959/RUS/07JAN77/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 8202921959/RUS/07JAN77//\n"
".RN/ /M-1/ \n"
".R/FOID PP8202921959-1/ \n"
"1/ \n"
".L/0M93KG/UT\n"
".L/T0FW2F/1H\n"
".R/TKNE HK1 2986145389177/1-1/ \n"
".R/FQTV UT 000400000003315-1/ \n"
".R/DOCS HK1/P/RUS/7100236693/RUS/23DEC55/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7100236693/RUS/23DEC55//\n"
".RN/ /M-1/ \n"
".R/FOID PP7100236693-1/ \n"
"1PAVLYUK/SERGEY MR\n"
".L/0M217G/UT\n"
".L/MMIUPG/1S\n"
".I/UT230N18URJTJM1710HK\n"
".O/UT375O19VKOSCW1700HK\n"
".R/TKNE HK1 2981687512278/2-1PAVLYUK/SERGEY MR\n"
".R/DOCS HK1/P/RUS/8715696293/RUS/15OCT70/M/12DEC21/PAVLYUK\n"
".RN//SERGEY-1PAVLYUK/SERGEY MR\n"
".R/PSPT HK1 8715696293/RUS/15OCT70/PAVLYUK/SERGEY MR/M-1PAVLYUK\n"
".RN//SERGEY MR\n"
".R/FOID PP8715696293-1PAVLYUK/SERGEY MR\n"
"1/ \n"
".L/0MBBM4/UT\n"
".L/T0XX6B/1H\n"
".R/TKNE HK1 2986145400076/1-1/ \n"
".R/DOCS HK1/P/RUS/4501232065/RUS/12APR54/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4501232065/RUS/12APR54// /M\n"
".RN/-1/ \n"
".R/FOID PP4501232065-1/ \n"
"-VKO001V\n"
"1/ \n"
".L/0M71MD/UT\n"
".L/T0TBD7/1H\n"
".O/UT399V19VKOGRV1040HK\n"
".R/TKNE HK1 2986145372447/1-1/ \n"
".R/DOCS HK1/P/RUS/1804621728/RUS/19MAY60/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 1804621728/RUS/19MAY60// /F\n"
".RN/-1/ \n"
".R/FOID PP1804621728-1/ \n"
"-VKO032H\n"
"1/ \n"
".L/0M8B8D/UT\n"
".L/T0FK8Z/1H\n"
".R/TKNE HK1 2986145403990/1-1/ \n"
".R/DOCS HK1/P/UKR/037781/UKR/07DEC68/M/17FEB17/\n"
".RN///-1/ \n"
".R/PSPT HK1 NP037781/UKR/07DEC68//\n"
".RN/ /M-1/ \n"
".R/FOID PPNP037781-1/ \n"
"-VKO032H\n"
"1/ \n"
".L/0M4L16/UT\n"
".L/T0S0R3/1H\n"
".R/TKNE HK1 2986145356739/1-1/ \n"
".R/OTHS HK1 FQTSTATUS BASIC-1/ \n"
".R/FQTV UT 3000003948657-1/ \n"
".R/DOCS HK1/P/RUS/7102734114/RUS/04OCT77/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7102734114/RUS/04OCT77// /M\n"
".RN/-1/ \n"
".R/FOID PP7102734114-1/ \n"
"1AZIMLI/MIRSADIQ-C3\n"
".L/0M6726/UT\n"
".L/T0STL2/1H\n"
".R/CHLD HK1 21JUL11-1AZIMLI/MIRSADIQ\n"
".R/TKNE HK1 2986145366801/1-1AZIMLI/MIRSADIQ\n"
".R/DOCS HK1/P/UKR/C00273662/UKR/21JUL11/M/16FEB21/AZIMLI\n"
".RN//MIRSADIQ-1AZIMLI/MIRSADIQ\n"
".R/PSPT HK1 NPC00273662/UKR/21JUL11/AZIMLI/MIRSADIQ/M-1AZIMLI\n"
".RN//MIRSADIQ\n"
".R/FOID PPNPC00273662-1AZIMLI/MIRSADIQ\n"
"1AZIMOVA/KAMILA-C3\n"
".R/TKNE HK1 2986145366799/1-1AZIMOVA/KAMILA\n"
".R/DOCS HK1/P/UKR/P4916451/UKR/03SEP82/F/16FEB21/AZIMOVA/KAMILA\n"
".RN/-1AZIMOVA/KAMILA\n"
".R/PSPT HK1 NPP4916451/UKR/03SEP82/AZIMOVA/KAMILA/F-1AZIMOVA\n"
".RN//KAMILA\n"
".R/FOID PPNPP4916451-1AZIMOVA/KAMILA\n"
"1AZIMOVA/NARGIZ-C3\n"
".R/CHLD HK1 09JAN07-1AZIMOVA/NARGIZ\n"
".R/TKNE HK1 2986145366800/1-1AZIMOVA/NARGIZ\n"
".R/DOCS HK1/P/UKR/C00273665/UKR/09JAN07/F/16FEB21/AZIMOVA\n"
".RN//NARGIZ-1AZIMOVA/NARGIZ\n"
".R/PSPT HK1 NPC00273665/UKR/09JAN07/AZIMOVA/NARGIZ/F-1AZIMOVA\n"
".RN//NARGIZ\n"
".R/FOID PPNPC00273665-1AZIMOVA/NARGIZ\n"
"1BATISHCHEV/EDUARD MR-E2\n"
".L/0MC174/UT\n"
".L/ZQHLXR/1A\n"
".R/TKNE HK1 2981651957460/1-1BATISHCHEV/EDUARD MR\n"
".R/DOCS HK1/P/RUS/4615938791/RUS/12JUL70/M/14DEC20/BATISHCHEV\n"
".RN//EDUARD-1BATISHCHEV/EDUARD MR\n"
".R/PSPT HK1 4615938791/RUS/12JUL70/BATISHCHEV/EDUARD MR/M\n"
".RN/-1BATISHCHEV/EDUARD MR\n"
".R/FOID PP4615938791-1BATISHCHEV/EDUARD MR\n"
"1BELASH/HENADZI\n"
".L/0M5422/UT\n"
".L/T0S64G/1H\n"
".R/TKNE HK1 2986145359420/1-1BELASH/HENADZI\n"
".R/DOCS HK1/P/UKR/MP3520046/UKR/23JUL69/M/11JUL24/BELASH\n"
".RN//HENADZI-1BELASH/HENADZI\n"
".R/PSPT HK1 NPMP3520046/UKR/23JUL69/BELASH/HENADZI/M-1BELASH\n"
".RN//HENADZI\n"
".R/FOID PPNPMP3520046-1BELASH/HENADZI\n"
"1/ \n"
".L/0MD3BD/UT\n"
".L/T0CPTS/1H\n"
".R/TKNE HK1 2986145410133/1-1/ \n"
".R/DOCS HK1/P/RUS/7108679890/RUS/22DEC89/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7108679890/RUS/22DEC89//\n"
".RN/ /M-1/ \n"
".R/FOID PP7108679890-1/ \n"
"1/\n"
".L/0MC438/UT\n"
".L/T0C67G/1H\n"
".O/UT161K19VKOGOJ1010HK\n"
".R/TKNE HK1 2986145404677/1-1/\n"
".R/OTHS HK1 FQTSTATUS BASIC-1/\n"
".R/FQTV UT 0006000000009189-1/\n"
".R/DOCS HK1/P/RUS/7411794371/RUS/14DEC59/M///\n"
".RN/-1/\n"
".R/PSPT HK1 7411794371/RUS/14DEC59///M\n"
".RN/-1/\n"
".R/FOID PP7411794371-1/\n"
"-VKO032H\n"
"1/ \n"
".L/0G497L/UT\n"
".L/SWBK8C/1H\n"
".R/TKNE HK1 2986145388435/1-1/ \n"
".R/OTHS HK0 RE UT XX1 TJMVKO0454H19FEB-1/ \n"
".R/OTHS HK0 RE UT/// RUB1500.00 RUB1500.00-1/\n"
".RN/ \n"
".R/DOCS HK1/P/RUS/731154145/RUS/06MAY58/M/15OCT20/\n"
".RN///-1/ \n"
".R/PSPT HK1 731154145/RUS/06MAY58// /M\n"
".RN/-1/ \n"
".R/FOID PP731154145-1/ \n"
"1/ \n"
".L/0M5G1B/UT\n"
".L/T0SK0W/1H\n"
".R/INFT HK1 /  15NOV14-1/\n"
".RN/ \n"
".R/TKNE HK1 2986145363088/1-1/ \n"
".R/TKNE HK1 INF2986145363089/1-1/ \n"
".R/DOCS HK1/P/RUS/7111885929/RUS/20OCT77/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7111885929/RUS/20OCT77// /F\n"
".RN/-1/ \n"
".R/FOID PP7111885929-1/ \n"
".R/DOCS HK1/F/RUS/II593064/RUS/15NOV14/MI///\n"
".RN//-1/ \n"
"1/ \n"
".L/0MC4KG/UT\n"
".L/T0C6TV/1H\n"
".R/TKNE HK1 2986145404919/1-1/ \n"
".R/DOCS HK1/P/RUS/6702839443/RUS/30JUN82/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 6702839443/RUS/30JUN82// /M\n"
".RN/-1/ \n"
".R/FOID PP6702839443-1/ \n"
"1/ \n"
".L/0M861D/UT\n"
".L/T0F9WC/1H\n"
".R/TKNE HK1 2986145382575/1-1/ \n"
".R/DOCS HK1/P/RUS/7108630635/RUS/08SEP88/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7108630635/RUS/08SEP88// /M\n"
".RN/-1/ \n"
".R/FOID PP7108630635-1/ \n"
"1/ \n"
".L/0M8G90/UT\n"
".L/T0FP64/1H\n"
".R/TKNE HK1 2986145386012/1-1/ \n"
".R/DOCS HK1/P/RUS/8206140069/RUS/12SEP81/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 8206140069/RUS/12SEP81// /M\n"
".RN/-1/ \n"
".R/FOID PP8206140069-1/ \n"
"1/ \n"
".L/0M5F3L/UT\n"
".L/T0SGCV/1H\n"
".R/TKNE HK1 2986145362805/1-1/ \n"
".R/DOCS HK1/P/RUS/7114107539/RUS/29SEP69/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7114107539/RUS/29SEP69// \n"
".RN//F-1/ \n"
".R/FOID PP7114107539-1/ \n"
"1/ \n"
".L/0MB86G/UT\n"
".L/T0XT0B/1H\n"
".R/TKNE HK1 2986145399088/1-1/ \n"
".R/DOCS HK1/P/RUS/7402281835/RUS/17JUN81/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7402281835/RUS/17JUN81// \n"
".RN//M-1/ \n"
".R/FOID PP7402281835-1/ \n"
"-VKO032H\n"
"1/ \n"
".L/0M3G74/UT\n"
".L/T0R8LC/1H\n"
".R/TKNE HK1 2986145348218/1-1/ \n"
".R/OTHS HK1 FQTSTATUS BASIC-1/ \n"
".R/FQTV UT 0003000000039584-1/ \n"
".R/DOCS HK1/P/RUS/7007971874/RUS/22MAY87/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7007971874/RUS/22MAY87//\n"
".RN/ /F-1/ \n"
".R/FOID PP7007971874-1/ \n"
"1/ \n"
".L/0MBBK6/UT\n"
".L/T0XX67/1H\n"
".R/TKNE HK1 2986145400146/1-1/ \n"
".R/DOCS HK1/P/RUS/7111890065/RUS/24SEP91/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7111890065/RUS/24SEP91// /F\n"
".RN/-1/ \n"
".R/FOID PP7111890065-1/ \n"
"1/ \n"
".L/0M77CL/UT\n"
".L/T0TPMT/1H\n"
".R/TKNE HK1 2986145375124/1-1/ \n"
".R/DOCS HK1/P/RUS/5802612738/RUS/25MAR77/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 5802612738/RUS/25MAR77//\n"
".RN/ /M-1/ \n"
".R/FOID PP5802612738-1/ \n"
"1KUDINOVA/IRINA MRS-E2\n"
".L/0MC174/UT\n"
".L/ZQHLXR/1A\n"
".R/TKNE HK1 2981651957459/1-1KUDINOVA/IRINA MRS\n"
".R/DOCS HK1/P/RUS/6599286552/RUS/11NOV74/F/14DEC20/KUDINOVA\n"
".RN//IRINA-1KUDINOVA/IRINA MRS\n"
".R/PSPT HK1 6599286552/RUS/11NOV74/KUDINOVA/IRINA MRS/F\n"
".RN/-1KUDINOVA/IRINA MRS\n"
".R/FOID PP6599286552-1KUDINOVA/IRINA MRS\n"
"1/ \n"
".L/0MC7L8/UT\n"
".L/T0C939/1H\n"
".R/TKNE HK1 2986145406006/1-1/ \n"
".R/OTHS HK1 FQTSTATUS BASIC-1/ \n"
".R/FQTV UT 0006000000024901-1/ \n"
".R/DOCS HK1/P/RUS/7111902827/RUS/30DEC66/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7111902827/RUS/30DEC66// /M\n"
".RN/-1/ \n"
".R/FOID PP7111902827-1/ \n"
"1/ \n"
".L/0M85FB/UT\n"
".L/T0F9D7/1H\n"
".R/TKNE HK1 2986145382423/1-1/ \n"
".R/DOCS HK1/P/RUS/7107549817/RUS/19APR87/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7107549817/RUS/19APR87// /M\n"
".RN/-1/ \n"
".R/FOID PP7107549817-1/ \n"
"1/ \n"
".L/0M9158/UT\n"
".L/T0FXG8/1H\n"
".R/TKNE HK1 2986145388036/1-1/ \n"
".R/DOCS HK1/P/RUS/7102553175/RUS/01SEP71/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7102553175/RUS/01SEP71// /M\n"
".RN/-1/ \n"
".R/FOID PP7102553175-1/ \n"
"-VKO032H\n"
"1/ \n"
".L/0M1B1L/UT\n"
".L/T0NMSW/1H\n"
".R/TKNE HK1 2986145393123/1-1/ \n"
".R/OTHS HK0 RE UT XX1 TJMVKO0454H19FEB-1/\n"
".RN/ .-1/ \n"
".R/OTHS HK0 RE UT/// TAX YQ RUB1500.00 RUB1500.00-1\n"
".RN// \n"
".R/DOCS HK1/P/RUS/4509261895/RUS/02SEP87/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 4509261895/RUS/02SEP87// \n"
".RN//F-1/ \n"
".R/FOID PP4509261895-1/ \n"
"1/ \n"
".L/0M8K12/UT\n"
".L/T0FPX4/1H\n"
".R/TKNE HK1 2986145386402/1-1/ \n"
".R/DOCS HK1/P/RUS/4507877085/RUS/09SEP84/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4507877085/RUS/09SEP84// \n"
".RN//M-1/ \n"
".R/FOID PP4507877085-1/ \n"
"1SEMENIUK/ALEKSANDR KONSTANTINOVICH\n"
".L/0MB18D/UT\n"
".L/T0XKD9/1H\n"
".R/TKNE HK1 2986145395986/1-1SEMENIUK/ALEKSANDR KONSTANTINOVICH\n"
".R/DOCS HK1/P/RUS/7105359100/RUS/23SEP85/M//SEMENIUK/ALEKSANDR\n"
".RN//KONSTANTINOVICH-1SEMENIUK/ALEKSANDR KONSTANTINOVICH\n"
".R/PSPT HK1 7105359100/RUS/23SEP85/SEMENIUK/ALEKSANDR\n"
".RN/ KONSTANTINOVICH/M-1SEMENIUK/ALEKSANDR KONSTANTINOVICH\n"
".R/FOID PP7105359100-1SEMENIUK/ALEKSANDR KONSTANTINOVICH\n"
"1/ -B2\n"
".L/0M3K6L/UT\n"
".L/T0R9N0/1H\n"
".R/TKNE HK1 2986145348744/1-1/ \n"
".R/DOCS HK1/P/RUS/7104279637/RUS/30DEC84/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7104279637/RUS/30DEC84// \n"
".RN//F-1/ \n"
".R/FOID PP7104279637-1/ \n"
"1SLADKOVA/FAINASERGEEVNA\n"
".L/0M7F8G/UT\n"
".L/T0TZV7/1H\n"
".R/TKNE HK1 2986145377721/1-1SLADKOVA/FAINASERGEEVNA\n"
".R/DOCS HK1/P/RUS/7113039972/RUS/26OCT93/M//SLADKOVA\n"
".RN//FAINASERGEEVNA-1SLADKOVA/FAINASERGEEVNA\n"
".R/PSPT HK1 7113039972/RUS/26OCT93/SLADKOVA/FAINASERGEEVNA/M\n"
".RN/-1SLADKOVA/FAINASERGEEVNA\n"
".R/FOID PP7113039972-1SLADKOVA/FAINASERGEEVNA\n"
"1/ \n"
".L/0MCKCB/UT\n"
".L/T0CKV9/1H\n"
".R/TKNE HK1 2986145408190/1-1/ \n"
".R/OTHS HK1 FQTSTATUS SILVER-1/ \n"
".R/FQTV UT 3000002635511-1/ \n"
".R/DOCS HK1/P/RUS/7111903562/RUS/22JAN67/F//\n"
".RN///-1/ \n"
".R/PSPT HK1 7111903562/RUS/22JAN67//\n"
".RN/ /F-1/ \n"
".R/FOID PP7111903562-1/ \n"
"-VKO032H\n"
"1/ -D2\n"
".L/0M7374/UT\n"
".L/T0TV59/1H\n"
".O/UT277H19VKOEGO1550HK\n"
".R/TKNE HK1 2986145384331/1-1/ \n"
".R/OTHS HK1      I594758\n"
".RN/-1/ \n"
".R/DOCS HK1/P/UKR/639507/UKR/25SEP91/M/25SEP27/\n"
".RN///-1/ \n"
".R/PSPT HK1 NP639507/UKR/25SEP91// \n"
".RN//M-1/ \n"
".R/FOID PPNP639507-1/ \n"
"1/ -D2\n"
".R/INFT HK1 /  31MAR14-1\n"
".RN// \n"
".R/TKNE HK1 2986145384332/1-1/ \n"
".R/TKNE HK1 INF2986145384333/1-1/ \n"
".R/DOCS HK1/P/UKR/046196/UKR/25MAR93/F/25MAR27/\n"
".RN///-1/ \n"
".R/PSPT HK1 NP046196/UKR/25MAR93//\n"
".RN/ /F-1/ \n"
".R/FOID PPNP046196-1/ \n"
".R/DOCS HK1/F/UKR/I594758/UKR/31MAR14/MI///\n"
".RN//-1/ \n"
"1/ -B2\n"
".L/0M3K6L/UT\n"
".L/T0R9N0/1H\n"
".R/TKNE HK1 2986145348743/1-1/ \n"
".R/DOCS HK1/P/RUS/7404520987/RUS/17FEB85/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7404520987/RUS/17FEB85// \n"
".RN//M-1/ \n"
".R/FOID PP7404520987-1/ \n"
"-VKO003L\n"
"1/ -F2\n"
".L/0L1C2B/UT\n"
".L/T054CT/1H\n"
".O/UT161L19VKOGOJ1010HK\n"
".R/TKNE HK1 2986145196488/1-1/ \n"
".R/DOCS HK1/P/RUS/2211743163/RUS/08OCT91/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 2211743163/RUS/08OCT91// \n"
".RN//F-1/ \n"
".R/FOID PP2211743163-1/ \n"
"1/ -F2\n"
".R/TKNE HK1 2986145196487/1-1/ \n"
".R/DOCS HK1/P/RUS/2204487017/RUS/15APR54/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 2204487017/RUS/15APR54// /M\n"
".RN/-1/ \n"
".R/FOID PP2204487017-1/ \n"
"1ZAKHARCHUK/OLESYA ALEKSANDROVNA MS\n"
".L/0C8D38/UT\n"
".L/YGLQ77/1A\n"
".O/UT161L19VKOGOJ1010HK\n"
".R/TKNE HK1 2981651530227/3-1ZAKHARCHUK/OLESYA ALEKSANDROVNA MS\n"
".R/DOCS HK1/P/RUS/2203528975/RUS/07DEC78/F/22JAN26/ZAKHARCHUK\n"
".RN//OLESYA/ALEKSANDROVNA-1ZAKHARCHUK/OLESYA ALEKSANDROVNA MS\n"
".R/PSPT HK1 2203528975/RUS/07DEC78/ZAKHARCHUK/OLESYA\n"
".RN/ ALEKSANDROVNA MS/F-1ZAKHARCHUK/OLESYA ALEKSANDROVNA MS\n"
".R/FOID PP2203528975-1ZAKHARCHUK/OLESYA ALEKSANDROVNA MS\n"
"-VKO027K\n"
"-VKO027K\n"
"1/ \n"
".L/0KL3L4/UT\n"
".L/T02FB8/1H\n"
".R/TKNE HK1 2986145170116/2-1/ \n"
".R/DOCS HK1/P/RUS/4606196044/RUS/23NOV83/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4606196044/RUS/23NOV83//\n"
".RN/ /M-1/ \n"
".R/FOID PP4606196044-1/ \n"
"1BUSHKOV/ALEXANDER MR\n"
".L/0KG9CD/UT\n"
".L/ZE3BIM/1A\n"
".R/TKNE HK1 2982409203457/2-1BUSHKOV/ALEXANDER MR\n"
".R/DOCS HK1/P/RUS/6902507586/RUS/24AUG79/M/07FEB26/BUSHKOV\n"
".RN//ALEXANDER-1BUSHKOV/ALEXANDER MR\n"
".R/PSPT HK1 6902507586/RUS/24AUG79/BUSHKOV/ALEXANDER MR/M\n"
".RN/-1BUSHKOV/ALEXANDER MR\n"
".R/FOID PP6902507586-1BUSHKOV/ALEXANDER MR\n"
"1BYKOVICH/ALEKSANDR MR\n"
".L/0C2G8B/UT\n"
".L/VRAYOP/1S\n"
".O/UT485K19VKOKUF1035HK\n"
".R/TKNE HK1 2981687278781/1-1BYKOVICH/ALEKSANDR MR\n"
".R/DOCS HK1/P/RUS/3615019611/RUS/17DEC69/M/22NOV20/BYKOVICH\n"
".RN//ALEKSANDR-1BYKOVICH/ALEKSANDR MR\n"
".R/PSPT HK1 3615019611/RUS/17DEC69/BYKOVICH/ALEKSANDR MR/M\n"
".RN/-1BYKOVICH/ALEKSANDR MR\n"
".R/FOID PP3615019611-1BYKOVICH/ALEKSANDR MR\n"
"1/-H2\n"
".L/0M1832/UT\n"
".L/T0NG92/1H\n"
".R/TKNE HK1 2986145329071/1-1/\n"
".R/DOCS HK1/P/RUS/7114076707/RUS/25MAY69/M///\n"
".RN/-1/\n"
".R/PSPT HK1 7114076707/RUS/25MAY69///M-1\n"
".RN//\n"
".R/FOID PP7114076707-1/\n"
"1/-H2\n"
".R/TKNE HK1 2986145329072/1-1/\n"
".R/DOCS HK1/P/RUS/7100232437/RUS/15SEP72/F///\n"
".RN/-1/\n"
".R/PSPT HK1 7100232437/RUS/15SEP72///F-1\n"
".RN//\n"
".R/FOID PP7100232437-1/\n"
"1/ -G2\n"
".L/012B3G/UT\n"
".L/SNCTTG/1H\n"
".R/TKNE HK1 2986142572684/1-1/ \n"
".R/DOCS HK1/P/RUS/5210979638/RUS/05JAN97/F//\n"
".RN///-1/ \n"
".R/PSPT HK1 5210979638/RUS/05JAN97//\n"
".RN/ /F-1/ \n"
".R/FOID PP5210979638-1/ \n"
"1/ \n"
".L/0FDKG2/UT\n"
".L/SW41K4/1H\n"
".R/TKNE HK1 2986144885735/2-1/ \n"
".R/DOCS HK1/P/RUS/4514563223/RUS/11MAR69/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4514563223/RUS/11MAR69// /M\n"
".RN/-1/ \n"
".R/FOID PP4514563223-1/ \n"
"1/ \n"
".L/0KM3D8/UT\n"
".L/T03N04/1H\n"
".R/TKNE HK1 2986145180027/2-1/ \n"
".R/DOCS HK1/P/RUS/4610873366/RUS/24NOV81/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 4610873366/RUS/24NOV81// /M\n"
".RN/-1/ \n"
".R/FOID PP4610873366-1/ \n"
"-VKO027K\n"
"1/ \n"
".L/0M1DF8/UT\n"
".L/T0NT8M/1H\n"
".R/TKNE HK1 2986145331667/1-1/ \n"
".R/DOCS HK1/P/RUS/6710099999/RUS/15APR91/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 6710099999/RUS/15APR91// /F\n"
".RN/-1/ \n"
".R/FOID PP6710099999-1/ \n"
"1KOROLKOV/VLADIMIR\n"
".L/0K1C4B/UT\n"
".L/SWSDD7/1H\n"
".O/UT557K19VKOMCX1210HK\n"
".R/TKNE HK1 2986145064082/1-1KOROLKOV/VLADIMIR\n"
".R/DOCS HK1/P/RUS/6507117862/RUS/13JUN86/M//KOROLKOV/VLADIMIR\n"
".RN/-1KOROLKOV/VLADIMIR\n"
".R/PSPT HK1 6507117862/RUS/13JUN86/KOROLKOV/VLADIMIR/M\n"
".RN/-1KOROLKOV/VLADIMIR\n"
".R/FOID PP6507117862-1KOROLKOV/VLADIMIR\n"
"1/ -G2\n"
".L/012B3G/UT\n"
".L/SNCTTG/1H\n"
".R/TKNE HK1 2986142572683/1-1/ \n"
".R/FQTV UT 0003000000095962-1/ \n"
".R/DOCS HK1/P/RUS/5208644875/RUS/18JUN88/F//\n"
".RN///-1/ \n"
".R/PSPT HK1 5208644875/RUS/18JUN88//\n"
".RN/ /F-1/ \n"
".R/FOID PP5208644875-1/ \n"
"1KRUSINSKIY/DANIIL\n"
".L/0L7M9G/UT\n"
".L/ZJEAI3/1A\n"
".R/TKNE HK1 2981651855873/1-1KRUSINSKIY/DANIIL\n"
".R/FQTV UT 6000000456123-1KRUSINSKIY/DANIIL\n"
".R/DOCS HK1/P/RUS/7102658515/RUS/20JUL77/M/10AUG02/KRUSINSKIY\n"
".RN//DANIIL-1KRUSINSKIY/DANIIL\n"
".R/PSPT HK1 7102658515/RUS/20JUL77/KRUSINSKIY/DANIIL/M\n"
".RN/-1KRUSINSKIY/DANIIL\n"
".R/FOID PP7102658515-1KRUSINSKIY/DANIIL\n"
"1/\n"
".L/0K6440/UT\n"
".L/SWXMFB/1H\n"
".R/TKNE HK1 2986145098042/1-1/\n"
".R/DOCS HK1/P/RUS/5609868008/RUS/31JAN89/M///\n"
".RN/-1/\n"
".R/PSPT HK1 5609868008/RUS/31JAN89///M-1\n"
".RN//\n"
".R/FOID PP5609868008-1/\n"
"1/ \n"
".L/0M1DCB/UT\n"
".L/T0NT3W/1H\n"
".R/TKNE HK1 2986145331598/1-1/ \n"
".R/DOCS HK1/P/RUS/7109735844/RUS/15NOV82/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 7109735844/RUS/15NOV82// /M\n"
".RN/-1/ \n"
".R/FOID PP7109735844-1/ \n"
"1MAMMADOV/NATIG\n"
".L/0LB132/UT\n"
".L/T0BWV0/1H\n"
".O/UT743H19VKOKVD1050HK\n"
".R/TKNE HK1 2986145261121/1-1MAMMADOV/NATIG\n"
".R/DOCS HK1/P/AZE/P4728901/AZE/02MAR69/M/06DEC21/MAMMADOV/NATIG\n"
".RN/-1MAMMADOV/NATIG\n"
".R/PSPT HK1 NPP4728901/AZE/02MAR69/MAMMADOV/NATIG/M-1MAMMADOV\n"
".RN//NATIG\n"
".R/FOID PPNPP4728901-1MAMMADOV/NATIG\n"
"1/ \n"
".L/0LB172/UT\n"
".L/T0BWB7/1H\n"
".R/TKNE HK1 2986145261227/1-1/ \n"
".R/DOCS HK1/P/RUS/9815598418/RUS/12JUN70/M//\n"
".RN///-1/ \n"
".R/PSPT HK1 9815598418/RUS/12JUN70//\n"
".RN/ /M-1/ \n"
".R/FOID PP9815598418-1/ \n"
"-VKO027K\n"
"1/ \n"
".L/0LM9BD/UT\n"
".L/T0MBTZ/1H\n"
".R/TKNE HK1 2986145317838/2-1/ \n"
".R/DOCS HK1/P/RUS/6701400790/RUS/12SEP71/M//\n"
".RN///-1/ \n"
".R/PSPT HK1 6701400790/RUS/12SEP71//\n"
".RN/ /M-1/ \n"
".R/FOID PP6701400790-1/ \n"
"1/ \n"
".L/0M1F24/UT\n"
".L/T0NTFG/1H\n"
".R/TKNE HK1 2986145331855/1-1/ \n"
".R/DOCS HK1/P/RUS/6704265258/RUS/12SEP81/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 6704265258/RUS/12SEP81// /F\n"
".RN/-1/ \n"
".R/FOID PP6704265258-1/ \n"
"1/ \n"
".L/0L58D4/UT\n"
".L/T07FT9/1H\n"
".R/TKNE HK1 2986145225375/1-1/ \n"
".R/DOCS HK1/P/RUS/4509860036/RUS/13SEP63/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 4509860036/RUS/13SEP63// /F\n"
".RN/-1/ \n"
".R/FOID PP4509860036-1/ \n"
"1/ \n"
".L/0L88K2/UT\n"
".L/T09R8T/1H\n"
".O/UT395V19VKOOGZ1125HK\n"
".R/TKNE HK1 2986145248275/1-1/ \n"
".R/DOCS HK1/P/RUS/7106509562/RUS/13APR87/F///\n"
".RN//-1/ \n"
".R/PSPT HK1 7106509562/RUS/13APR87// \n"
".RN//F-1/ \n"
".R/FOID PP7106509562-1/ \n"
"1/ \n"
".L/0DF794/UT\n"
".L/SZPG0R/1H\n"
".O/UT373V19VKOMRV1110HK\n"
".R/TKNE HK1 2986144748902/1-1/ \n"
".R/DOCS HK1/P/RUS/0700352201/RUS/09DEC75/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 0700352201/RUS/09DEC75// /M\n"
".RN/-1/ \n"
".R/FOID PP0700352201-1/ \n"
"1SKRZYPEK/MARCIN MR\n"
".L/0LK00G/UT\n"
".L/ZLVIXZ/1A\n"
".O/UT389H19VKOKGD1200HK\n"
".R/TKNE HK1 1691621464179/1-1SKRZYPEK/MARCIN MR\n"
".R/DOCS HK1/P/POL/EE0627287/POL/15SEP79/M/22NOV22/SKRZYPEK\n"
".RN//MARCIN-1SKRZYPEK/MARCIN MR\n"
".R/PSPT HK1 ZAEE0627287/POL/15SEP79/SKRZYPEK/MARCIN MR/M\n"
".RN/-1SKRZYPEK/MARCIN MR\n"
".R/FOID PPZAEE0627287-1SKRZYPEK/MARCIN MR\n"
"1SMIRNOVA/NADEZHDA MRS\n"
".L/0KG9M0/UT\n"
".L/ZEB3NT/1A\n"
".R/TKNE HK1 2982409203460/2-1SMIRNOVA/NADEZHDA MRS\n"
".R/DOCS HK1/P/RUS/4508936979/RUS/20JAN82/F/10FEB21/SMIRNOVA\n"
".RN//NADEZHDA-1SMIRNOVA/NADEZHDA MRS\n"
".R/PSPT HK1 4508936979/RUS/20JAN82/SMIRNOVA/NADEZHDA MRS/F\n"
".RN/-1SMIRNOVA/NADEZHDA MRS\n"
".R/FOID PP4508936979-1SMIRNOVA/NADEZHDA MRS\n"
"1/ \n"
".L/0LLD9D/UT\n"
".L/T0LXL3/1H\n"
".R/TKNE HK1 2986145310661/1-1/ \n"
".R/DOCS HK1/P/RUS/1502906995/RUS/07JUN80/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 1502906995/RUS/07JUN80// /M\n"
".RN/-1/ \n"
".R/FOID PP1502906995-1/ \n"
"-VKO027K\n"
"1/ \n"
".L/0G626G/UT\n"
".L/SWVV7C/1H\n"
".R/TKNE HK1 2986144966322/1-1/ \n"
".R/OTHS HK1 FQTSTATUS BASIC-1/ \n"
".R/FQTV UT 3000003420848-1/ \n"
".R/DOCS HK1/P/RUS/6705633808/RUS/09JUN86/M///\n"
".RN//-1/ \n"
".R/PSPT HK1 6705633808/RUS/09JUN86// /M\n"
".RN/-1/ \n"
".R/FOID PP6705633808-1/ \n"
"1VINICHENKO/NIKOLAY MR\n"
".L/0G5678/UT\n"
".L/Y5B6MQ/1A\n"
".O/UT373V19VKOMRV1110HK\n"
".R/TKNE HK1 2981651605434/1-1VINICHENKO/NIKOLAY MR\n"
".R/FQTV UT 3000002349881-1VINICHENKO/NIKOLAY MR\n"
".R/DOCS HK1/P/RUS/0702886943/RUS/11MAY52/M/11MAY52/VINICHENKO\n"
".RN//NIKOLAY-1VINICHENKO/NIKOLAY MR\n"
".R/PSPT HK1 0702886943/RUS/11MAY52/VINICHENKO/NIKOLAY MR/M\n"
".RN/-1VINICHENKO/NIKOLAY MR\n"
".R/FOID PP0702886943-1VINICHENKO/NIKOLAY MR\n"
"1/\n"
".L/0LD2B8/UT\n"
".L/T0GGXM/1H\n"
".R/TKNE HK1 2986145276098/1-1/\n"
".R/DOCS HK1/P/RUS/7103890702/RUS/24JUN55/F///\n"
".RN/-1/\n"
".R/PSPT HK1 7103890702/RUS/24JUN55///F-1\n"
".RN//\n"
".R/FOID PP7103890702-1/\n"
"ENDPNL\n"
"</tlg_text> "
"    </LoadTlg> "
"  </query> "
"</term> ";

const string req_open_reg =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='sopp' ver='1' opr='DEN' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <WriteTrips> "
"      <trips> "
"        <trip> "
"          <point_id>2944573</point_id> "
"          <tripstages> "
"            <stage> "
"              <stage_id>10</stage_id> "
"              <act>29.02.2016 18:34:00</act> "
"            </stage> "
"            <stage> "
"              <stage_id>20</stage_id> "
"              <act>29.02.2016 18:34:00</act> "
"            </stage> "
"          </tripstages> "
"          <stations> "
"            <work_mode mode=''> "
"              <name>31</name> "
"            </work_mode> "
"          </stations> "
"        </trip> "
"      </trips> "
"    </WriteTrips> "
"  </query> "
"</term> ";

const string req_prep_reg =
"<?xml version='1.0' encoding='cp866'?> "
"<term> "
"  <query handle='0' id='prepreg' ver='1' opr='DEN' screen='PREPREG.EXE' mode='STAND' lang='RU' term_id='595428461'> "
"    <CrsDataApplyUpdates> "
"      <point_id>2944573</point_id> "
"      <question>1</question> "
"      <crsdata/> "
"      <trip_sets> "
"        <pr_tranzit>0</pr_tranzit> "
"        <pr_tranz_reg>0</pr_tranz_reg> "
"        <pr_block_trzt>0</pr_block_trzt> "
"        <pr_free_seating>0</pr_free_seating> "
"        <pr_check_load>0</pr_check_load> "
"        <pr_overload_reg>1</pr_overload_reg> "
"        <pr_exam>0</pr_exam> "
"        <pr_exam_check_pay>0</pr_exam_check_pay> "
"        <pr_check_pay>1</pr_check_pay> "
"        <pr_reg_with_tkn>0</pr_reg_with_tkn> "
"        <pr_reg_with_doc>0</pr_reg_with_doc> "
"        <pr_reg_without_tkna>0</pr_reg_without_tkna> "
"        <auto_weighing>0</auto_weighing> "
"        <apis_control>1</apis_control> "
"        <apis_manual_input>0</apis_manual_input> "
"        <piece_concept>1</piece_concept> "
"      </trip_sets> "
"      <tripcounters/> "
"    </CrsDataApplyUpdates> "
"  </query> "
"</term> ";

const string req_brd_with_reg =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='cache' ver='1' opr='DEN' screen='MAINDCS.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <cache_apply>"
"      <params>"
"        <code>TRIP_BRD_WITH_REG</code>"
"        <interface_ver>681796922</interface_ver>"
"        <data_ver>-1</data_ver>"
"      </params>"
"      <sqlparams>"
"        <point_id type='0'>2944839</point_id>"
"      </sqlparams>"
"      <rows>"
"        <row index='0' status='modified'>"
"          <col index='0'>"
"            <old>2944839</old>"
"            <new>2944839</new>"
"          </col>"
"          <col index='1'>"
"            <old/>"
"            <new/>"
"          </col>"
"          <col index='2'>"
"            <old/>"
"            <new/>"
"          </col>"
"          <col index='3'>"
"            <old>0</old>"
"            <new>1</new>"
"          </col>"
"        </row>"
"      </rows>"
"    </cache_apply>"
"  </query>"
"</term>";

const string req_trip_exam_with_brd =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='cache' ver='1' opr='DEN' screen='MAINDCS.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <cache_apply>"
"      <params>"
"        <code>TRIP_EXAM_WITH_BRD</code>"
"        <interface_ver>681796925</interface_ver>"
"        <data_ver>-1</data_ver>"
"      </params>"
"      <sqlparams>"
"        <point_id type='0'>2944839</point_id>"
"      </sqlparams>"
"      <rows>"
"        <row index='0' status='modified'>"
"          <col index='0'>"
"            <old>2944839</old>"
"            <new>2944839</new>"
"          </col>"
"          <col index='1'>"
"            <old>1</old>"
"            <new>1</new>"
"          </col>"
"          <col index='2'>"
"            <old> ซ 1</old>"
"            <new> ซ 1</new>"
"          </col>"
"          <col index='3'>"
"            <old>0</old>"
"            <new>1</new>"
"          </col>"
"        </row>"
"      </rows>"
"    </cache_apply>"
"  </query>"
"</term>";

const string req_save_bag =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinSavePax>"
"      <agent_stat_period>18</agent_stat_period>"
"      <segments>"
"        <segment>"
"          <point_dep>2945093</point_dep>"
"          <point_arv>2945094</point_arv>"
"          <airp_dep></airp_dep>"
"          <airp_arv></airp_arv>"
"          <class></class>"
"          <grp_id>36792</grp_id>"
"          <tid>12234083</tid>"
"          <passengers>"
"            <pax>"
"              <pax_id>29401913</pax_id>"
"              <surname></surname>"
"              <name></name>"
"              <pers_type></pers_type>"
"              <refuse/>"
"              <ticket_no>2982408012570</ticket_no>"
"              <coupon_no/>"
"              <ticket_rem>TKNA</ticket_rem>"
"              <ticket_confirm>0</ticket_confirm>"
"              <document>"
"                <type>P</type>"
"                <issue_country>RUS</issue_country>"
"                <no>7774441110</no>"
"                <nationality>RUS</nationality>"
"                <birth_date>01.05.1976 00:00:00</birth_date>"
"                <gender>F</gender>"
"                <surname></surname>"
"                <first_name></first_name>"
"              </document>"
"              <doco/>"
"              <addresses/>"
"              <bag_pool_num>1</bag_pool_num>"
"              <subclass></subclass>"
"              <tid>12234083</tid>"
"            </pax>"
"          </passengers>"
"          <paid_bag_emd/>"
"        </segment>"
"      </segments>"
"      <hall>1</hall>"
"      <bag_refuse/>"
"      <value_bags/>"
"      <bags>"
"        <bag>"
"          <num>1</num>"
"          <bag_type>0DG</bag_type>"
"          <pr_cabin>0</pr_cabin>"
"          <amount>1</amount>"
"          <weight>10</weight>"
"          <value_bag_num/>"
"          <pr_liab_limit>0</pr_liab_limit>"
"          <to_ramp>0</to_ramp>"
"          <using_scales>0</using_scales>"
"          <is_trfer>0</is_trfer>"
"          <bag_pool_num>1</bag_pool_num>"
"        </bag>"
"        <bag>"
"          <num>2</num>"
"          <bag_type>0DG</bag_type>"
"          <pr_cabin>0</pr_cabin>"
"          <amount>1</amount>"
"          <weight>5</weight>"
"          <value_bag_num/>"
"          <pr_liab_limit>0</pr_liab_limit>"
"          <to_ramp>0</to_ramp>"
"          <using_scales>0</using_scales>"
"          <is_trfer>0</is_trfer>"
"          <bag_pool_num>1</bag_pool_num>"
"        </bag>"
"      </bags>"
"      <tags pr_print='1'/>"
"    </TCkinSavePax>"
"  </query>"
"</term>";

const string req_takeoff =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='sopp' ver='1' opr='DEN' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <WriteDests>"
"      <data>"
"        <move_id>1086141</move_id>"
"        <canexcept>1</canexcept>"
"        <reference/>"
"        <dests>"
"          <dest>"
"            <modify/>"
"            <point_id>2945079</point_id>"
"            <point_num>0</point_num>"
"            <airp></airp>"
"            <airline></airline>"
"            <flt_no>1</flt_no>"
"            <craft>5</craft>"
"            <bort>TEST</bort>"
"            <scd_out>02.03.2016 11:00:00</scd_out>"
"            <act_out>02.03.2016 20:04:00</act_out>"
"            <trip_type>ฏ</trip_type>"
"            <pr_tranzit>0</pr_tranzit>"
"            <pr_reg>1</pr_reg>"
"          </dest>"
"          <dest>"
"            <modify/>"
"            <point_id>2945080</point_id>"
"            <point_num>1</point_num>"
"            <first_point>2945079</first_point>"
"            <airp></airp>"
"            <scd_in>02.03.2016 12:00:00</scd_in>"
"            <est_in>02.03.2016 21:04:00</est_in>"
"            <trip_type>ฏ</trip_type>"
"            <pr_tranzit>0</pr_tranzit>"
"            <pr_reg>0</pr_reg>"
"          </dest>"
"        </dests>"
"      </data>"
"    </WriteDests>"
"  </query>"
"</term>";

const string req_load_tag_packs =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <LoadTagPacks/>"
"  </query>"
"</term>";

const string req_tag_type =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='cache' ver='1' opr='DEN' screen='MAINDCS.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <cache_apply>"
"      <params>"
"        <code>TRIP_BT</code>"
"        <interface_ver>681474970</interface_ver>"
"        <data_ver>-1</data_ver>"
"      </params>"
"      <sqlparams>"
"        <point_id type='0'>2944839</point_id>"
"      </sqlparams>"
"      <rows>"
"        <row index='0' status='inserted'>"
"          <sqlparams>"
"            <point_id type='0'>2944839</point_id>"
"          </sqlparams>"
"          <col index='0'/>"
"          <col index='1'></col>"
"          <col index='2'></col>"
"        </row>"
"      </rows>"
"    </cache_apply>"
"  </query>"
"</term>";

const string req_save_pax_bag =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinSavePax>"
"      <agent_stat_period>22</agent_stat_period>"
"      <segments>"
"        <segment>"
"          <point_dep>4405302</point_dep>"
"          <point_arv>4405303</point_arv>"
"          <airp_dep></airp_dep>"
"          <airp_arv></airp_arv>"
"          <class></class>"
"          <grp_id>2591827</grp_id>"
"          <tid>18814754</tid>"
"          <passengers>"
"            <pax>"
"              <pax_id>33069822</pax_id>"
"              <surname></surname>"
"              <name/>"
"              <pers_type></pers_type>"
"              <refuse/>"
"              <ticket_no/>"
"              <coupon_no/>"
"              <ticket_rem/>"
"              <ticket_confirm>0</ticket_confirm>"
"              <document/>"
"              <doco/>"
"              <addresses/>"
"              <bag_pool_num>1</bag_pool_num>"
"              <subclass></subclass>"
"              <tid>18814754</tid>"
"            </pax>"
"          </passengers>"
"          <paid_bag_emd/>"
"        </segment>"
"      </segments>"
"      <hall>1</hall>"
"      <bag_refuse/>"
"      <value_bags/>"
"      <bags>"
"        <bag>"
"          <num>1</num>"
"          <bag_type>04</bag_type>"
"          <pr_cabin>0</pr_cabin>"
"          <amount>1</amount>"
"          <weight>20</weight>"
"          <value_bag_num/>"
"          <pr_liab_limit>0</pr_liab_limit>"
"          <to_ramp>0</to_ramp>"
"          <using_scales>0</using_scales>"
"          <is_trfer>0</is_trfer>"
"          <bag_pool_num>1</bag_pool_num>"
"        </bag>"
"      </bags>"
"      <tags pr_print='1'/>"
"      <unaccomps/>"
"    </TCkinSavePax>"
"  </query>"
"</term>";

const string req_search_pax =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinLoadPax>"
"      <point_id>4405302</point_id>"
"      <reg_no>1</reg_no>"
"    </TCkinLoadPax>"
"  </query>"
"</term>";

const string req_delete_all_pax =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='sopp' ver='1' opr='DEN' screen='SOPP.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinDeleteAllPassangers>"
"      <point_id>4405302</point_id>"
"    </TCkinDeleteAllPassangers>"
"  </query>"
"</term>";


const string req_new_pax =
"<?xml version='1.0' encoding='cp866'?>"
"<term>"
"  <query handle='0' id='CheckIn' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='595428461'>"
"    <TCkinSavePax>"
"      <agent_stat_period>110</agent_stat_period>"
"      <transfer/>"
"      <segments>"
"        <segment>"
"          <point_dep>4405302</point_dep>"
"          <point_arv>4405303</point_arv>"
"          <airp_dep></airp_dep>"
"          <airp_arv></airp_arv>"
"          <class></class>"
"          <status>K</status>"
"          <wl_type/>"
"          <mark_flight>"
"            <airline></airline>"
"            <flt_no>1</flt_no>"
"            <suffix/>"
"            <scd>22.12.2016 11:00:00</scd>"
"            <airp_dep></airp_dep>"
"            <pr_mark_norms>0</pr_mark_norms>"
"          </mark_flight>"
"          <passengers>"
"            <pax>"
"              <pax_id/>"
"              <surname>DEN</surname>"
"              <name/>"
"              <pers_type></pers_type>"
"              <seat_no/>"
"              <preseat_no/>"
"              <seat_type/>"
"              <seats>1</seats>"
"              <ticket_no/>"
"              <coupon_no/>"
"              <ticket_rem/>"
"              <ticket_confirm>0</ticket_confirm>"
"              <document/>"
"              <doco/>"
"              <addresses/>"
"              <subclass></subclass>"
"              <bag_pool_num/>"
"              <transfer/>"
"              <rems/>"
"              <fqt_rems/>"
"              <norms/>"
"            </pax>"
"          </passengers>"
"          <paid_bag_emd/>"
"        </segment>"
"      </segments>"
"      <hall>1</hall>"
"      <paid_bags/>"
"    </TCkinSavePax>"
"  </query>"
"</term>";
