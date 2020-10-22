create or replace procedure SP_WB_REF_WS_AIR_S_L_C_A_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_IDN number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';

V_R_COUNT number:=0;
cXML_data CLOB:='';
  begin

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IDN[1]')) into P_IDN from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_S_L_C_ADV
    where idn=P_IDN;

    if V_R_COUNT=0 then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_S_L_C_IDN
        where id=P_IDN;

        if V_R_COUNT>0 then
          begin
            insert into WB_REF_WS_AIR_S_L_C_ADV (ID,
	                                                 ID_AC,
	                                                   ID_WS,
	                                                     ID_BORT,
                                                 	       ADV_ID,
                                                 	         IDN,
                                                 	           DATE_FROM,
	                                              	             CH_CAI_BALANCE_ARM,
                                            	                   CH_CAI_INDEX_UNIT,
	                                            	                   CH_CI_BALANCE_ARM,
	                                            	                     CH_CI_INDEX_UNIT,
	                                             	                       CH_USE_AS_DEFAULT,
	                                             	                         U_NAME,
	                                              	                         U_IP,
	                                              	                           U_HOST_NAME,
	                                              	                             DATE_WRITE)
            select SEC_WB_REF_WS_AIR_S_L_C_ADV.nextval,
                     ID_AC,
	                     ID_WS,
	                       ID_BORT,
                           ADV_ID,
                             ID,
                               null,
                                 1,
                                   0,
                                     1,
                                       0,
                                         0,
                                           P_U_NAME,
	                                           P_U_IP,
	                                             P_U_HOST_NAME,
                                                 SYSDATE()
            from WB_REF_WS_AIR_S_L_C_IDN
            where id=P_IDN;

            commit;
          end;
        end if;
      end;
    end if;

  select count(id) into V_R_COUNT
  from WB_REF_WS_AIR_S_L_C_ADV
  where idn=P_IDN;

  if V_R_COUNT>0 then
    begin
      SELECT XMLAGG(XMLELEMENT("list", xmlattributes(to_char(e.id) "id",
                                                       e.table_name "table_name",
                                                         to_char(e.CH_CAI_BALANCE_ARM) as "CH_CAI_BALANCE_ARM",
                                                           to_char(e.CH_CAI_INDEX_UNIT) as "CH_CAI_INDEX_UNIT",
                                                             to_char(e.CH_CI_BALANCE_ARM) as "CH_CI_BALANCE_ARM",
                                                               to_char(e.CH_CI_INDEX_UNIT) as "CH_CI_INDEX_UNIT",
                                                                 to_char(e.CH_USE_AS_DEFAULT) as "CH_USE_AS_DEFAULT",
                                                                   e.U_NAME as "U_NAME",
                                                                     e.U_IP as "U_IP",
                                                                       e.U_HOST_NAME as "U_HOST_NAME",
                                                                         e.DATE_WRITE as "DATE_WRITE"))).getClobVal() into cXML_data
      from (select a.id,
                     i.table_name,
                       a.CH_CAI_BALANCE_ARM,
                         a.CH_CAI_INDEX_UNIT,
	                         a.CH_CI_BALANCE_ARM,
	                           a.CH_CI_INDEX_UNIT,
	                             a.CH_USE_AS_DEFAULT,
                                 a.U_NAME,
                                   a.U_IP,
                                     a.U_HOST_NAME,
                                       to_char(a.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
            from WB_REF_WS_AIR_S_L_C_IDN i join WB_REF_WS_AIR_S_L_C_ADV a
            on i.id=P_IDN and
               i.id=a.idn) e;

      cXML_out:=cXML_out||cXML_data;
    end;
  end if;


  cXML_out:=cXML_out||'</root>';

  end SP_WB_REF_WS_AIR_S_L_C_A_DATA;
/
