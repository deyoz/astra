var requests = [
    'LCI\nUT128/22FEB.SVO\nLR WM.S.P',
    'LCI\nUT128/22FEB.SVO\nCU\nWM.S.P.G.80/80/30/15.KG',
    'LCI\nUT128/22FEB.SVO\nLR WB.LANG.EN\nLR SP.WB\nLR BT',
    'LCI\nUT128/22FEB.SVO\nCU\nWM.WB.TT.4257/PT.2000/HT.150/BT.1000/CT.1100/MT.7.KG',
    'LCI\nUT128/22FEB.SVO\nLR WB.LANG.EN\nLR SR.WB.C.C6Y387',
    'LCI\nUT128/22FEB.SVO\nLR WB.LANG.EN\nLR SR.WB.S.1A/1B/1C/1D/1E/1F/1G/1H/1J/1K/2A/2B/2C/2D/2E/2F/2G/2H/2J/2K\nLR SR.WB.S.3A/3B/3C/3D/3E/3F/3G/3H/3J/3K/4A/4B/4C/4D/4E/4F/4G/4H/4J/4K\nLR SR.WB.S.5A/5B/5C/5D/5E/5F/5G/5H/5J/5K\nLR SR.WB.S.1B/1E/1G/1H/1J/1K/2B/2E/2G/2H/2J/2K/3A/3B/3C/3D/3E/3F/3G/3H\nLR SR.WB.S.3J/3K/4F/4G/4H/4J/4K/5G/5H/5J/5K/6A/6B/6C/6D/6E/6F/7A/7B/7C\nLR SR.WB.S.7D/7E/7F/8A/8B/8C/8D/8E/8F/9A/9B/9C/9D/9E/9F/10A/10B/10C\nLR SR.WB.S.10D/10E/10F/14A/14B/14C/14D/14E/14F/15A/15B/15C/15D/15E/15F\nLR SR.WB.S.16A/16B/16C/16D/16E/16F/17A/17B/17C/17D/17E/17F/18A/18B/18C\nLR SR.WB.S.18D/18E/18F/19A/19B/19C/19D/19E/19F/20A/20B/20C/20D/20E/20F\nLR SR.WB.S.21A/21B/21C/21D/21E/21F/22A/22B/22C/22D/22E/22F/23A/23B/23C\nLR SR.WB.S.23D/23E/23F/24A/24B/24C/24D/24E/24F/25A/25B/25C/25D/25E/25F\nLR SR.WB.S.26A/26B/26C/26D/26E/26F/27A/27B/27C/27D/27E/27F/28A/28B/28C\nLR SR.WB.S.28D/28E/28F/29A/29B/29C/29D/29E/29F/30B/30C/30D/30E/31A/31B\nLR SR.WB.S.31E/31F',
'ЮТ 001 23.03.17 ДМД 1\nACT:                \'[<ACT(,,dd mmm yy hh:nn:ss)>]\'\nACT RU:             \'[<ACT(,,dd mmm yy hh:nn:ss,R)>]\'\nACT EN:             \'[<ACT(,,dd mmm yy hh:nn:ss,E)>]\'\nAGENT:              \'[<AGENT>]\'\nAGENT RU:           \'[<AGENT(,,,R)>]\'\nAGENT EN:           \'[<AGENT(,,,E)>]\'\nAIRLINE:            \'[<AIRLINE>]\'\nAIRLINE RU:         \'[<AIRLINE(,,,R)>]\'\nAIRLINE EN:         \'[<AIRLINE(,,,E)>]\'\nAIRLINE_NAME:       \'[<AIRLINE_NAME>]\'\nAIRLINE_NAME RU:    \'[<AIRLINE_NAME(,,,R)>]\'\nAIRLINE_NAME EN:    \'[<AIRLINE_NAME(,,,E)>]\'\nAIRLINE_SHORT:      \'[<AIRLINE_SHORT>]\'\nAIRLINE_SHORT RU:   \'[<AIRLINE_SHORT(,,,R)>]\'\nAIRLINE_SHORT EN:   \'[<AIRLINE_SHORT(,,,E)>]\'\nAIRP_ARV:           \'[<AIRP_ARV>]\'\nAIRP_ARV RU:        \'[<AIRP_ARV(,,,R)>]\'\nAIRP_ARV EN:        \'[<AIRP_ARV(,,,E)>]\'\nAIRP_ARV_NAME:      \'[<AIRP_ARV_NAME>]\'\nAIRP_ARV_NAME RU:   \'[<AIRP_ARV_NAME(,,,R)>]\'\nAIRP_ARV_NAME EN:   \'[<AIRP_ARV_NAME(,,,E)>]\'\nAIRP_DEP:           \'[<AIRP_DEP>]\'\nAIRP_DEP RU:        \'[<AIRP_DEP(,,,R)>]\'\nAIRP_DEP EN:        \'[<AIRP_DEP(,,,E)>]\'\nAIRP_DEP_NAME:      \'[<AIRP_DEP_NAME>]\'\nAIRP_DEP_NAME RU:   \'[<AIRP_DEP_NAME(,,,R)>]\'\nAIRP_DEP_NAME EN:   \'[<AIRP_DEP_NAME(,,,E)>]\'\nBAGGAGE:            \'[<BAGGAGE>]\'\nBAGGAGE RU:         \'[<BAGGAGE(,,,R)>]\'\nBAGGAGE EN:         \'[<BAGGAGE(,,,E)>]\'\nBAG_AMOUNT:         \'[<BAG_AMOUNT>]\'\nBAG_AMOUNT RU:      \'[<BAG_AMOUNT(,,,R)>]\'\nBAG_AMOUNT EN:      \'[<BAG_AMOUNT(,,,E)>]\'\nBAG_WEIGHT:         \'[<BAG_WEIGHT>]\'\nBAG_WEIGHT RU:      \'[<BAG_WEIGHT(,,,R)>]\'\nBAG_WEIGHT EN:      \'[<BAG_WEIGHT(,,,E)>]\'\nBCBP_M_2:           \'[<BCBP_M_2>]\'\nBCBP_M_2 RU:        \'[<BCBP_M_2(,,,R)>]\'\nBCBP_M_2 EN:        \'[<BCBP_M_2(,,,E)>]\'\nBI_AIRP_TERMINAL:   \'[<BI_AIRP_TERMINAL>]\'\nBI_AIRP_TERMINAL RU:\'[<BI_AIRP_TERMINAL(,,,R)>]\'\nBI_AIRP_TERMINAL EN:\'[<BI_AIRP_TERMINAL(,,,E)>]\'\nBI_HALL:            \'[<BI_HALL>]\'\nBI_HALL RU:         \'[<BI_HALL(,,,R)>]\'\nBI_HALL EN:         \'[<BI_HALL(,,,E)>]\'\nBI_HALL_CAPTION:    \'[<BI_HALL_CAPTION>]\'\nBI_HALL_CAPTION RU: \'[<BI_HALL_CAPTION(,,,R)>]\'\nBI_HALL_CAPTION EN: \'[<BI_HALL_CAPTION(,,,E)>]\'\nBI_RULE:            \'[<BI_RULE>]\'\nBI_RULE RU:         \'[<BI_RULE(,,,R)>]\'\nBI_RULE EN:         \'[<BI_RULE(,,,E)>]\'\nBI_RULE_GUEST:      \'[<BI_RULE_GUEST>]\'\nBI_RULE_GUEST RU:   \'[<BI_RULE_GUEST(,,,R)>]\'\nBI_RULE_GUEST EN:   \'[<BI_RULE_GUEST(,,,E)>]\'\nBRAND:              \'[<BRAND>]\'\nBRAND RU:           \'[<BRAND(,,,R)>]\'\nBRAND EN:           \'[<BRAND(,,,E)>]\'\nBRD_FROM:           \'[<BRD_FROM>]\'\nBRD_FROM RU:        \'[<BRD_FROM(,,,R)>]\'\nBRD_FROM EN:        \'[<BRD_FROM(,,,E)>]\'\nBRD_TO:             \'[<BRD_TO>]\'\nBRD_TO RU:          \'[<BRD_TO(,,,R)>]\'\nBRD_TO EN:          \'[<BRD_TO(,,,E)>]\'\nCHD:                \'[<CHD>]\'\nCHD RU:             \'[<CHD(,,,R)>]\'\nCHD EN:             \'[<CHD(,,,E)>]\'\nCITY_ARV_NAME:      \'[<CITY_ARV_NAME>]\'\nCITY_ARV_NAME RU:   \'[<CITY_ARV_NAME(,,,R)>]\'\nCITY_ARV_NAME EN:   \'[<CITY_ARV_NAME(,,,E)>]\'\nCITY_DEP_NAME:      \'[<CITY_DEP_NAME>]\'\nCITY_DEP_NAME RU:   \'[<CITY_DEP_NAME(,,,R)>]\'\nCITY_DEP_NAME EN:   \'[<CITY_DEP_NAME(,,,E)>]\'\nCLASS:              \'[<CLASS>]\'\nCLASS RU:           \'[<CLASS(,,,R)>]\'\nCLASS EN:           \'[<CLASS(,,,E)>]\'\nCLASS_NAME:         \'[<CLASS_NAME>]\'\nCLASS_NAME RU:      \'[<CLASS_NAME(,,,R)>]\'\nCLASS_NAME EN:      \'[<CLASS_NAME(,,,E)>]\'\nDESK:               \'[<DESK>]\'\nDESK RU:            \'[<DESK(,,,R)>]\'\nDESK EN:            \'[<DESK(,,,E)>]\'\nDOCUMENT:           \'[<DOCUMENT>]\'\nDOCUMENT RU:        \'[<DOCUMENT(,,,R)>]\'\nDOCUMENT EN:        \'[<DOCUMENT(,,,E)>]\'\nDUPLICATE:          \'[<DUPLICATE>]\'\nDUPLICATE RU:       \'[<DUPLICATE(,,,R)>]\'\nDUPLICATE EN:       \'[<DUPLICATE(,,,E)>]\'\nEST:                \'[<EST(,,dd mmm yy hh:nn:ss)>]\'\nEST RU:             \'[<EST(,,dd mmm yy hh:nn:ss,R)>]\'\nEST EN:             \'[<EST(,,dd mmm yy hh:nn:ss,E)>]\'\nETICKET_NO:         \'[<ETICKET_NO>]\'\nETICKET_NO RU:      \'[<ETICKET_NO(,,,R)>]\'\nETICKET_NO EN:      \'[<ETICKET_NO(,,,E)>]\'\nETKT:               \'[<ETKT>]\'\nETKT RU:            \'[<ETKT(,,,R)>]\'\nETKT EN:            \'[<ETKT(,,,E)>]\'\nEXCESS:             \'[<EXCESS>]\'\nEXCESS RU:          \'[<EXCESS(,,,R)>]\'\nEXCESS EN:          \'[<EXCESS(,,,E)>]\'\nFLT_NO:             \'[<FLT_NO>]\'\nFLT_NO RU:          \'[<FLT_NO(,,,R)>]\'\nFLT_NO EN:          \'[<FLT_NO(,,,E)>]\'\nFQT:                \'[<FQT>]\'\nFQT RU:             \'[<FQT(,,,R)>]\'\nFQT EN:             \'[<FQT(,,,E)>]\'\nFQT_TIER_LEVEL:     \'[<FQT_TIER_LEVEL>]\'\nFQT_TIER_LEVEL RU:  \'[<FQT_TIER_LEVEL(,,,R)>]\'\nFQT_TIER_LEVEL EN:  \'[<FQT_TIER_LEVEL(,,,E)>]\'\nFULLNAME:           \'[<FULLNAME>]\'\nFULLNAME RU:        \'[<FULLNAME(,,,R)>]\'\nFULLNAME EN:        \'[<FULLNAME(,,,E)>]\'\nFULL_PLACE_ARV:     \'[<FULL_PLACE_ARV>]\'\nFULL_PLACE_ARV RU:  \'[<FULL_PLACE_ARV(,,,R)>]\'\nFULL_PLACE_ARV EN:  \'[<FULL_PLACE_ARV(,,,E)>]\'\nFULL_PLACE_DEP:     \'[<FULL_PLACE_DEP>]\'\nFULL_PLACE_DEP RU:  \'[<FULL_PLACE_DEP(,,,R)>]\'\nFULL_PLACE_DEP EN:  \'[<FULL_PLACE_DEP(,,,E)>]\'\nGATE:               \'[<GATE>]\'\nGATE RU:            \'[<GATE(,,,R)>]\'\nGATE EN:            \'[<GATE(,,,E)>]\'\nGATES:              \'[<GATES>]\'\nGATES RU:           \'[<GATES(,,,R)>]\'\nGATES EN:           \'[<GATES(,,,E)>]\'\nHALL:               \'[<HALL>]\'\nHALL RU:            \'[<HALL(,,,R)>]\'\nHALL EN:            \'[<HALL(,,,E)>]\'\nINF:                \'[<INF>]\'\nINF RU:             \'[<INF(,,,R)>]\'\nINF EN:             \'[<INF(,,,E)>]\'\nLIST_SEAT_NO:       \'[<LIST_SEAT_NO>]\'\nLIST_SEAT_NO RU:    \'[<LIST_SEAT_NO(,,,R)>]\'\nLIST_SEAT_NO EN:    \'[<LIST_SEAT_NO(,,,E)>]\'\nLONG_ARV:           \'[<LONG_ARV>]\'\nLONG_ARV RU:        \'[<LONG_ARV(,,,R)>]\'\nLONG_ARV EN:        \'[<LONG_ARV(,,,E)>]\'\nLONG_DEP:           \'[<LONG_DEP>]\'\nLONG_DEP RU:        \'[<LONG_DEP(,,,R)>]\'\nLONG_DEP EN:        \'[<LONG_DEP(,,,E)>]\'\nNAME:               \'[<NAME>]\'\nNAME RU:            \'[<NAME(,,,R)>]\'\nNAME EN:            \'[<NAME(,,,E)>]\'\nNO_SMOKE:           \'[<NO_SMOKE>]\'\nNO_SMOKE RU:        \'[<NO_SMOKE(,,,R)>]\'\nNO_SMOKE EN:        \'[<NO_SMOKE(,,,E)>]\'\nONE_SEAT_NO:        \'[<ONE_SEAT_NO>]\'\nONE_SEAT_NO RU:     \'[<ONE_SEAT_NO(,,,R)>]\'\nONE_SEAT_NO EN:     \'[<ONE_SEAT_NO(,,,E)>]\'\nPAX_ID:             \'[<PAX_ID>]\'\nPAX_ID RU:          \'[<PAX_ID(,,,R)>]\'\nPAX_ID EN:          \'[<PAX_ID(,,,E)>]\'\nPAX_TITLE:          \'[<PAX_TITLE>]\'\nPAX_TITLE RU:       \'[<PAX_TITLE(,,,R)>]\'\nPAX_TITLE EN:       \'[<PAX_TITLE(,,,E)>]\'\nPLACE_ARV:          \'[<PLACE_ARV>]\'\nPLACE_ARV RU:       \'[<PLACE_ARV(,,,R)>]\'\nPLACE_ARV EN:       \'[<PLACE_ARV(,,,E)>]\'\nPLACE_DEP:          \'[<PLACE_DEP>]\'\nPLACE_DEP RU:       \'[<PLACE_DEP(,,,R)>]\'\nPLACE_DEP EN:       \'[<PLACE_DEP(,,,E)>]\'\nPNR:                \'[<PNR>]\'\nPNR RU:             \'[<PNR(,,,R)>]\'\nPNR EN:             \'[<PNR(,,,E)>]\'\nREG_NO:             \'[<REG_NO>]\'\nREG_NO RU:          \'[<REG_NO(,,,R)>]\'\nREG_NO EN:          \'[<REG_NO(,,,E)>]\'\nREM:                \'[<REM>]\'\nREM RU:             \'[<REM(,,,R)>]\'\nREM EN:             \'[<REM(,,,E)>]\'\nREM_TXT0:           \'[<REM_TXT0>]\'\nREM_TXT0 RU:        \'[<REM_TXT0(,,,R)>]\'\nREM_TXT0 EN:        \'[<REM_TXT0(,,,E)>]\'\nREM_TXT1:           \'[<REM_TXT1>]\'\nREM_TXT1 RU:        \'[<REM_TXT1(,,,R)>]\'\nREM_TXT1 EN:        \'[<REM_TXT1(,,,E)>]\'\nREM_TXT2:           \'[<REM_TXT2>]\'\nREM_TXT2 RU:        \'[<REM_TXT2(,,,R)>]\'\nREM_TXT2 EN:        \'[<REM_TXT2(,,,E)>]\'\nREM_TXT3:           \'[<REM_TXT3>]\'\nREM_TXT3 RU:        \'[<REM_TXT3(,,,R)>]\'\nREM_TXT3 EN:        \'[<REM_TXT3(,,,E)>]\'\nREM_TXT4:           \'[<REM_TXT4>]\'\nREM_TXT4 RU:        \'[<REM_TXT4(,,,R)>]\'\nREM_TXT4 EN:        \'[<REM_TXT4(,,,E)>]\'\nREM_TXT5:           \'[<REM_TXT5>]\'\nREM_TXT5 RU:        \'[<REM_TXT5(,,,R)>]\'\nREM_TXT5 EN:        \'[<REM_TXT5(,,,E)>]\'\nREM_TXT6:           \'[<REM_TXT6>]\'\nREM_TXT6 RU:        \'[<REM_TXT6(,,,R)>]\'\nREM_TXT6 EN:        \'[<REM_TXT6(,,,E)>]\'\nREM_TXT7:           \'[<REM_TXT7>]\'\nREM_TXT7 RU:        \'[<REM_TXT7(,,,R)>]\'\nREM_TXT7 EN:        \'[<REM_TXT7(,,,E)>]\'\nREM_TXT8:           \'[<REM_TXT8>]\'\nREM_TXT8 RU:        \'[<REM_TXT8(,,,R)>]\'\nREM_TXT8 EN:        \'[<REM_TXT8(,,,E)>]\'\nREM_TXT9:           \'[<REM_TXT9>]\'\nREM_TXT9 RU:        \'[<REM_TXT9(,,,R)>]\'\nREM_TXT9 EN:        \'[<REM_TXT9(,,,E)>]\'\nRFISC_BSN_LONGUE:   \'[<RFISC_BSN_LONGUE>]\'\nRFISC_BSN_LONGUE RU:\'[<RFISC_BSN_LONGUE(,,,R)>]\'\nRFISC_BSN_LONGUE EN:\'[<RFISC_BSN_LONGUE(,,,E)>]\'\nRFISC_FAST_TRACK:   \'[<RFISC_FAST_TRACK>]\'\nRFISC_FAST_TRACK RU:\'[<RFISC_FAST_TRACK(,,,R)>]\'\nRFISC_FAST_TRACK EN:\'[<RFISC_FAST_TRACK(,,,E)>]\'\nRFISC_UPGRADE:      \'[<RFISC_UPGRADE>]\'\nRFISC_UPGRADE RU:   \'[<RFISC_UPGRADE(,,,R)>]\'\nRFISC_UPGRADE EN:   \'[<RFISC_UPGRADE(,,,E)>]\'\nRK_AMOUNT:          \'[<RK_AMOUNT>]\'\nRK_AMOUNT RU:       \'[<RK_AMOUNT(,,,R)>]\'\nRK_AMOUNT EN:       \'[<RK_AMOUNT(,,,E)>]\'\nRK_WEIGHT:          \'[<RK_WEIGHT>]\'\nRK_WEIGHT RU:       \'[<RK_WEIGHT(,,,R)>]\'\nRK_WEIGHT EN:       \'[<RK_WEIGHT(,,,E)>]\'\nRSTATION:           \'[<RSTATION>]\'\nRSTATION RU:        \'[<RSTATION(,,,R)>]\'\nRSTATION EN:        \'[<RSTATION(,,,E)>]\'\nSCD:                \'[<SCD(,,dd mmm yy hh:nn:ss)>]\'\nSCD RU:             \'[<SCD(,,dd mmm yy hh:nn:ss,R)>]\'\nSCD EN:             \'[<SCD(,,dd mmm yy hh:nn:ss,E)>]\'\nSEAT_NO:            \'[<SEAT_NO>]\'\nSEAT_NO RU:         \'[<SEAT_NO(,,,R)>]\'\nSEAT_NO EN:         \'[<SEAT_NO(,,,E)>]\'\nSTR_SEAT_NO:        \'[<STR_SEAT_NO>]\'\nSTR_SEAT_NO RU:     \'[<STR_SEAT_NO(,,,R)>]\'\nSTR_SEAT_NO EN:     \'[<STR_SEAT_NO(,,,E)>]\'\nSUBCLS:             \'[<SUBCLS>]\'\nSUBCLS RU:          \'[<SUBCLS(,,,R)>]\'\nSUBCLS EN:          \'[<SUBCLS(,,,E)>]\'\nSURNAME:            \'[<SURNAME>]\'\nSURNAME RU:         \'[<SURNAME(,,,R)>]\'\nSURNAME EN:         \'[<SURNAME(,,,E)>]\'\nTAGS:               \'[<TAGS>]\'\nTAGS RU:            \'[<TAGS(,,,R)>]\'\nTAGS EN:            \'[<TAGS(,,,E)>]\'\nTEST_SERVER:        \'[<TEST_SERVER>]\'\nTEST_SERVER RU:     \'[<TEST_SERVER(,,,R)>]\'\nTEST_SERVER EN:     \'[<TEST_SERVER(,,,E)>]\'\nTICKET_NO:          \'[<TICKET_NO>]\'\nTICKET_NO RU:       \'[<TICKET_NO(,,,R)>]\'\nTICKET_NO EN:       \'[<TICKET_NO(,,,E)>]\'\nTIME_PRINT:         \'[<TIME_PRINT(,,dd mmm yy hh:nn:ss)>]\'\nTIME_PRINT RU:      \'[<TIME_PRINT(,,dd mmm yy hh:nn:ss,R)>]\'\nTIME_PRINT EN:      \'[<TIME_PRINT(,,dd mmm yy hh:nn:ss,E)>]\'\nVOUCHER_CODE:       \'[<VOUCHER_CODE>]\'\nVOUCHER_CODE RU:    \'[<VOUCHER_CODE(,,,R)>]\'\nVOUCHER_CODE EN:    \'[<VOUCHER_CODE(,,,E)>]\'\nVOUCHER_TEXT:       \'[<VOUCHER_TEXT>]\'\nVOUCHER_TEXT RU:    \'[<VOUCHER_TEXT(,,,R)>]\'\nVOUCHER_TEXT EN:    \'[<VOUCHER_TEXT(,,,E)>]\'\nVOUCHER_TEXT1:      \'[<VOUCHER_TEXT1>]\'\nVOUCHER_TEXT1 RU:   \'[<VOUCHER_TEXT1(,,,R)>]\'\nVOUCHER_TEXT1 EN:   \'[<VOUCHER_TEXT1(,,,E)>]\'\nVOUCHER_TEXT10:     \'[<VOUCHER_TEXT10>]\'\nVOUCHER_TEXT10 RU:  \'[<VOUCHER_TEXT10(,,,R)>]\'\nVOUCHER_TEXT10 EN:  \'[<VOUCHER_TEXT10(,,,E)>]\'\nVOUCHER_TEXT2:      \'[<VOUCHER_TEXT2>]\'\nVOUCHER_TEXT2 RU:   \'[<VOUCHER_TEXT2(,,,R)>]\'\nVOUCHER_TEXT2 EN:   \'[<VOUCHER_TEXT2(,,,E)>]\'\nVOUCHER_TEXT3:      \'[<VOUCHER_TEXT3>]\'\nVOUCHER_TEXT3 RU:   \'[<VOUCHER_TEXT3(,,,R)>]\'\nVOUCHER_TEXT3 EN:   \'[<VOUCHER_TEXT3(,,,E)>]\'\nVOUCHER_TEXT4:      \'[<VOUCHER_TEXT4>]\'\nVOUCHER_TEXT4 RU:   \'[<VOUCHER_TEXT4(,,,R)>]\'\nVOUCHER_TEXT4 EN:   \'[<VOUCHER_TEXT4(,,,E)>]\'\nVOUCHER_TEXT5:      \'[<VOUCHER_TEXT5>]\'\nVOUCHER_TEXT5 RU:   \'[<VOUCHER_TEXT5(,,,R)>]\'\nVOUCHER_TEXT5 EN:   \'[<VOUCHER_TEXT5(,,,E)>]\'\nVOUCHER_TEXT6:      \'[<VOUCHER_TEXT6>]\'\nVOUCHER_TEXT6 RU:   \'[<VOUCHER_TEXT6(,,,R)>]\'\nVOUCHER_TEXT6 EN:   \'[<VOUCHER_TEXT6(,,,E)>]\'\nVOUCHER_TEXT7:      \'[<VOUCHER_TEXT7>]\'\nVOUCHER_TEXT7 RU:   \'[<VOUCHER_TEXT7(,,,R)>]\'\nVOUCHER_TEXT7 EN:   \'[<VOUCHER_TEXT7(,,,E)>]\'\nVOUCHER_TEXT8:      \'[<VOUCHER_TEXT8>]\'\nVOUCHER_TEXT8 RU:   \'[<VOUCHER_TEXT8(,,,R)>]\'\nVOUCHER_TEXT8 EN:   \'[<VOUCHER_TEXT8(,,,E)>]\'\nVOUCHER_TEXT9:      \'[<VOUCHER_TEXT9>]\'\nVOUCHER_TEXT9 RU:   \'[<VOUCHER_TEXT9(,,,R)>]\'\nVOUCHER_TEXT9 EN:   \'[<VOUCHER_TEXT9(,,,E)>]\'\n',
'PRINT LOADSHEET\nUT001/30',
'<run_stat>\n    <stat_type>Short</stat_type>\n    <stat_mode>Transfer</stat_mode>\n    <FirstDate>01.05.2018 00:00:00</FirstDate>\n    <LastDate>30.06.2018 23:59:59</LastDate>\n    <ak>КЮ</ak>\n    <ap/>\n    <flt_no/>\n    <seance/>\n    <access>\n        <airlines/>\n        <airlines_permit>0</airlines_permit>\n        <airps/>\n        <airps_permit>0</airps_permit>\n    </access>\n</run_stat>',
'<term>\n  <query lang=\"RU\">\n    <CREWCHECKIN>\n      <FLIGHT>                \n        <AIRLINE>6W</AIRLINE>  \n        <FLT_NO>776</FLT_NO>              \n        <SUFFIX/>\n        <SCD>07.09.2015</SCD> \n        <AIRP_DEP>RTW</AIRP_DEP>          \n      </FLIGHT>\n      <CREW_GROUPS>\n        <CREW_GROUP>           \n          <AIRP_ARV>DME</AIRP_ARV>            \n          <CREW_MEMBERS>\n            <CREW_MEMBER>       \n                <CREW_TYPE>CR1</CREW_TYPE>\n                <LOCATION>CABIN</LOCATION>\n                <PERSONAL_DATA>\n                    <DOCS>\n                        <NO>1234567891</NO>\n                        <TYPE>P</TYPE>\n                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>\n                        <NO>1234567891</NO>\n                        <NATIONALITY>RUS</NATIONALITY>\n                        <BIRTH_DATE>01.05.1976</BIRTH_DATE>\n                        <GENDER>M</GENDER>\n                        <EXPIRY_DATE>12.05.2020</EXPIRY_DATE>\n                        <SURNAME>IVANOV</SURNAME>          \n                        <FIRST_NAME>IVAN</FIRST_NAME>\n                        <SECOND_NAME>ROMANOVIC</SECOND_NAME>\n                    </DOCS>\n                    <DOCO>\n                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>\n                        <TYPE>V</TYPE>\n                        <NO>VI78787787878</NO>\n                        <ISSUE_PLACE>MOSKVA TURISTKAY STRIT 25</ISSUE_PLACE>\n                        <ISSUE_DATE>12.02.2014</ISSUE_DATE>\n                        <EXPIRY_DATE>15.12.2025</EXPIRY_DATE>\n                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>\n                    </DOCO>\n                    <DOCA>\n                        <TYPE>B</TYPE>\n                        <COUNTRY>RUS</COUNTRY>\n                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>\n                        <CITY>MOSKVA</CITY>\n                        <REGION>MOSKVA</REGION>\n                        <POSTAL_CODE>125373</POSTAL_CODE>\n                    </DOCA>\n                    <DOCA>\n                        <TYPE>R</TYPE>\n                        <COUNTRY>RUS</COUNTRY>\n                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>\n                        <CITY>MOSKVA</CITY>\n                        <REGION>MOSKVA</REGION>\n                        <POSTAL_CODE>266373</POSTAL_CODE>\n                    </DOCA>\n                    <DOCA>\n                        <TYPE>D</TYPE>\n                        <COUNTRY>USA</COUNTRY>\n                        <ADDRESS>FIFTH AVENUE</ADDRESS>\n                        <CITY>NY</CITY>\n                        <REGION>NY</REGION>\n                        <POSTAL_CODE>9999</POSTAL_CODE>\n                    </DOCA>\n                </PERSONAL_DATA>\n            </CREW_MEMBER>\n            <CREW_MEMBER>       \n                <CREW_TYPE>CR1</CREW_TYPE>\n                <LOCATION>CABIN</LOCATION>\n                <PERSONAL_DATA>\n                    <DOCS>\n                        <NO>7778889991</NO>\n                        <TYPE>P</TYPE>\n                        <ISSUE_COUNTRY>RUS</ISSUE_COUNTRY>\n                        <NO>99988887774</NO>\n                        <NATIONALITY>RUS</NATIONALITY>\n                        <BIRTH_DATE>02.05.1976</BIRTH_DATE>\n                        <GENDER>M</GENDER>\n                        <EXPIRY_DATE>22.05.2020</EXPIRY_DATE>\n                        <SURNAME>REPIN</SURNAME>          \n                        <FIRST_NAME>DIMA</FIRST_NAME>\n                        <SECOND_NAME></SECOND_NAME>\n                    </DOCS>\n                    <DOCO>\n                        <BIRTH_PLACE>MOSKVA RUSSIY</BIRTH_PLACE>\n                        <TYPE>V</TYPE>\n                        <NO>VI78787787878</NO>\n                        <ISSUE_PLACE>MOSKVA POLKA STRIT 25</ISSUE_PLACE>\n                        <ISSUE_DATE>15.02.2014</ISSUE_DATE>\n                        <EXPIRY_DATE>19.12.2025</EXPIRY_DATE>\n                        <APPLIC_COUNTRY>USA</APPLIC_COUNTRY>\n                    </DOCO>\n                    <DOCA>\n                        <TYPE>B</TYPE>\n                        <COUNTRY>RUS</COUNTRY>\n                        <ADDRESS>DUBROVCA CTRIT 25 256</ADDRESS>\n                        <CITY>MOSKVA</CITY>\n                        <REGION>MOSKVA</REGION>\n                        <POSTAL_CODE>125373</POSTAL_CODE>\n                    </DOCA>\n                    <DOCA>\n                        <TYPE>R</TYPE>\n                        <COUNTRY>RUS</COUNTRY>\n                        <ADDRESS>POPOVCA CTRIT 48</ADDRESS>\n                        <CITY>MOSKVA</CITY>\n                        <REGION>MOSKVA</REGION>\n                        <POSTAL_CODE>266373</POSTAL_CODE>\n                    </DOCA>\n                    <DOCA>\n                        <TYPE>D</TYPE>\n                        <COUNTRY>USA</COUNTRY>\n                        <ADDRESS>FIFTH AVENUE</ADDRESS>\n                        <CITY>NY</CITY>\n                        <REGION>NY</REGION>\n                        <POSTAL_CODE>9999</POSTAL_CODE>\n                    </DOCA>\n                </PERSONAL_DATA>\n            </CREW_MEMBER>\n          </CREW_MEMBERS>\n        </CREW_GROUP>\n      </CREW_GROUPS>\n    </CREWCHECKIN>\n  </query>\n</term>',
'<print_bp2>\n  <airline>ЮТ</airline>\n  <flt_no>1</flt_no>\n  <scd_out>22.08.18</scd_out>\n  <airp_dep>ДМД</airp_dep>\n  <reg_no>1</reg_no>\n  <dev_model>TPM200</dev_model>\n  <fmt_type>Graphics2D</fmt_type>\n</print_bp2>',
'NTM\nЮТ002/31AUG SVO PART2/19951011002103\n-СОЧ.0/0/0.T0.PAX/0/0.PAD/0/0\nSI СОЧ B/0.C/0.M/0\nPART 1 END'
    ];