$(init)
$(init_jxt_pult ������)

$(sql "insert into APIS_SETS(AIRLINE, COUNTRY_DEP, COUNTRY_ARV, COUNTRY_CONTROL, FORMAT, TRANSPORT_TYPE, TRANSPORT_PARAMS, EDI_ADDR, EDI_OWN_ADDR, ID, PR_DENIAL) values('??', '��', NULL, '��', 'IAPI_CN', '?', '?', 'NIAC', 'NORDWIND', id__seq.nextval, 0)")
$(sql "insert into EDI_ADDRS(ADDR, CFG_ID, CANON_NAME) values ('NIAC', 0, 'MOWET')")
$(sql "insert into EDIFACT_PROFILES (NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER) values ('IAPI', 'D', '05B', 'UN', 'UNOA', 4)")
$(sql "update ROT set H2H=1, H2H_REM_ADDR_NUM=1, H2H_ADDR='1HCNIAPIR', OUR_H2H_ADDR='1HCNIAPIQ'")


<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P0IPR\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC:ZZ+NORDWIND:ZZ+190819:0930+00000000001++IAPI"
UNG+CUSRES+NIAC+NEW AIRLINES+190819:0930+1023+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:2516"
RFF+AF:FV256"
DTM+189:1908011800:201"
DTM+232:1908012300:201"
LOC+125+DME"
LOC+87+PEK"
RFF+AF:FV257"
DTM+189:1908021800:201"
DTM+232:1908022300:201"
LOC+125+VKO"
LOC+87+AER"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:MU010174741777004"
ERC+0Z"
ERP+2"
RFF+AVF:NY7HZ0"
RFF+ABO:MU010174741777000"
ERC+1Z"
FTX+AAP+++AIRLINE NOT OPEN IAPI RULE, please contact with NIA of China, tel861056095288"
UNT+12+11085B94E1F8FA"
UNE+1+1023"
UNZ+1+1"

>>
UNB+SIRE:4+NORDWIND:ZZ+NIAC:ZZ+xxxxxx:xxxx+00000000001++IAPI"
UNG+CUSRES+NEW AIRLINES+NIAC+xxxxxx:xxxx+1023+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+312"
RFF+TN:2516"
RFF+AF:FV256"
DTM+189:1908011800:201"
DTM+232:1908012300:201"
LOC+125+DME"
LOC+87+PEK"
RFF+AF:FV257"
DTM+189:1908021800:201"
DTM+232:1908022300:201"
LOC+125+VKO"
LOC+87+AER"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:MU010174741777004"
ERC+0Z"
ERP+2"
RFF+AVF:NY7HZ0"
RFF+ABO:MU010174741777000"
ERC+1Z"
UNT+22+11085B94E1F8FA"
UNE+1+1023"
UNZ+1+00000000001"


<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P0IPR\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC+MU+190923:1659+1569229144986++IAPI"
UNG+CUSRES+NIAC+NORDWIND+190923:1659+15692291449862+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:1909230912028139441"
RFF+AF:MU589"
DTM+189:1911180320:201"
DTM+232:1911181430:201"
LOC+125+SFO"
LOC+87+PVG"
ERP+2"
RFF+AVF:NY7HZZ"
RFF+ABO:33280728"
ERC+1Z"
UNT+13+11085B94E1F8FA"
UNE+1+15692291449862"
UNZ+1+1569229144986"

>>
UNB+SIRE:4+MU+NIAC+xxxxxx:xxxx+1569229144986++IAPI"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+312"
ERP+1"
ERC+118"
UNT+5+11085B94E1F8FA"
UNZ+1+1569229144986"
