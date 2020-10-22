create or replace PROCEDURE SP_WB_REF_WS_AIR_SUP_INFO(cXML_in IN clob,
                                                        cXML_out OUT CLOB)
AS

P_LANG varchar2(50):='';
P_ID number:=-1;
cXML_data_i clob:='';
cXML_data_d clob:='';
cXML_data_r clob:='';
cXML_data_c clob:='';
V_R_COUNT number;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(i.id) into V_R_COUNT
    from WB_REF_WS_AIR_SUPPL_INFO_ITEM i join WB_REF_WS_AIR_SUP_INFO_ADV_COL da
    on 1=1 join WB_REF_WS_AIR_SUP_INFO_DET_COL dd
       on dd.ADV_COL_ID=da.id;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("interface", xmlattributes(to_char(e.item_id) "item_id",
                                                              e.item_name "item_name",
                                                                to_char(e.adv_col_id) "adv_col_id",
                                                                  e.adv_col_name "adv_col_name",
                                                                    to_char(e.det_col_id) "det_col_id",
                                                                      e.det_col_name "det_col_name"))).getClobVal() into cXML_data_i
        from (select distinct i.id as item_id,

                              case when P_LANG='ENG' then i.item_eng else i.item_rus end item_name,

                              dd.adv_col_id,
                              da.name adv_col_name,
                              dd.id det_col_id,
                              dd.name det_col_name,
                              da.sort_prior adv_sort_prior,
                              dd.sort_prior det_sort_prior
            from WB_REF_WS_AIR_SUPPL_INFO_ITEM i join WB_REF_WS_AIR_SUP_INFO_ADV_COL da
            on 1=1 join WB_REF_WS_AIR_SUP_INFO_DET_COL dd
               on dd.ADV_COL_ID=da.id
            order by case when P_LANG='ENG' then i.item_eng else i.item_rus end,
                     da.sort_prior,
                     da.name,
                     dd.sort_prior,
                     dd.name) e
        order by e.item_name,
                 e.adv_sort_prior,
                 e.adv_col_name,
                 e.det_sort_prior,
                 e.det_col_name;

        cXML_out:=cXML_out||cXML_data_i;
      end;
    end if;

    select count(od.id) into V_R_COUNT
    from WB_REF_WS_AIR_SUPPL_INFO_ADV od
    where od.id=P_ID;

    if V_R_COUNT>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("data", xmlattributes(to_char(e.item_id) "item_id",
                                                         to_char(e.det_col_id) "det_col_id",
                                                           to_char(e.is_check) "is_check"))).getClobVal() into cXML_data_d
        from (select i.id as item_id,
                     dd.id det_col_id,
                     od.is_check
             from WB_REF_WS_AIR_SUPPL_INFO_ITEM i join WB_REF_WS_AIR_SUP_INFO_ADV_COL da
             on 1=1 join WB_REF_WS_AIR_SUP_INFO_DET_COL dd
                on dd.ADV_COL_ID=da.id join WB_REF_WS_AIR_SUPPL_INFO_DATA od
                   on od.ITEM_ID=i.id and
                      od.DET_COL_ID=dd.id and
                      od.adv_id=P_ID
             order by i.item_ENG,
                      da.sort_prior,
                      da.name,
                      dd.sort_prior,
                      dd.name) e;

        cXML_out:=cXML_out||cXML_data_d;

        SELECT XMLAGG(XMLELEMENT("remark", xmlattributes(to_char(item_id) "item_id",
                                                           remark "remark"))).getClobVal() into cXML_data_r
        from WB_REF_WS_AIR_SUPPL_INFO_ADV_I
        where adv_id=P_ID;

        cXML_out:=cXML_out||cXML_data_r;

        SELECT XMLAGG(XMLELEMENT("user_change", xmlattributes(e.u_name "U_NAME",
                                                                e.u_ip "U_IP",
                                                                  e.u_host_name "U_HOST_NAME",
                                                                    e.date_write "DATE_WRITE",
                                                                      e.date_from_d "date_from_d",
                                                                        e.date_from_m "date_from_m",
                                                                          e.date_from_y "date_from_y"))).getClobVal() into cXML_data_c
        from (select U_NAME,
                     U_IP,
                     U_HOST_NAME,
                     to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE,
                     to_char(EXTRACT(DAY FROM date_from)) date_from_d,
                     to_char(EXTRACT(MONTH FROM date_from)) date_from_m,
                     to_char(EXTRACT(YEAR FROM date_from)) date_from_y
              from WB_REF_WS_AIR_SUPPL_INFO_ADV
              where ID=P_ID) e
        where ROWNUM=1;

        cXML_out:=cXML_out||cXML_data_c;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';

  END SP_WB_REF_WS_AIR_SUP_INFO;
/
