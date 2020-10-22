create or replace procedure SP_WB_REF_WS_AIR_CFV_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
cXML_data clob;
V_R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_CFV_ADV
    where idn=P_ID;

    if V_R_COUNT>0 then
      begin
         cXML_out:='<?xml version="1.0" ?><root>';

        select '<adv_data TABLE_NAME="'||WB_CLEAR_XML(a.TABLE_NAME)||'" '||
                         'PROC_NAME="'||WB_CLEAR_XML(i.PROC_NAME)||'" '||
	                       'REMARKS="'||WB_CLEAR_XML(i.REMARKS)||'" '||
	                       'DENSITY="'||nvl(to_char(i.DENSITY), 'NULL')||'" '||
	                       'MAX_VOLUME="'||nvl(to_char(i.MAX_VOLUME), 'NULL')||'" '||
	                       'MAX_WEIGHT="'||nvl(to_char(i.MAX_WEIGHT), 'NULL')||'" '||
	                       'CH_WEIGHT="'||to_char(i.CH_WEIGHT)||'" '||
	                       'CH_VOLUME="'||to_char(i.CH_VOLUME)||'" '||
	                       'CH_BALANCE_ARM="'||to_char(i.CH_BALANCE_ARM)||'" '||
	                       'CH_INDEX_UNIT="'||to_char(i.CH_INDEX_UNIT)||'" '||
	                       'CH_STANDART="'||to_char(i.CH_STANDART)||'" '||
	                       'CH_NON_STANDART="'||to_char(i.CH_NON_STANDART)||'" '||
	                       'CH_USE_BY_DEFAULT="'||to_char(i.CH_USE_BY_DEFAULT)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           INTO cXML_data
           FROM WB_REF_WS_AIR_CFV_ADV i join WB_REF_WS_AIR_CFV_IDN a
           on i.IDN=a.id and
              a.id=P_ID;

        cXML_out:=cXML_out||cXML_data;

        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_CFV_TBL
        where idn=P_ID;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("tbl_data", xmlattributes(e.WEIGHT "WEIGHT",
                                                                 e.VOLUME as "VOLUME",
                                                                   e.ARM as "ARM",
                                                                     e.INDEX_UNIT as "INDEX_UNIT"))).getClobVal() into cXML_data
            from (select case when l.WEIGHT is null then 'NULL' else to_char(l.WEIGHT) end WEIGHT,
                           case when l.VOLUME is null then 'NULL' else to_char(l.VOLUME) end VOLUME,
                             case when l.ARM is null then 'NULL' else to_char(l.ARM) end ARM,
                               case when l.INDEX_UNIT is null then 'NULL' else to_char(l.INDEX_UNIT) end INDEX_UNIT
                  from WB_REF_WS_AIR_CFV_TBL l
                  where l.idn=P_ID
                  order by l.WEIGHT) e;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;

        select count(b.id) into V_R_COUNT
        from WB_REF_WS_AIR_CFV_ADV a join WB_REF_AIRCO_WS_BORTS b
        on a.idn=P_ID and
           a.ID_AC=b.ID_AC and
           a.ID_WS=b.ID_WS;

        if V_R_COUNT>0 then
          begin
            SELECT XMLAGG(XMLELEMENT("bort_list_data", xmlattributes(e.id as "id",
                                                                       e.BORT_NUM as "BORT_NUM",
                                                                         e.is_checked as "IS_CHECKED"))).getClobVal() into cXML_data
            from (select to_char(b.id) as id,
                           b.BORT_NUM,
                             case when exists(select cb.id
                                              from WB_REF_WS_AIR_CFV_BORT cb
                                              where cb.id_bort=b.id and
                                                    cb.idn=P_ID)
                                  then '1'
                                  else '0'
                              end is_checked
                  from WB_REF_WS_AIR_CFV_ADV a join WB_REF_AIRCO_WS_BORTS b
                  on a.idn=P_ID and
                     a.ID_AC=b.ID_AC and
                     a.ID_WS=b.ID_WS
                  order by b.BORT_NUM) e
            order by e.BORT_NUM;

            cXML_out:=cXML_out||cXML_data;
          end;
        end if;


        cXML_out:=cXML_out||'</root>';
     end;
    end if;


end SP_WB_REF_WS_AIR_CFV_DATA;
/
