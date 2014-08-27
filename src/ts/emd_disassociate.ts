include(ts/macro.ts)

$(init_jxt_pult åéÇêéå)
$(login)
$(init_eds ûí UTET UTDC)


{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDDisassociateForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>2276899</point_id>
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
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
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
    <kick req_ctxt_id...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Success'...


%%
###################################################################################################

$(init_jxt_pult åéÇêéå)
$(login)
$(init_eds ûí UTET UTDC)

{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDDisassociateForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>2276899</point_id>
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
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
EQN+1:TD+1:TF"
TKT+2981212121212:J::4::2982348111616"
CPN+1:::::::1::703"
UNT+7+1"
UNZ+1+$(last_edifact_ref)0001"

$(sql {update EDISESSION_TIMEOUTS set time_out = sysdate - 1})
$(run_daemon edi_timeout)

>> lines=auto
    <kick req_ctxt_id...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Time out' edi_error_code='' remark=''...


%%
###################################################################################################

$(init_jxt_pult åéÇêéå)
$(login)
$(init_eds ûí UTET UTDC)

{<?xml version='1.0' encoding='UTF-8'?>
 <term>
   <query handle='0' id='EMDDisassociateForm' ver='1' opr='PIKE' screen='AIR.EXE' mode='STAND' lang='RU' term_id='2479792165'>
     <DisassociateEMD>
       <point_id>2276899</point_id>
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
ORG+ûí:åéÇ++++Y+::RU+åéÇêéå"
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
    <kick req_ctxt_id...

!! capture=on err=ignore
$(lastRedisplay)

>> lines=auto
    <result status='Error in remote host' edi_error_code='118' remark='COUPON SUSPENDED OR REACHED FINAL STATUS'...
