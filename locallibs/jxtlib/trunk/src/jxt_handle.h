#ifndef __JXT_HANDLE_H__
#define __JXT_HANDLE_H__

#ifdef __cplusplus

#include <vector>
#include <string>

class HandleParams;

/*****************************************************************************/
/*****************     C++-функции для работы с окнами JXT     ***************/
/*****************************************************************************/

namespace JxtHandles
{
  void closeCurrentJxtHandle();
  /* Удаляет контекст текущего окна и делает текущим его родительское окно.  */
  /* Родительское окно определяется по значению поля PARENT_HANDLE в         */
  /* контексте текущего окна.                                                */
  /* Замечание: номер текущего окна определяется значением поля HANDLE       */
  /* системного (с номером 0) контекста.                                     */

  void closeJxtHandleByNum(int handle);
  /* Удаляет контекст окна с номером handle и делает текущим его             */
  /* родительское окно.                                                      */

  int getFreeJxtHandle();
  /* Возвращает номер первого незанятого окна.                               */

  void createNewJxtHandle(int num);
  /* Создает контекст для окна с номером num и делает его текущим.           */
  /* Текущее окно становится родительским окном нового окна.                 */

  void createNewJxtHandle();
  /* Создает контекст для нового окна с первым найденным незанятым номером и */
  /* делает его текущим. Текущее окно становится родительским окном нового   */
  /* окна.                                                                   */

  void duplicateJxtHandle();
  /* Создает контекст нового окна и переносит в него все данные из контекста */
  /* текущего окна. Новое окно становится текущим, текущее - родительским    */
  /* нового окна (Таким образом, единственное значение, отличающееся у       */
  /* старого и нового окна - PARENT_HANDLE).                                 */

  int numberOfOpenJxtHandles();
  /* Возвращает количество окон, числящихся за текущим пультом. Системное    */
  /* (с номером 0) окно не учитывается.                                      */

  int getJxtHandleNumByIface(const std::string &iface_id);
  /* Возвращает номер окна, в котором открыт интерфейс с идентификатором     */
  /* iface_id (признаком служит запись IFACE в контексте окна). Если искомое */
  /* окно не найдено, возвращает -1.                                         */

  void getJxtHandleByIface(const std::string &iface_id);
  /* Ищет окно, в котором открыт интерфейс с идентификатором iface_id. Если  */
  /* такое окно есть, оно становится текущим. Если нет, создает новое окно   */
  /* и делает его текущим.                                                   */

  int findJxtHandleByParams(HandleParams *hp);
  /* Возвращает номер окна, записи в контексте которого соответствуют        */
  /* параметру hp. Если такого окна не найдено, возвращает -1.               */

  int getJxtHandleByParams(HandleParams *hp);
  /* Ищет окно, контекст которого соответствует параметру hp. Если такое     */
  /* есть, делает его текущим и возвращает его номер. Если нет, создает      */
  /* окно, делает его текущим и возвращает 0.                                */

  int getCurrJxtHandle();
  /* Возвращает номер текущего окна.                                         */

} // namespace JxtHandles

/*****************************************************************************/
/*************     Старые C++-функции для работы с окнами JXT     ************/
/** Все эти функции оставлены только для совместимости с существующим кодом **/
/*****     Пользуйтесь C++-функциями из пространства имен JxtHandles     *****/
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
/* Замечание: значение параметра pult не важно, параметр оставлен только   */
/* для совместимости с существующим кодом.                                 */

#endif /* __cplusplus */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************/
/*****************      C-функции для работы с окнами JXT      ***************/
/** Все эти функции оставлены только для совместимости с существующим кодом **/
/*****     Пользуйтесь C++-функциями из пространства имен JxtHandles     *****/
/*****************************************************************************/

void closeHandle(const char *pult);
/* Аналог JxtHandles::closeCurrentJxtHandle()                                */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

int createHandleByNum(const char *pult, int num);
/* Аналог JxtHandles::createNewJxtHandle(int num)                            */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

int _createNewHandle(const char *file, int line, const char *pult);
#define createNewHandle(x) _createNewHandle(__FILE__,__LINE__,(x))
/* Аналог JxtHandles::createNewJxtHandle()                                   */
/* Возвращает 0.                                                             */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

void closeHandleByNum(const char *pult, int num);
/* Аналог JxtHandles::closeJxtHandleByNum(int num)                           */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

int getFreeHandle(const char *pult);
/* Аналог JxtHandles::getFreeJxtHandle()                                     */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

const char *getCurrHandle(const char *pult);
/* Аналог JxtHandles::getCurrJxtHandle()                                     */
/* Возвращает указатель на массив char, содержащий запись номера текущего    */
/* окна, закрытый '\0'.                                                      */
/* Замечание: значение параметра pult не важно, параметр оставлен только     */
/* для совместимости с существующим кодом.                                   */

int DuplicateHandle();
/* Аналог JxtHandles::duplicateJxtHandle()                                   */
/* Возвращает 0.                                                             */

int getHandleByIface(const char *iface);
/* Аналог JxtHandles::getJxtHandleByIface()                                  */
/* Возвращает номер найденного/созданного окна.                              */

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
