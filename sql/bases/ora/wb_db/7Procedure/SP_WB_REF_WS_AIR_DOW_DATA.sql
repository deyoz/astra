create or replace procedure SP_WB_REF_WS_AIR_DOW_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
cXML_data clob;
V_R_COUNT number:=0;

V_IS_AC number:=1;
V_IS_WS number:=1;
V_IS_BORT number:=1;
V_ID number:=-1;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_IDN
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT=0 then
      begin
        if P_ID_AC>0 then
          begin
            select count(id) into V_IS_AC
            from WB_REF_AIRCOMPANY_KEY
            where id=P_ID_AC;
          end;
        end if;

        if P_ID_WS>0 then
          begin
            select count(id) into V_IS_WS
            from WB_REF_WS_TYPES
            where id=P_ID_WS;
          end;
        end if;

        if (V_IS_AC>0) and
             (V_IS_WS>0) and
               (V_IS_BORT>0) then
          begin
            V_ID:=SEC_WB_REF_WS_AIR_DOW_IDN.nextval();

            insert into WB_REF_WS_AIR_DOW_IDN (ID,
	                                               ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,	
	                                                     U_NAME,
	                                                       U_IP,
	                                                         U_HOST_NAME,
	                                                           DATE_WRITE)
            select V_ID,
                     P_ID_AC,
                       P_ID_WS,
                         P_ID_BORT,
                           P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 SYSDATE()
            from dual;

            insert into WB_REF_WS_AIR_DOW_ADV (ID,
	                                               ID_AC,
	                                                 ID_WS,
		                                                 ID_BORT,
                                                       IDN,
	                                                       DATE_FROM,
	                                                         CH_BASIC_WEIGHT,
	                                                           CH_DOW,
	                                                             REMARK,
		                                                             U_NAME,
		                                                               U_IP,
		                                                                 U_HOST_NAME,
		                                                                   DATE_WRITE)
            select SEC_WB_REF_WS_AIR_DOW_ADV.nextval,
                     P_ID_AC,
                       P_ID_WS,
                         P_ID_BORT,
                           V_ID,
                             NULL,
                               1,
                                 0,
                                   'EMPTY_STRING',
                                     P_U_NAME,
	                                     P_U_IP,
	                                       P_U_HOST_NAME,
                                           SYSDATE()
            from dual;

            commit;
          end;
        end if;
      end;
    end if;


    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_ADV
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select '<adv_data CH_BASIC_WEIGHT="'||to_char(i.CH_BASIC_WEIGHT)||'" '||
                         'CH_DOW="'||to_char(i.CH_DOW)||'" '||
                         'REMARK="'||WB_CLEAR_XML(nvl(i.REMARK, 'EMPTY_STRING'))||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
        INTO cXML_data
        from WB_REF_WS_AIR_DOW_ADV i
        where i.ID_AC=P_ID_AC and
              i.ID_WS=P_ID_WS and
              i.ID_BORT=P_ID_BORT;

        cXML_out:=cXML_out||cXML_data;

        select count(id) into V_R_COUNT
        from WB_REF_WS_DOW_SPEC;

        if V_R_COUNT>0 then
          begin
            select XMLAGG(XMLELEMENT("table_data",
                  xmlattributes(to_char(q.item_id) as "item_id",
                                  q.item_name as "item_name",
                                    to_char(q.IS_INCLUDED) as "IS_INCLUDED",
                                      to_char(q.IS_EXCLUDED) as "IS_EXCLUDED",
                                        q.REMARK as "REMARK"))).getClobVal() into cXML_data
            from (select distinct s.ID item_id,
                                    s.name item_name,
                                      nvl(i.IS_INCLUDED, 0) IS_INCLUDED,
                                        nvl(i.IS_EXCLUDED, 0) IS_EXCLUDED,
                                          nvl(i.remark, 'EMPTY_STRING') REMARK,
                                            s.sort_prior
                  from WB_REF_WS_AIR_DOW_ADV a join WB_REF_WS_DOW_SPEC s
                  on a.ID_AC=P_ID_AC and
                     a.ID_WS=P_ID_WS and
                     a.ID_BORT=P_ID_BORT left outer join WB_REF_WS_AIR_DOW_ITM i
                     on i.ITEM_ID=s.id and
                        i.adv_id=a.id
                  order by s.sort_prior) q
            order by q.sort_prior;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

      end;
    end if;

  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_DOW_DATA;
/
