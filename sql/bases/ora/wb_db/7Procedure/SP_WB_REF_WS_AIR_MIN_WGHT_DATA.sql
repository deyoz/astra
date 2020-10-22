create or replace procedure SP_WB_REF_WS_AIR_MIN_WGHT_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin
   cXML_out:='<?xml version="1.0" ?><root>';

   select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

   select count(id),
            id_ac,
              id_ws,
                id_bort
          into V_R_COUNT,
                 P_ID_AC,
                   P_ID_WS,
                     P_ID_BORT
   from WB_REF_WS_AIR_MIN_WGHT_IDN
   where id=P_ID
   group by id_ac,
              id_ws,
                id_bort;

    if V_R_COUNT>0 then
      begin
        select '<adv_data TABLE_NAME="'||i.TABLE_NAME||'" '||
                         'CONDITION="'||WB_CLEAR_XML(i.CONDITION)||'" '||
                         'TYPE="'||WB_CLEAR_XML(i.TYPE)||'" '||
                         'TYPE_FROM="'||nvl(to_char(i.TYPE_FROM), 'NULL')||'" '||
                         'TYPE_TO="'||nvl(to_char(i.TYPE_TO), 'NULL')||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
           FROM WB_REF_WS_AIR_MIN_WGHT_IDN i
           WHERE i.ID=P_ID;

        cXML_out:=cXML_out||cXML_data;

        select count(u.id) into V_R_COUNT
        from WB_REF_WS_AIR_MIN_WGHT_USE a join WB_REF_WS_AIR_GR_CH_USE_ITEMS u
        on a.idn=P_ID and
           u.id=a.USE_ITEM_ID;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("use_data", xmlattributes(q.ITEM_NAME as "ITEM_NAME",
                                                                 to_char(q.USE_ITEM_ID) as "use_item_id",
                                                                   q.value_1 as "value_1",
                                                                     q.value_2 as "value_2"))).getClobVal() into cXML_data
            from (select ui.ITEM_NAME,
                           case when u.value_1 is NULL
                                then 'NULL'
                                else to_char(u.value_1)
                           end value_1,
                             case when u.value_2 is NULL
                                  then 'NULL'
                                  else to_char(u.value_2)
                             end value_2,
                               u.USE_ITEM_ID
                  from WB_REF_WS_AIR_MIN_WGHT_USE u join WB_REF_WS_AIR_GR_CH_USE_ITEMS ui
                  on u.idn=P_ID and
                     ui.id=u.USE_ITEM_ID
                  order by ui.ITEM_NAME,
                           nvl(u.value_1, -999999999999999999),
                           nvl(u.value_2, 99999999999999999)) q;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;



     end;
    end if;

  cXML_out:=cXML_out||'</root>';

end SP_WB_REF_WS_AIR_MIN_WGHT_DATA;
/
