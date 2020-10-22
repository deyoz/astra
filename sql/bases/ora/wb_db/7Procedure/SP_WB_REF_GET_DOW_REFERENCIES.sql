create or replace PROCEDURE SP_WB_REF_GET_DOW_REFERENCIES (cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
P_ELEM_ID number:=-1;
REC_COUNT number:=0;
BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select count(t1.ELEM_ID) into REC_COUNT
  from WB_TMP_XML t1
  where t1.ELEM_ID = P_ELEM_ID and t1.TABLENAME = 'AHMDOW';

  if REC_COUNT > 0 then
  begin
    select t1.XML_VALUE
    INTO cXML_data
    from
    (
      select t1.XML_VALUE
      from WB_TMP_XML t1
      where t1.TABLENAME = 'AHMDOW' and t1.ELEM_ID = P_ELEM_ID
    ) t1
    where rownum <= 1;
  end;
  else
  begin
    cXML_data := '<?xml version="1.0" encoding="utf-8"?><root name="get_ahm_dow_referencies" result="ok"></root>';
  end;
  end if;

/*
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<!--
    Ответ.
    "Справочники" DOW для данного рейса (элемента расписания).

-->
<root name="get_ahm_dow_referencies" result="ok">
    <!-- WeightConfigurationCodes могут отсутствовать. -->
    <WeightConfigurationCodes>
        <!-- code, обязан содержать атрибут Name и хотя бы один из
             остальных атрибутов, иначе он бессмысленен. -->
        <code
            Name="Standard"
            CrewCodeName="Domestic"
            PantryCodeName="Domestic"
            PotableWaterCodeName="Domestic"
            SWACodeNames0="Domestic"
            SWACodeNames1="Domestic"
            SWACodeNames2="Domestic"
        />
    </WeightConfigurationCodes>

    <CabinConfigurations>
        <code Name="C12Y144"/>
        <code Name="C6Y150"/>
    </CabinConfigurations>

    <!-- Все атрибуты во всех элементах <code> обязательные. -->
    <CrewCodes>
        <code
            Name="Domestic"
            Weight="75"
            Idx="-0.2"
            Included="N"
            Remarks="Domestic 2/4"
        />
        <code
            Name="Standard"
            Weight="80"
            Idx="-0.2"
            Included="Y"
            Remarks="STD 2/4"
        />
    </CrewCodes>

    <PantryCodes>
        <code
            Name="Domestic"
            Weight="75"
            Idx="-0.2"
            Included="Y"
            Remarks="G1-200/G4-350"
        />
    </PantryCodes>

    <PotableWaterCodes>
        <code
            Name="Domestic"
            Weight="75"
            Idx="-0.2"
            Included="Y"
            Remarks="AFT TANK 200"
        />
    </PotableWaterCodes>

    <SWACodes>
        <code
            Name="Domestic"
            Weight="75"
            Idx="-0.2"
            Included="Y"
            Remarks="TOW BAR"
        />
        <code
            Name="Wheel"
            Weight="25"
            Idx="-0.1"
            Included="Y"
            Remarks="wheel"
        />
        <code
            Name="Wheel 2"
            Weight="50"
            Idx="-0.2"
            Included="Y"
            Remarks="Two wheels"
        />
    </SWACodes>
</root>';
*/
cXML_out:=cXML_data;
END SP_WB_REF_GET_DOW_REFERENCIES;
/
