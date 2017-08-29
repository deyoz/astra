$(defmacro get_svc_availability_resp
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_availability>
    <svc_list passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\">
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0L1\" emd_type=\"EMD-A\" carry_on=\"false\" group=\"BG\" subgroup=\"SP\">
        <name language=\"en\">FISHING EQUIPMENT UPTO44LB20KG</name>
        <name language=\"ru\">���������� ������ �� 44� 20��</name>
        <description>FI</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0M5\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">CARRYY 10KG 22LB UPTO39LI 100LCM</name>
        <name language=\"ru\">��� ����� 10�� 22� �� 39�100��</name>
        <description>10</description>
        <description>4Q</description>
      </svc>
      <svc company=\"UT\" service_type=\"C\" rfic=\"C\" rfisc=\"0MN\" emd_type=\"EMD-A\" carry_on=\"true\" group=\"BG\" subgroup=\"CY\">
        <name language=\"en\">PET IN CABIN</name>
        <name language=\"ru\">�������� �������� � ������</name>
        <description>PE</description>
      </svc>
      <free_baggage_norm company=\"UT\" type=\"piece\">
        <text language=\"en\">Free baggage allowance 1PC
1ST checked bag:         FIREARMS UP TO 23KG (04U)
Free baggage exception:  PIECE OF BAG UPTO23KG 203LCM , PIECE OF BAG UPTO 23KG
                           203LCM (0GP)
                     and PC OF BAG FROM 23KG UPTO 32KG (0M6)
        </text>
        <text language=\"ru\">��ଠ ��ᯫ�⭮�� ������ 1��
1-� ���� ��ᯫ�⭮�� ������:  ����� ����������� �� 50�23�� (04U)
�� �室�� � ��ᯫ���� ����:  � ��: 1� �� ����� 23��/203�� , � ��  1� �� �����
                                 23�� 203�� (0GP)
                             � ����� ������ ��� ��24�� ��32�� (0M6)
        </text>
      </free_baggage_norm>
      <free_carry_on_norm company=\"UT\" type=\"piece\">
        <text language=\"en\">Carry-on bag 1PC
1ST carry bag:       CABIN BAG UPTO 10KG 55X40X20CM (0MJ)
        </text>
        <text language=\"ru\">��ᯫ�⭠� ��筠� ����� 1��
1-� ���� ��筮� �����:               ������ ����� ��10�� 55�40�20�� (0MJ)
        </text>
      </free_carry_on_norm>
    </svc_list>
  </svc_availability>
</answer>})
}
)


$(defmacro get_svc_payment_status_resp
{
$(utf8 {<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<answer>
  <svc_payment_status>
    <free_baggage_norm passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" company=\"UT\" type=\"piece\">
      <text language=\"en\">Free baggage allowance 1PC 1ST checked bag:         PIECE OF BAG UPTO23KG 203LCM , PIECE OF BAG UPTO 23KG 203LCM (0GP) or FIREARMS UP TO 23KG (04U)</text>
      <text language=\"ru\">��ଠ ��ᯫ�⭮�� ������ 1�� 1-� ���� ��ᯫ�⭮�� ������:  � ��: 1� �� ����� 23��/203�� , � ��  1� �� ����� 23�� 203�� (0GP) ��� ����� ����������� �� 50�23�� (04U)</text>
    </free_baggage_norm>
    <free_carry_on_norm passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" company=\"UT\" type=\"piece\">
      <text language=\"en\">Carry-on bag 1PC 1ST carry bag:       CABIN BAG UPTO 10KG 55X40X20CM (0MJ)</text>
      <text language=\"ru\">��ᯫ�⭠� ��筠� ����� 1�� 1-� ���� ��筮� �����:               ������ ����� ��10�� 55�40�20�� (0MJ)</text>
    </free_carry_on_norm>
    <svc passenger-id=\"$(last_generated_pax_id)\" segment-id=\"0\" rfisc=\"0L1\" service_type=\"C\" payment_status=\"free\" company=\"UT\"/>
  </svc_payment_status>
</answer>})
}
)


$(defmacro get_svc_availability_invalid_resp
{
$(utf8 invalid_answer)
}
)
