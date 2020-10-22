create or replace procedure SP_WB_REF_WS_AIR_SEAT_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_PLAN_ID number:=-1;
P_SEAT_ID number:=-1;
P_CALC_PAR_LIST number:=0;
cXML_data clob;
V_R_COUNT number:=0;
begin

  cXML_out:='<?xml version="1.0" ?><root>';

  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PLAN_ID[1]')) into P_PLAN_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_SEAT_ID[1]')) into P_SEAT_ID from dual;
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_CALC_PAR_LIST[1]')) into P_CALC_PAR_LIST from dual;

  if nvl(P_CALC_PAR_LIST, 0)=1 then
    begin
      select count(id) into V_R_COUNT
      from WB_REF_WS_SEATS_PARAMS;

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("SEATS_PARAMS_LIST",
                      xmlattributes(q.id as "id",
                                      q.name as "name",
                                        q.remark as "remark"))).getClobVal() into cXML_data
          from (select to_char(id) id,
                         name,
                           remark,
                             sort_prior
                from WB_REF_WS_SEATS_PARAMS
                order by sort_prior) q
          order by q.sort_prior;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;

      select count(d.id) into V_R_COUNT
      from WB_REF_WS_AIR_S_L_P_S a join WB_REF_WS_AIR_S_L_P_S_P d
      on a.PLAN_ID=P_PLAN_ID and
         a.SEAT_ID=P_SEAT_ID and
         a.id=d.S_SEAT_ID;

      if V_R_COUNT>0 then
        begin
          select XMLAGG(XMLELEMENT("ADV_PARAM_DATA",
                  xmlattributes(to_char(d.PARAM_ID) as "PARAM_ID"))).getClobVal() into cXML_data
          from WB_REF_WS_AIR_S_L_P_S a join WB_REF_WS_AIR_S_L_P_S_P d
          on a.PLAN_ID=P_PLAN_ID and
             a.SEAT_ID=P_SEAT_ID and
             a.id=d.S_SEAT_ID;

          cXML_out:=cXML_out||cXML_data;
        end;
      end if;

    end;
  end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_P_S
  where PLAN_ID=P_PLAN_ID and
        SEAT_ID=P_SEAT_ID;

  if V_R_COUNT>0 then
    begin
      select XMLAGG(XMLELEMENT("ADV_DATA",
                  xmlattributes(id as "id",
                                  to_char(MAX_WEIGHT) as "MAX_WEIGHT",
                                    U_NAME as "U_NAME",
                                      U_IP as "U_IP",
                                        to_char(IS_SEAT) as "IS_SEAT",
                                          to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') as "DATE_WRITE",
                                            WB_WS_AIR_SEAT_PARAM_STR(id) as "PARAM_STR"))).getClobVal() into cXML_data
      from WB_REF_WS_AIR_S_L_P_S
      where PLAN_ID=P_PLAN_ID and
            SEAT_ID=P_SEAT_ID;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SEAT_DATA;
/
