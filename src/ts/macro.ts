$(defmacro login
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <UserLogon>
       <term_version>201311-0154253</term_version>
       <userr>PIKE</userr>
       <passwd>PIKE</passwd>
       <airlines/>
       <devices/>
       <command_line_params>
         <param>RESTART</param>
         <param>NOCUTE</param>
         <param>LANGRU</param>
       </command_line_params>
     </UserLogon>
   </query>
</term>}
}
) # end-of-defmacro

$(defmacro login2
  user=PIKE
  passwd=PIKE
{
{<?xml version='1.0' encoding='CP866'?>
<term>
  <query handle='0' id='MainDCS' ver='1' opr='' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <UserLogon>
      <term_version>201311-0154253</term_version>
      <lang dictionary_lang='RU' dictionary_checksum='622046546'>RU</lang>
      <userr>PIKE</userr>
      <passwd>PIKE</passwd>
      <airlines/>
      <devices/>
      <command_line_params>
        <param>RESTART</param>
        <param>NOCUTE</param>
        <param>LANGRU</param>
      </command_line_params>
    </UserLogon>
  </query>
</term>}
}
) # end-of-defmacro



$(defmacro PREPARE_SEASON_SCD
  airl=UT
  depp=DME
  arrp=AER
  fltno=747
  craft=TU5
{
{<?xml version='1.0' encoding='CP866'?>
 <term>
  <query handle='0' id='season' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <write>
      <filter>
        <season>2</season>
      </filter>
      <SubrangeList>
        <subrange>
          <modify>insert</modify>
          <move_id>-1</move_id>
          <first>$(date_format %d.%m.%Y -1mon) 12:00:00</first>
          <last>$(date_format %d.%m.%Y +1mon) 12:00:00</last>
          <days>1234567</days>
          <dests  >
            <dest>
              <cod>$(depp)</cod>
              <company>$(airl)</company>
              <trip>$(fltno)</trip>
              <bc>$(craft)</bc>
              <takeoff>30.12.1899 10:00:00</takeoff>
              <y>-1</y>
            </dest>
            <dest>
              <cod>$(arrp)</cod>
              <land>30.12.1899 12:00:00</land>
            </dest>
          </dests  >
        </subrange>
      </SubrangeList>
    </write>
  </query>
</term>}
}
) # end-of-macro


$(defmacro INBOUND_PNL
    airl
    depp
    arrp
    flt
{MOWKB1H
.MOWRMUT 020815
PNL
$(airl)$(flt)/$(ddmon +0 en) $(depp) PART1
CFG/060F060C060Y
RBD F/F C/C Y/YKMU
AVAIL
 $(depp)  $(arrp)
F060
C060
Y059
-LED000F
-LED000C
-LED001Y
1REPIN/IVAN
.L/0840Z6/$(airl)
.L/09T1B3/1H
-LED000K
-LED000M
-LED000U
ENDPNL}) #end-of-macro


$(defmacro PREPARE_ONE_FLIGHT
    airl=UT
    depp=DME
    arrp=LED
    flt=103
{
$(PREPARE_SEASON_SCD $(airl) $(depp) $(arrp) $(flt))
$(create_spp $(ddmmyyyy +0))

<<
$(INBOUND_PNL $(airl) $(depp) $(arrp) $(flt))

}) #end-of-macro
