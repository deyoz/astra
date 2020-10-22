include(ts/macro.ts)

#meta: suite emd

$(init_term)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� REPIN IVAN)


{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSystemUpdate' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
       <TickCpnNo>1</TickCpnNo>
       <EmdNoEdit>2981212121212</EmdNoEdit>
       <EmdCpnNo>1</EmdCpnNo>
     </DisassociateEMD>
   </query>
 </term>}

>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:794"
ORG+1H:���+++��+Y+::RU+������"
EQN+1:TD+1:TF"
TKT+2981212121212:J::4::2982348111616"
CPN+1:::::::1::703"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+140820:1049+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:794+3"
UNT+3+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Success'...


%%
###################################################################################################

$(init_term)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� REPIN IVAN)

{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSystemUpdate' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
       <TickCpnNo>1</TickCpnNo>
       <EmdNoEdit>2981212121212</EmdNoEdit>
       <EmdCpnNo>1</EmdCpnNo>
     </DisassociateEMD>
   </query>
 </term>}


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:794"
ORG+1H:���+++��+Y+::RU+������"
EQN+1:TD+1:TF"
TKT+2981212121212:J::4::2982348111616"
CPN+1:::::::1::703"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(pg_sql {update EDISESSION_TIMEOUTS set time_out = current_timestamp - interval '1 hour'})
$(run_daemon edi_timeout)

>> lines=auto
    <kick...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Time out' edi_error_code='' remark=''...


%%
###################################################################################################

$(init_term)

$(init_eds �� UTET UTDC)

$(PREPARE_FLIGHT_1PAX_1SEG �� 103 ��� ��� REPIN IVAN)

{<?xml version='1.0' encoding='CP866'?>
 <term>
   <query handle='0' id='EMDSystemUpdate' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>$(last_point_id_spp)</point_id>
       <TickNoEdit>2982348111616</TickNoEdit>
       <TickCpnNo>1</TickCpnNo>
       <EmdNoEdit>2981212121212</EmdNoEdit>
       <EmdCpnNo>1</EmdCpnNo>
     </DisassociateEMD>
   </query>
 </term>}


>>
UNB+SIRE:1+UTDC+UTET+xxxxxx:xxxx+$(last_edifact_ref)0001+++O"
UNH+1+TKCREQ:96:2:IA+$(last_edifact_ref)"
MSG+:794"
ORG+1H:���+++��+Y+::RU+������"
EQN+1:TD+1:TF"
TKT+2981212121212:J::4::2982348111616"
CPN+1:::::::1::703"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

<<
UNB+SIRE:1+UTET+UTDC+140820:1049+$(last_edifact_ref)0001+++T"
UNH+1+TKCRES:06:1:IA+$(last_edifact_ref)"
MSG+:794+7"
ERC+118"
IFT+3+COUPON SUSPENDED OR REACHED FINAL STATUS"
UNT+5+1"
UNZ+1+$(last_edifact_ref)0001"

>> lines=auto
    <kick...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Error in remote host' edi_error_code='118' remark='COUPON SUSPENDED OR REACHED FINAL STATUS'...
