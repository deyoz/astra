$(init)
$(init_jxt_pult ������)

$(sql "insert into APPS_SETS(AIRLINE, APPS_COUNTRY, FLT_CLOSEOUT, FORMAT, ID, INBOUND, OUTBOUND, PR_DENIAL) values ('??', '??', 1, 'IAPI_CN', id__seq.nextval, 1, 1, 0)")
$(sql "insert into EDI_ADDRS(ADDR, CFG_ID, CANON_NAME) values ('NIAC', 0, 'MOWET')")
$(sql "insert into EDIFACT_PROFILES (NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER) values ('IAPI', 'D', '05B', 'UN', 'UNOA', 4)")
$(sql "update ROT set H2H=1, H2H_REM_ADDR_NUM=1, H2H_ADDR='1HCNIAPIR', OUR_H2H_ADDR='1HCNIAPIQ'")


<< h2h=V.\VDLG.WA/E11HCNIAPIR/I11HCNIAPIQ/P0IPR\VGYA\$() charset=UNOA
UNB+SIRE:4+NIAC:ZZ+NORDWIND:ZZ+190819:0930+00000000001++IAPI"
UNG+CUSRES+NIAC:ZZ+NORDWIND:ZZ+190819:0930+1+UN+D:05B"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN:IATA"
BGM+132"
RFF+TN:2516"
RFF+AF:FV256"
DTM+189:1908011800:201"
DTM+232:1908012300:201"
LOC+125+DME"
LOC+87+PEK"
ERP+1"
ERC+1"
FTX+AAP+++AIRLINE NOT OPEN IAPI RULE"
UNT+12+11085B94E1F8FA"
UNE+1+1"
UNZ+1+1"

>>
UNB+SIRE:4+NORDWIND:ZZ+NIAC:ZZ+xxxxxx:xxxx+00000000001++IAPI"
UNH+11085B94E1F8FA+CUSRES:D:05B:UN"
BGM+132"
ERP+1"
ERC+1"
UNT+5+11085B94E1F8FA"
UNZ+1+00000000001"
