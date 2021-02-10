### �㭪�� cache:
### �।�����祭� ��� �ନ஢���� ����� �� �ନ���� �� ����㧪� ��� ��������� cache tables
### ��易⥫�� ��ࠬ���� (5 ���):
### $(cache <queryLogin>
###         <queryLang>
###         <�������� ��� (cache_tables.code)>
###         <����� ����䥩� (cache_tables.tid)>
###         <����� ������ (���筮 - ���ᨬ���� tid ������)>
###  )
### �᫨ 㪠���� ⮫쪮 ��易⥫�� ��ࠬ����, � �㤥� ����㧪� ������
### ��� �������� ��襩, �᫨ 㪠���� ����� ������, � �㤥� �ந������� ���筠� �����㧪� ������ (refresh)
###
### ��᫥ ��易⥫��� ��ࠬ��஢ ����� 㪠���� SQL-��ࠬ���� (���� ��� �����) ����室��� ��� �������� ��襩
### ��� �⮣� ���� �ᯮ�짮���� �㭪�� cache_sql_param:
### $(cache_sql_param <�������� ��ࠬ���> <⨯ ��ࠬ���> <���祭�� ��ࠬ���>)
### ⨯ ��ࠬ���: string/integer/double/datetime
###
### ��᫥ ��易⥫��� ��ࠬ��஢ � ����易⥫��� SQL-��ࠬ��஢ ����� 㪠�뢠�� ��ࠬ���� ��� ��������� ������
### �� �ਬ��塞�� ��������� ��⮨� �� ⨯� ��������� ��ப� ��� � ���祭�� ����� ��� ���������
### ⨯ ���������: insert/update/delete
### ����� ����� � ���祭�� ����� ࠧ�������� '=' (� �⮬ ��砥 �ਤ���� �ਬ����� ������ ����窨 ��� 䨣��� ᪮���, ���ਬ�� "���=���祭��")
### ����� ����� � ���祭�� ����� ⠪�� ࠧ�������� ':'
### �᫨ ���祭�� ���� �� 㪠����, ��� �ਭ����� ���祭�� NULL
### � ��饬 ��砥 � ������� ��ࠬ��஢ ����� ���� �ਬ����� ������⢮ ��������� �����६����:
### <⨯_���������1> <�����_�����_�_���祭��1> <⨯_���������2> <�����_�����_�_���祭��2> ...
###
### ��� ����祭�� ⥪�饩 ���ᨨ ����䥩� ��� ����� �ᯮ�짮���� $(cache_iface_ver <�������� ���>)
### �� �ਬ������ ��������� �����, �⮡� ����� ����䥩� ᮢ������ � ⥪�饩
###
### ����� ��襩 � ����� ��������� case insensitive
#######################################################################################################################################

$(defmacro combine_brd_with_reg
  point_dep
  hall
{
$(cache PIKE RU TRIP_BRD_WITH_REG $(cache_iface_ver TRIP_BRD_WITH_REG) ""
  $(cache_sql_param point_id integer $(point_dep))
  insert point_id:$(point_dep) type:101 pr_misc:1 hall:$(hall))
})

$(defmacro combine_exam_with_brd
  point_dep
  hall
{
$(cache PIKE RU TRIP_EXAM_WITH_BRD $(cache_iface_ver TRIP_EXAM_WITH_BRD) ""
  $(cache_sql_param point_id integer $(point_dep))
  insert point_id:$(point_dep) type:102 pr_misc:1 hall:$(hall))
})

$(defmacro deny_ets_interactive
  airline
  flt_no
  airp_dep
{
$(cache PIKE RU MISC_SET $(cache_iface_ver MISC_SET) ""
  insert type_code:11
         airline:$(get_elem_id etAirline $(airline))
         flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         pr_misc:1
 )
})

$(defmacro prepare_bp_printing
  airline
  flt_no
  airp_dep
  class
  bp_type
{
$(cache PIKE RU AIRLINE_BP_SET $(cache_iface_ver AIRLINE_BP_SET) ""
  $(cache_sql_param airline string $(get_elem_id etAirline $(airline)))
  insert flt_no:$(flt_no)
         airp_dep:$(get_elem_id etAirp $(airp_dep))
         class:$(get_elem_id etClass $(class))
         bp_code:$(if $(eq $(bp_type) "") $(get_random_bp_type) $(bp_type))
 )
})


