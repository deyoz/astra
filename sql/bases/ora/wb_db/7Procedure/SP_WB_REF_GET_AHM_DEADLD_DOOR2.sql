create or replace PROCEDURE SP_WB_REF_GET_AHM_DEADLD_DOOR2 (cXML_in IN clob, cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select t1.XML_VALUE
  INTO cXML_data
  from
  (
    select t1.XML_VALUE
    from WB_TMP_XML t1
    where t1.TABLENAME = 'AHMDeadloadDoors' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;
/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Расположение дверей грузовых отсеков борта.
-->
<root name="get_ahm_deadload_doors" result="ok">
    <deck Name="L">
        <hold Name="FWD">
            <door
                BalanceArmFWD="506.5"
                BalanceArmAFT="607.95"
                Side="R"
            />
        </hold>
        <hold Name="AFT">
            <door
                BalanceArmFWD="1683.05"
                BalanceArmAFT="1784.95"
                Side="R"
            />
        </hold>
        <hold Name="BULK">
            <door
                BalanceArmFWD="1899.4"
                BalanceArmAFT="1932.6"
                Side="R"
            />
        </hold>
    </deck>
    <deck Name="M">
        <hold Name="FWD">
            <door
                BalanceArmFWD="376"
                BalanceArmAFT="424"
                Side="R"
            />
        </hold>
        <hold Name="AFT">
            <door
                BalanceArmFWD="857"
                BalanceArmAFT="905"
                Side="L"
            />
        </hold>
    </deck>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_AHM_DEADLD_DOOR2;
/
