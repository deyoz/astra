#ifndef __JXT_HANDLE_H__
#define __JXT_HANDLE_H__

#ifdef __cplusplus

#include <vector>
#include <string>

class HandleParams;

/*****************************************************************************/
/*****************     C++-�㭪樨 ��� ࠡ��� � ������ JXT     ***************/
/*****************************************************************************/

namespace JxtHandles
{
  void closeCurrentJxtHandle();
  /* ������ ���⥪�� ⥪�饣� ���� � ������ ⥪�騬 ��� த�⥫�᪮� ����.  */
  /* ����⥫�᪮� ���� ��।������ �� ���祭�� ���� PARENT_HANDLE �         */
  /* ���⥪�� ⥪�饣� ����.                                                */
  /* ����砭��: ����� ⥪�饣� ���� ��।������ ���祭��� ���� HANDLE       */
  /* ��⥬���� (� ����஬ 0) ���⥪��.                                     */

  void closeJxtHandleByNum(int handle);
  /* ������ ���⥪�� ���� � ����஬ handle � ������ ⥪�騬 ���             */
  /* த�⥫�᪮� ����.                                                      */

  int getFreeJxtHandle();
  /* �����頥� ����� ��ࢮ�� ������⮣� ����.                               */

  void createNewJxtHandle(int num);
  /* ������� ���⥪�� ��� ���� � ����஬ num � ������ ��� ⥪�騬.           */
  /* ����饥 ���� �⠭������ த�⥫�᪨� ����� ������ ����.                 */

  void createNewJxtHandle();
  /* ������� ���⥪�� ��� ������ ���� � ���� �������� �������� ����஬ � */
  /* ������ ��� ⥪�騬. ����饥 ���� �⠭������ த�⥫�᪨� ����� ������   */
  /* ����.                                                                   */

  void duplicateJxtHandle();
  /* ������� ���⥪�� ������ ���� � ��७��� � ���� �� ����� �� ���⥪�� */
  /* ⥪�饣� ����. ����� ���� �⠭������ ⥪�騬, ⥪�饥 - த�⥫�᪨�    */
  /* ������ ���� (����� ��ࠧ��, �����⢥���� ���祭��, �⫨��饥�� �       */
  /* ��ண� � ������ ���� - PARENT_HANDLE).                                 */

  int numberOfOpenJxtHandles();
  /* �����頥� ������⢮ ����, ������� �� ⥪�騬 ���⮬. ���⥬���    */
  /* (� ����஬ 0) ���� �� ���뢠����.                                      */

  int getJxtHandleNumByIface(const std::string &iface_id);
  /* �����頥� ����� ����, � ���஬ ����� ����䥩� � �����䨪��஬     */
  /* iface_id (�ਧ����� �㦨� ������ IFACE � ���⥪�� ����). �᫨ �᪮��� */
  /* ���� �� �������, �����頥� -1.                                         */

  void getJxtHandleByIface(const std::string &iface_id);
  /* ��� ����, � ���஬ ����� ����䥩� � �����䨪��஬ iface_id. �᫨  */
  /* ⠪�� ���� ����, ��� �⠭������ ⥪�騬. �᫨ ���, ᮧ���� ����� ����   */
  /* � ������ ��� ⥪�騬.                                                   */

  int findJxtHandleByParams(HandleParams *hp);
  /* �����頥� ����� ����, ����� � ���⥪�� ���ண� ᮮ⢥������        */
  /* ��ࠬ���� hp. �᫨ ⠪��� ���� �� �������, �����頥� -1.               */

  int getJxtHandleByParams(HandleParams *hp);
  /* ��� ����, ���⥪�� ���ண� ᮮ⢥����� ��ࠬ���� hp. �᫨ ⠪��     */
  /* ����, ������ ��� ⥪�騬 � �����頥� ��� �����. �᫨ ���, ᮧ����      */
  /* ����, ������ ��� ⥪�騬 � �����頥� 0.                                */

  int getCurrJxtHandle();
  /* �����頥� ����� ⥪�饣� ����.                                         */

} // namespace JxtHandles

