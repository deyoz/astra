create or replace PROCEDURE SP_WBA_DEADLOAD_DETAILS(cXML_in IN clob, cXML_out OUT CLOB)
AS
-- ���� ������ � ����㧪�� ������
-- ������: ����⮢ �.�.
cXML_data XMLType; P_ID number:=-1;  vID_AC number; vID_WS number; vID_BORT number; vID_SL number; P_IDN number;
TABLEVAR varchar(40) := 'DeadloadDetails';
REC_COUNT number:=0;
REC_COUNT2 number:=0;
BEGIN
  -- ������� �室��� ��ࠬ���
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  -- ����� ���������� �������������� �����
  -- �᫨ � Calcs ���� ����� �� ३�� - ��६ ���㤠, ���� �� AHM ��� �� �����誨
  select count(t1.ELEM_ID) into REC_COUNT
  from WB_CALCS_XML t1
  where t1.ELEM_ID = P_ID and t1.DATA_NAME = TABLEVAR;

  if REC_COUNT > 0 then
    begin
      select t1.XML_DATA
      INTO cXML_out
      from
      (
        select t1.XML_DATA
        from WB_CALCS_XML t1
        where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ID
      ) t1
      where rownum <= 1;

      if cXML_out is null then
        cXML_out := '';
      end if;
    end;
  end if;

  if (REC_COUNT <= 0) or (cXML_out is null) then
    -- ��६ ����� �� 㬮�砭��
      cXML_out := '';
  end if;
END SP_WBA_DEADLOAD_DETAILS;
/
