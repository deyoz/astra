create or replace PROCEDURE SP_WB_REF_GET_AHM_DEADLD_DECK2(cXML_in IN clob, cXML_out OUT CLOB)
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
    where t1.TABLENAME = 'AHMDeadloadDecks' and t1.ELEM_ID = P_ELEM_ID
  ) t1
  where rownum <= 1;

/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    Конфигурация грузовых отсеков борта.
-->
<root name="get_ahm_deadload_decks" result="ok">
    <deck Name="L"> <!-- Возможные значения "U","M","L". -->
        <hold
            Name="FWD"
            MaxWeight="1500"
            MaxVolume="280"
            LateralCentroid="0"
            LateralArmLH="-60"
            LateralArmLR="60"
            BalanceArmFWD="354"
            BalanceArmAFT="500"
        >
            <compartment
                Name="1"
                MaxWeight="703"
                MaxVolume="106"
                LateralCentroid="0"
                LateralArmLH="-60"
                LateralArmLR="60"
                BalanceArmFWD="354"
                BalanceArmAFT="429"
            >
                <bulk
                    Name="11"
                    MaxWeight="703"
                    MaxVolume=""
                    LateralCentroid="0"
                    LateralArmLH="-60"
                    LateralArmLR="60"
                    BalanceArmFWD="354"
                    BalanceArmAFT="429"
                />
            </compartment>
            <compartment
                Name="2"
                MaxWeight="869"
                MaxVolume="181"
                LateralCentroid="0"
                LateralArmLH="-60"
                LateralArmLR="60"
                BalanceArmFWD="429"
                BalanceArmAFT="500"
            >
                <bulk
                    Name="22"
                    MaxWeight="869"
                    MaxVolume="181"
                    LateralCentroid="0"
                    LateralArmLH="-60"
                    LateralArmLR="60"
                    BalanceArmFWD="429"
                    BalanceArmAFT="500"
                />
            </compartment>
        </hold>
        <hold
            Name="AFT"
            MaxWeight="2800"
            MaxVolume="500"
            LateralCentroid="0"
            LateralArmLH="-60"
            LateralArmLR="60"
            BalanceArmFWD="731"
            BalanceArmAFT="1008"
        >
            <compartment
                Name="3"
                MaxWeight="1756"
                MaxVolume=""
                LateralCentroid="0"
                LateralArmLH="-60"
                LateralArmLR="60"
                BalanceArmFWD="731"
                BalanceArmAFT="852"
            >
                <bulk
                    Name="33"
                    MaxWeight="1756"
                    MaxVolume=""
                    LateralCentroid="0"
                    LateralArmLH="-60"
                    LateralArmLR="60"
                    BalanceArmFWD="731"
                    BalanceArmAFT="852"
                />
            </compartment>
            <compartment
                Name="4"
                MaxWeight="1132"
                MaxVolume="216"
                LateralCentroid="0"
                LateralArmLH="-22"
                LateralArmLR="22"
                BalanceArmFWD="852"
                BalanceArmAFT="1008"
            >
                <bulk
                    Name="44"
                    MaxWeight="1132"
                    MaxVolume="216"
                    LateralCentroid="0"
                    LateralArmLH="-22"
                    LateralArmLR="22"
                    BalanceArmFWD="852"
                    BalanceArmAFT="1008"
                />
            </compartment>
        </hold>
    </deck>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_AHM_DEADLD_DECK2;
/
