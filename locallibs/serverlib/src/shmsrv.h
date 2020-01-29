/*
 *  completly new UNIX SOCKET version
 */

#ifndef _SHMSRV_H
#define _SHMSRV_H

#ifndef _OBJECT_H
#include "object.h"
#endif

#define ADD_ZONE  1
#define DEL_ZONE  2
#define UPD_ZONE  3
#define GET_ZONE  4
#define ADU_ZONE  5
#define GET_LIST  6
#define DROP_LIST 7
#define EXIT_PULT 8

#define SERVER_ERROR        1
#define ORACLE_ERROR        2
#define REQUEST_ERROR       3
#define INVALID_ZONE_NUMBER 4
#define ZONE_WAS_DELETED    5

#define SHMA_CHNGD  1   /*area was changed and must be commited*/
#define SHMA_RDONLY 2   /*area is readonly*/
#define SHMA_NEW    4   /*area is new - server must create it*/
#define SHMA_DEL    8   /*area was deleted*/
#define SHMA_DEAD   16  /*corrupted */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WITHZONES

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Zone {
   Item I;
   void *ptr;
   size_t len;
   int flag;
   char title[35];
   int id;
} Zone;
classtype_(Zone);

void pr_digest(const unsigned char *p,char *out);
void printZoneList();

/*user functions*/

/*
set new zone date 
n - zone number
len - new length of data
data - pointer to new data
head - new zone description (must be 0 termitated or NULL if unchanged)
*/
int UpdateZone(int n, size_t len, const void* data, const char *head);

/*
create new zone
n - zone number
len - length of zone
head - description
*/
pZone get_zone(int zone, const char*who);
pZone NewZone(int n, size_t len, const char* head);
int SaveZoneList(void);
void RestoreZoneList(void);
void InitZoneList(const char *pu);
void DropZoneList(void);
void UpdateZoneList(const char *pu);
int EndShmOP(void);

int DropAllAreas (const char*who);
//int DropPult (const char*who);

/*
  write  data to zone identified by pp
  data - pointer to data
  start - strting offset
  len - length of data
  returns 0 success if success
 */
int setZoneData(pZone p,const void *data, size_t start, int len);

int setZoneData2(pZone p,const void *data, size_t elemsize, int start, int nelem);
/*
  read  data from zone identified by pp
  data - pointer to data
  start - strting offset
  len - length of data
  returns 0 success if success
 */
int getZoneData(const pZone p, void *data, size_t start, size_t len);

int getZoneData2(const pZone p, void *data, size_t elemsize, size_t start, size_t nelem);
/*
  get zone descriptor by zone number
*/
pZone getZonePtr(int n);

/*
delete zone number n
*/
int DelZone (int n);
/*
resize zone identified by descriptor pp
new size is set to len
old data retained
returns 0 if success
 */
int ReallocZone(pZone pp, size_t len);

#ifdef __cplusplus
}
#endif


#endif /*_SHMSRV_H*/

