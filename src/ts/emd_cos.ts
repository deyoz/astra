include(ts/macro.ts)

#meta: suite emd


$(init_jxt_pult Œ‚Œ)
$(login)
$(init_eds ’ UTET UTDC)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDStatus' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <ChangeStatus>
       <point_id>2276899</point_id>
       <EmdNoEdit>2982348111616</EmdNoEdit>
       <CpnNoEdit>1</CpnNoEdit>
       <CpnStatusEdit>C</CpnStatusEdit>
     </ChangeStatus>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:793"
ORG+’:Œ‚++++Y+::RU+Œ‚Œ"
EQN+1:TD"
TKT+2982348111616:J::3"
CPN+1:CK"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+140820:1049+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:793+3"
EQN+1:TD"
TKT+2982348111616:J::3"
CPN+1:CK::E"
UNT+6+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...
  
# TODO  
!!
$(lastRedisplay)
