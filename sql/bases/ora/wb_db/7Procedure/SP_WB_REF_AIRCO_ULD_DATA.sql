create or replace procedure SP_WB_REF_AIRCO_ULD_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
cXML_data clob;
V_R_COUNT number:=0;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_ULD_TYPES;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("uld_types_list", xmlattributes(to_char(e.id) "id",
                                                               e.name "name"))).getClobVal() into cXML_data
        from (select id,
                       name
              from WB_REF_ULD_TYPES
              order by name) e
        order by e.name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_ULD_ATA;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("ata_list", xmlattributes(e.id "id",
                                                             e.type_id,
                                                               e.name "name"))).getClobVal() into cXML_data
        from (select to_char(id) id,
                       to_char(type_id) type_id,
                         name
              from WB_REF_ULD_ATA
              order by type_id,
                       name) e
        order by e.type_id,
                 e.name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_ULD_IATA;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("iata_list", xmlattributes(e.id "id",
                                                              e.type_id "type_id",
                                                                e.name "name"))).getClobVal() into cXML_data
        from (select to_char(id) id,
                       to_char(type_id) type_id,
                         name
              from WB_REF_ULD_IATA
              order by type_id,
                       name) e
        order by e.type_id,
                 e.name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_ULD_IATA_ATA;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("ata_iata_rel_list", xmlattributes(e.ata_id "ata_id",
                                                                      e.iata_id "iata_id"))).getClobVal() into cXML_data
        from (select distinct to_char(ata.id) ata_id,
                                to_char(iata.id) iata_id,
                                  ata.type_id ata_type_id,
                                    ata.name ata_name,
                                      iata.name iata_name
              from WB_REF_ULD_ATA ata join WB_REF_ULD_IATA_ATA rel
              on ata.id=rel.ATA_ID join WB_REF_ULD_iATA iata
                 on iata.id=rel.iata_id
              order by ata.type_id,
                       ata.name,
                      iata.name) e
        order by e.ata_type_id,
                 e.ata_name,
                 e.iata_name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_ULD
    where id_ac=P_ID_AC;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("uld_table_data", xmlattributes(e.id "id",
                                                                   e.uld_type_name "uld_type_name",
                                                                     e.uld_ata_name "uld_ata_name",
                                                                       e.uld_iata_name "uld_iata_name",
                                                                         e.BEG_SER_NUM "BEG_SER_NUM",
                                                                           e.END_SER_NUM "END_SER_NUM",
                                                                             e.OWNER_CODE "OWNER_CODE",
                                                                               e.TARE_WEIGHT "TARE_WEIGHT",
                                                                                 e.MAX_WEIGHT "MAX_WEIGHT",
                                                                                   e.MAX_VOLUME "MAX_VOLUME",
                                                                                     e.WIDTH "WIDTH",
                                                                                       e.LENGTH "LENGTH",
                                                                                         e.HEIGHT "HEIGHT",
                                                                                           e.by_default "by_default",
                                                                                             e.U_NAME "U_NAME",
                                                                                               e.U_IP "U_IP",
                                                                                                 e.U_HOST_NAME "U_HOST_NAME",
                                                                                                   e.date_write "date_write"))).getClobVal() into cXML_data
        from (select to_char(acd.id) id,
                       ut.name uld_type_name,
                         ata.name uld_ata_name,
                           iata.name uld_iata_name,
                             nvl(acd.BEG_SER_NUM, 'NULL') BEG_SER_NUM,
                               nvl(acd.END_SER_NUM, 'NULL') END_SER_NUM,
                                 nvl(acd.OWNER_CODE, 'NULL') OWNER_CODE,
                                   case when acd.TARE_WEIGHT is null then 'NULL' else to_char(acd.TARE_WEIGHT) end TARE_WEIGHT,
	                                   case when acd.MAX_WEIGHT is null then 'NULL' else to_char(acd.MAX_WEIGHT) end MAX_WEIGHT,
	                                     case when acd.MAX_VOLUME is null then 'NULL' else to_char(acd.MAX_VOLUME) end MAX_VOLUME,
	                                       case when acd.WIDTH is null then 'NULL' else to_char(acd.WIDTH) end WIDTH,
	                                         case when acd.LENGTH is null then 'NULL' else to_char(acd.LENGTH) end LENGTH,
	                                           case when acd.HEIGHT is null then 'NULL' else to_char(acd.HEIGHT) end HEIGHT,
                                               to_char(acd.by_default) by_default,
                                                 acd.U_NAME,
                                                   acd.U_IP,
                                                     acd.U_HOST_NAME,
                                                       to_char(acd.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') date_write
              from WB_REF_AIRCO_ULD acd join WB_REF_ULD_TYPES ut
              on acd.id_ac=P_ID_AC and
                 acd.ULD_TYPE_ID=ut.id join WB_REF_ULD_ATA ata
                 on acd.ULD_ATA_ID=ata.id join WB_REF_ULD_IATA iata
                    on acd.ULD_IATA_ID=iata.id
              order by ut.name,
                       ata.name,
                       iata.name) e
        order by e.uld_type_name,
                   e.uld_ata_name,
                     e.uld_iata_name;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_AIRCO_ULD_DATA;
/
