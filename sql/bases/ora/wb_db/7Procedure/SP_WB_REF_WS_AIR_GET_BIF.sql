create or replace procedure SP_WB_REF_WS_AIR_GET_BIF
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

V_R_COUNT int:=0;
cXML_data CLOB:='';
  begin
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_BAS_IND_FORM
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT and
          DATE_FROM<=P_DATE_FROM;

    if V_R_COUNT>0 then
      begin
         select '<list REF_ARM="'||to_char(i.REF_ARM)||'" '||
                      'K_CONST="'||to_char(i.K_CONST)||'" '||
                      'C_CONST="'||to_char(i.C_CONST)||'" '||
                      'LEN_MAC_RC="'||to_char(i.LEN_MAC_RC)||'" '||
                      'LEMAC_LERC="'||to_char(i.LEMAC_LERC)||'"/>' into cXML_data
         from WB_REF_WS_AIR_BAS_IND_FORM i
         where i.ID_AC=P_ID_AC and
               i.ID_WS=P_ID_WS and
               i.ID_BORT=P_ID_BORT and
               i.DATE_FROM=(select max(ii.DATE_FROM)
                            from WB_REF_WS_AIR_BAS_IND_FORM ii
                            where ii.ID_AC=i.ID_AC and
                                  ii.ID_WS=i.ID_WS and
                                  ii.ID_BORT=i.ID_BORT and
                                  ii.DATE_FROM<=P_DATE_FROM);

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';

  end SP_WB_REF_WS_AIR_GET_BIF;
/
