#ifndef __SIRENAPROC_H__
#define __SIRENAPROC_H__

#ifdef __cplusplus
void testInitDB();
void testShutDBConnection();

namespace PgCpp {
    namespace details {
        class SessionDescription;
    }
}

#ifdef ENABLE_PG
void setupPgManagedSession();
void setupPgReadOnlySession();
PgCpp::details::SessionDescription *getPgSessionDescriptor();
#endif // ENABLE_PG

extern "C"
{
#endif /* __cplusplus */

int connect2DB();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SIRENAPROC_H__ */
