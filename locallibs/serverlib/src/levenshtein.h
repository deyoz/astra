#ifndef __LEVENSHTEIN_H
#define __LEVENSHTEIN_H

#include <string>

/* �����ﭨ� ������⥩�� �� a �� b */
/* � ��� �-� ����� ��।����� �-� �ࠢ����� ᨬ�����. *
 * �� 㤮���, � ��⭮��, ��� ���஢���� ���      *
 * � ����⢥ ��ࠧ� ����� ����⠢��� 'x' ������騩 *
 * '�� ᨬ���'                                     */
unsigned LevenshteinDistance(
        const std::string& a,
        const std::string& b,
        bool (*isEqual)(char a, char b) = NULL);
        
/* �����ﭨ� ������-������⥩�� �� a �� b                */
/* (�� ��� � ������⥩��, �� �������� ���⠬� ��� �㪢� - *
 * �� ���� ��ࠢ�����, � �� ���)                         */
unsigned DamerauLevenshteinDistance(
        const std::string& a,
        const std::string& b);

/* �㭪�� �������筠 �।��饩, �� ����᪠�� �� ����� max_distance ��ࠢ����� ����� ��ப��� */
/* �᫨ ��ࠢ���� ����� ��ப��� �����, � �����頥��� max_distance+1                        */
/************************************************************************************************/
/* ������ ࠡ���� ����॥                                                                      */
unsigned DamerauLevenshteinDistance_lim(
        const std::string& a,
        const std::string& b,
        const unsigned max_distance);

#endif /* #ifndef __LEVENSHTEIN_H */
