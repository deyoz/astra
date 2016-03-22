#include "wsdl_services.h"
using namespace std;

string azimut_login_test_show(adb_LoginRequest* request,axutil_env_t  *env)
{  string ret;
  
   if(!env) return "void return of adb_LoginRequest_create_with_values()";
   const char* property = adb_LoginRequest_get_property1(request, env);
   if(property) {ret+="property_Type: "; ret+=property;}
   return ret;
}



std::string azimut_login()
{
axutil_allocator_t *allocator = NULL;

allocator = axutil_allocator_init(NULL);

string s =  SendAzimutRequestToSirena();
std::cout<<s<<"\n";
return s;

//We pass NULL to the above function in order to use the default allocator functions.

/*axutil_log_t *log = NULL;
axutil_error_t *error = NULL;
log = axutil_log_create(allocator, NULL, NULL);
log = axutil_log_create(allocator, NULL, "mylog.log");
axutil_env_t *env  = axutil_env_create_with_error_log(allocator, error, log);
char name[] = "SirenaWeb";
char pass[] = "!QAZ2wsx";

adb_LoginRequest* x =  adb_LoginRequest_create_with_values(env, name, pass);
return azimut_login_test_show(x, env);*/
}


int azimut_login(int argc,char **argv)
{
    azimut_login();
    return 0;
}

