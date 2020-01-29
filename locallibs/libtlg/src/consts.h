#ifndef LIBTLG_CONSTS_H
#define LIBTLG_CONSTS_H

#ifdef IN
#error IN already defined
#endif

#define MAX_TLG_SIZE 10240 // ���ᨬ���� ࠧ��� ⫣ � UDP-ᮮ�饭��
#define MAX_TLG_LEN 900000 // ���ᨬ���� ࠧ��� ⥫��ࠬ� � C-��������, � C++ �ᯮ������ string

/**
 * @brief �ࠢ����� ��।�� ⥫��ࠬ
 * ���祭�� ���� sys_q �� air_q 
 * */
typedef enum Direction
{
    IN = 1,                         ///< ������ ��� ��ࠡ�⪨ ��� ��ࠢ��
    TYPEA_OUT = 2,                  ///< �������� � ��室��� (Edifact �� ��ࠢ��)
    REPEAT,                         ///< ��।� ⥫��ࠬ� �� ����� ��ࠡ���
    OTHER_OUT,                      ///< �������� � ��室��� (�� Edifact �� ��ࠢ��)
    WAIT_DELIV,                     ///< ������� ���⢥ত���� �� �����
    INCOMING = 6,                   ///< �������� �� �室��� �� ��ࠡ���
    DEL_ = 7,                       ///< 㤠���� ⥫��ࠬ��
    LONG_PART,                      ///< ��।� ���窮� ������� ⥫��ࠬ�
    DISPATCH                        ///< ��।� ⫣ ��� ��ᯥ��ਧ�樨
}Direction;



#endif /* LIBTLG_CONSTS_H */

