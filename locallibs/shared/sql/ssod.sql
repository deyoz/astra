DROP TABLE ssod_fields;
CREATE TABLE ssod_fields (
       version              VARCHAR2(4) NOT NULL,
       rec_id               VARCHAR2(1) NOT NULL,
       name                 VARCHAR2(6) NOT NULL,
       type                 VARCHAR2(1) NOT NULL,
       length               NUMBER(3) NOT NULL,
       position             NUMBER(3) NOT NULL,
       CONSTRAINT ssod_fields__PK 
              PRIMARY KEY (version, rec_id, name)
);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','SPED', 'D',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','RPSI', 'S',  4,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','REVN', 'N',  3, 12);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','TPST', 'S',  4, 15);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','PRDA', 'D',  6, 19);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','TIME', 'T',  4, 25);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','SREL', 'S',  8, 29);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','FTYP', 'S',  1, 37);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','FTSN', 'S',  2, 38);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','1','RESD', 'S',216, 40);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','AGTN', 'S',  8,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','CJCP', 'S',  3, 16);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','CPUI', 'S',  4, 19);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','DAIS', 'D',  6, 23);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','STAT', 'S',  3, 29);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TDNR', 'S', 15, 32);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','CDGT', 'N',  1, 47);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TRNC', 'S',  4, 48);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TCNR', 'S', 15, 52);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TCND', 'N',  1, 67);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','TACN', 'S',  5, 68);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','CDGT2','N',  1, 73);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','FORM', 'S',  1, 74);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','PXNM', 'S', 49, 75);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALTP', 'S',  1,124);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNF', 'S', 16,125);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNT', 'N',  4,141);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALNC2','N',  8,145);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALTP2','S',  1,153);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNF2','S', 16,154);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNT2','N',  4,170);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALNC3','N',  8,174);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALTP3','S',  1,182);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNF3','S', 16,183);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNT3','N',  4,199);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALNC4','N',  8,203);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ALTP4','S',  1,211);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNF4','S', 16,212);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','SCNT4','N',  4,228);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ESAC', 'S', 14,232);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','DISI', 'S',  1,246);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','ISOC', 'S',  2,247);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','AGNT', 'S',  5,249);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','EXCH', 'N',  1,254);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','2','NTAC', 'N',  1,255);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN1','N',  4,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN1','S', 15, 12);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT1','N',  1, 27);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN2','N',  4, 28);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN2','S', 15, 32);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT2','N',  1, 47);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN3','N',  4, 48);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN3','S', 15, 52);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT3','N',  1, 67);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN4','N',  4, 68);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN4','S', 15, 72);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT4','N',  1, 87);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN5','N',  4, 88);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN5','S', 15, 92);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT5','N',  1,107);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN6','N',  4,108);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN6','S', 15,112);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT6','N',  1,127);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RCPN7','N',  4,128);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RTDN7','S', 15,132);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','CDGT7','N',  1,147);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','DIRD', 'D',  6,148);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','TOUR', 'S', 15,154);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','ORNI', 'S', 19,169);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','3','RESD', 'S', 68,188);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','PNRR', 'S', 13,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TODC', 'S', 14, 21);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','INLS', 'S',  4, 35);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TKMI', 'S',  1, 39);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','ORIN', 'S', 32, 40);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TOUR', 'S', 15, 72);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','FARE', 'S', 11, 87);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','EQFR', 'S', 11, 98);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TAXA1','S', 11,109);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TAXA2','S', 11,120);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TAXA3','S', 11,131);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','TOTL', 'S', 11,142);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','NTSI', 'S',  4,153);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','SASI', 'S',  4,157);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','CLID', 'S',  8,161);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','BAID', 'S',  6,169);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','PXDA', 'S', 49,175);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','VLNC', 'N',  8,224);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','BOON', 'S', 10,232);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','BEOT', 'S',  1,242);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','4','RESD', 'S', 13,243);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','AEBA', 'N', 11,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','ETTS', 'N', 11, 19);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT1','S',  8, 30);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA1','N', 11, 38);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT2','S',  8, 49);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA2','N', 11, 57);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT3','S',  8, 68);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA3','N', 11, 76);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TDAM', 'N', 11, 87);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','CUTP', 'S',  4, 98);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TOCA', 'N', 11,102);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TCIN', 'S',  1,113);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT4','S',  8,114);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA4','N', 11,122);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT5','S',  8,133);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA5','N', 11,141);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFT6','S',  8,152);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TMFA6','N', 11,160);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COTP1','S',  6,171);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','CORT1','N',  5,177);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COAM1','N', 11,182);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COTP2','S',  6,193);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','CORT2','N',  5,199);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COAM2','N', 11,204);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COTP3','S',  6,215);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','CORT3','N',  5,221);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','COAM3','N', 11,226);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','NRID', 'S',  2,237);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','APBC', 'N', 11,239);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','5','TCTP', 'S',  6,250);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ORAC1','S',  5,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','DSTC1','S',  5, 13);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FFRF1','S', 16, 18);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','CARR1','S',  4, 34);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ROID1','S',  1, 38);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','RBKD1','S',  2, 39);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDA1','D',  5, 41);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NBDA1','S',  5, 46);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NADA1','S',  5, 51);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBTD1','S', 15, 56);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTNR1','S',  5, 71);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDT1','S',  5, 76);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBAL1','S',  3, 81);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBST1','S',  2, 84);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','SEGI1','N',  1, 86);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','STPO1','S',  1, 87);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ORAC2','S',  5, 88);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','DSTC2','S',  5, 93);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FFRF2','S', 16, 98);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','CARR2','S',  4,114);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ROID2','S',  1,118);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','RBKD2','S',  2,119);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDA2','S',  5,121);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NBDA2','S',  5,126);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NADA2','S',  5,131);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBTD2','S', 15,136);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTNR2','S',  5,151);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDT2','S',  5,156);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBAL2','S',  3,161);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBST2','S',  2,164);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','SEGI2','N',  1,166);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','STPO2','S',  1,167);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ORAC3','S',  5,168);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','DSTC3','S',  5,173);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FFRF3','S', 16,178);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','CARR3','S',  4,194);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','ROID3','S',  1,198);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','RBKD3','S',  2,199);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDA3','S',  5,201);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NBDA3','S',  5,206);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','NADA3','S',  5,211);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBTD3','S', 15,216);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTNR3','S',  5,231);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FTDT3','S',  5,236);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBAL3','S',  3,241);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','FBST3','S',  2,244);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','SEGI3','N',  1,246);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','STPO3','S',  1,247);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','6','RESD', 'S',  8,248);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','FRCA1','S', 87,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','FRCS1','N',  1, 95);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','FRCA2','S', 87, 96);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','FRCS2','N',  1,183);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','FCMI', 'S',  1,184);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','7','RESD', 'S', 71,185);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPAC1','S', 19,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPAM1','N', 11, 27);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','APLC1','S',  6, 38);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CUTP1','S',  4, 44);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','EXPC1','S',  2, 48);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPTP1','S', 10, 50);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','EXDA1','S',  4, 60);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CSTF1','S', 27, 64);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CRCC1','S',  1, 91);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','AVCD1','S',  2, 92);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','SAPP1','S',  1, 94);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPTI1','S', 25, 95);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','AUTA1','S', 11,120);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPAC2','S', 19,131);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPAM2','N', 11,150);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','APLC2','S',  6,161);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CUTP2','S',  4,167);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','EXPC2','S',  2,171);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPTP2','S', 10,173);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','EXDA2','S',  4,183);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CSTF2','S', 27,187);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','CRCC2','S',  1,214);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','AVCD2','S',  2,215);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','SAPP2','S',  1,217);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','FPTI2','S', 25,218);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','AUTA2','S', 11,243);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','8','RESD', 'S',  2,254);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','ENRS', 'S',147,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','FPIN1','S', 50,155);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','FPIN2','S', 50,205);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','9','RESD', 'S',  1,255);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','TRNN', 'N',  6,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','TDNR', 'S', 15,  8);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CDGT', 'N',  1, 23);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPNR', 'N',  1, 24);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP1','S',  4, 25);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP2','S',  4, 29);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPFR', 'N', 11, 33);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPFE', 'N', 11, 44);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPFP', 'N', 11, 55);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTC1','S',  3, 66);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTN1','N',  5, 69);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP3','S',  4, 74);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTR1','N', 11, 78);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTE1','N', 11, 89);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTP1','N', 11,100);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTC2','S',  3,111);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTN2','N',  5,114);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP4','S',  4,119);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTR2','N', 11,123);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTE2','N', 11,134);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTP2','N', 11,145);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTC3','S',  3,156);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTN3','N',  5,159);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP5','S',  4,164);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTR3','N', 11,168);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTE3','N', 11,179);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTP3','N', 11,190);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTC4','S',  3,201);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTN4','N',  5,204);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CUTP6','S',  4,209);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTR4','N', 11,213);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTE4','N', 11,224);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','CPTP4','N', 11,235);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','REFT', 'S',  1,246);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','F','RESD', 'S',  9,247);

INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','Z','RCID', 'S',  1,  1);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','Z','RRDC', 'N', 11,  2);
INSERT INTO ssod_fields(version,rec_id,name,type,length,position) VALUES('1.2','Z','RESD', 'S',243, 13);