/*****************************************************************************/
/*************     ���� C++-�㭪樨 ��� ࠡ��� � ������ JXT     ************/
/** �� �� �㭪樨 ��⠢���� ⮫쪮 ��� ᮢ���⨬��� � �������騬 ����� **/
/*****     �������� C++-�㭪�ﬨ �� ����࠭�⢠ ���� JxtHandles     *****/
/*****************************************************************************/

inline int getHandleByParams(HandleParams *hp)
{
  return JxtHandles::getJxtHandleByParams(hp);
}

inline int findHandleByParams(HandleParams *hp)
{
  int handle=JxtHandles::findJxtHandleByParams(hp);
  return handle<=0?0:handle;
}

inline int getHandleNumByIface(const char *iface)
{
  return JxtHandles::getJxtHandleNumByIface(iface?iface:"");
}

inline int getNumberOfHandles(const char *pult=0)
{
  return JxtHandles::numberOfOpenJxtHandles();
}
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮   */
/* ��� ᮢ���⨬��� � �������騬 �����.                                 */

#endif /* __cplusplus */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************/
/*****************      C-�㭪樨 ��� ࠡ��� � ������ JXT      ***************/
/** �� �� �㭪樨 ��⠢���� ⮫쪮 ��� ᮢ���⨬��� � �������騬 ����� **/
/*****     �������� C++-�㭪�ﬨ �� ����࠭�⢠ ���� JxtHandles     *****/
/*****************************************************************************/

void closeHandle(const char *pult);
/* ������ JxtHandles::closeCurrentJxtHandle()                                */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

int createHandleByNum(const char *pult, int num);
/* ������ JxtHandles::createNewJxtHandle(int num)                            */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

int _createNewHandle(const char *file, int line, const char *pult);
#define createNewHandle(x) _createNewHandle(__FILE__,__LINE__,(x))
/* ������ JxtHandles::createNewJxtHandle()                                   */
/* �����頥� 0.                                                             */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

void closeHandleByNum(const char *pult, int num);
/* ������ JxtHandles::closeJxtHandleByNum(int num)                           */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

int getFreeHandle(const char *pult);
/* ������ JxtHandles::getFreeJxtHandle()                                     */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

const char *getCurrHandle(const char *pult);
/* ������ JxtHandles::getCurrJxtHandle()                                     */
/* �����頥� 㪠��⥫� �� ���ᨢ char, ᮤ�ঠ騩 ������ ����� ⥪�饣�    */
/* ����, ������� '\0'.                                                      */
/* ����砭��: ���祭�� ��ࠬ��� pult �� �����, ��ࠬ��� ��⠢��� ⮫쪮     */
/* ��� ᮢ���⨬��� � �������騬 �����.                                   */

int DuplicateHandle();
/* ������ JxtHandles::duplicateJxtHandle()                                   */
/* �����頥� 0.                                                             */

int getHandleByIface(const char *iface);
/* ������ JxtHandles::getJxtHandleByIface()                                  */
/* �����頥� ����� ����������/ᮧ������� ����.                              */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class ContRec
{
  private:
    std::string name;
    std::string value;
  public:
    ContRec(const std::string &_name, const std::string &_val, int _len=-1);
    ContRec(const std::string &_name, int _val);
    std::string getName() const
    {
      return name;
    }
    std::string getValue() const
    {
      return value;
    }
};

class HandleParams
{
  private:
    std::vector<ContRec> data;
  public:
    HandleParams()
    {
      data.clear();
    }
    HandleParams(const std::string &iface)
    {
      data.clear();
      data.push_back(ContRec("IFACE",iface));
    }
    HandleParams &Add(const ContRec &cr)
    {
      data.push_back(cr);
      return *this;
    }
    HandleParams &operator <<(const ContRec &cr)
    {
      return Add(cr);
    }
    std::vector<ContRec> &getData()
    {
      return data;
    }
};

#endif /* __cplusplus */

#endif /* __JXT_HANDLE_H__ */
