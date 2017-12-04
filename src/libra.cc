#include "libra.h"
#include <serverlib/oci8.h>
#include <serverlib/oci8cursor.h>

#define NICKNAME "LIBRA"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"
#include "exceptions.h"
#include <xml_unit.h>
#include <serverlib/str_utils.h>

const int MAXBUFLEN = 1024*10;


void LibraInterface::Exec(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  tst();
/*  std::string data_base64 = NodeAsString( "data_base64", reqNode );
  std::string clob_in=StrUtils::b64_decode(data_base64), clob_out;*/
  std::string clob_in = NodeAsString( "data", reqNode );
  std::string clob_out;
  //clob_out.resize(200000);
  ProgTrace( TRACE5, "clob_in=%s", clob_in.c_str());
  OciCpp::Oci8Session* os =  instance();
  if ( os == NULL )  {
    throw EXCEPTIONS::Exception("invalid session got");
  }
   OciCpp::OciStatementHandle sth=os->getStHandle();
   std::string sql = "BEGIN SP_WB_ASTRA_CALLS(:clob_in,:clob_out); END;";
   //!!!std::string sql = "BEGIN SP_WB_ASTRA_TEST(:clob_in,:clob_out); END;";
   sword err;
   if(( err=OCIStmtPrepare(sth.stmthp, os->errhp, (const OraText*)sql.c_str(), strlen(sql.c_str()),
                           (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT) ))
   {
       os->checkerr(STDLOG,err);
       throw EXCEPTIONS::Exception("OCIStmtPrepare");
   }

   /*OCI8_OPEN(os, sth, sql.c_str());
   OCI8_BIND(os, sth, ":pult", pult.c_str(), pult.length()+1, SQLT_STR);
   OciDef(os, sth, defs);
   OCI8_EXEC(os, sth, 0);*/
   //OciCpp::Curs8Ctl cur1(STDLOG,sql,instance(),true/*no_cache*/);

   OCILobLocator* cl_in  = 0;
   if((err=OCIDescriptorAlloc(os->envhp,(dvoid **)&cl_in, OCI_DTYPE_LOB, 0, 0) )) {
       os->checkerr(STDLOG,err);
       throw EXCEPTIONS::Exception("OCIDescriptorAlloc");
   }
   if (err = OCILobCreateTemporary(os->svchp, os->errhp, cl_in,  (ub2)0,
   SQLCS_IMPLICIT,
   OCI_TEMP_BLOB,
   OCI_ATTR_NOCACHE,
   OCI_DURATION_SESSION))
   {
       if(os->checkerr(STDLOG,err))
        throw EXCEPTIONS::Exception("OCILobCreateTemporary");
   }
   ub4 amt = clob_in.length();
   ProgTrace( TRACE5, "amt=%d, !clob_in.empty()=%d", amt, !clob_in.empty() );
   if (!clob_in.empty() && (err= OCILobWrite(os->svchp, os->errhp, cl_in, &amt, 1,
                                (void*)clob_in.data(), clob_in.length(), OCI_ONE_PIECE, 0, 0, 0, SQLCS_IMPLICIT))) {
       if(os->checkerr(STDLOG,err))
        throw EXCEPTIONS::Exception("OCILobWrite");

     }
ProgTrace(TRACE5, "OCILobWrite amt=%d, clob_in.length()=%zu", amt, clob_in.length() );
   OCIBind* bin_in=NULL;
   if((err=OCIBindByName(sth.stmthp, &bin_in, os->errhp, (const text*)":clob_in", -1,
                                    (dvoid*)&cl_in, (sb4)-1, SQLT_CLOB, 0, 0, 0, 0, 0, OCI_DEFAULT)) ) {
     os->checkerr(STDLOG,err);
      throw EXCEPTIONS::Exception("OCIBindByName clob_in");
   }


   OCILobLocator* cl_out  = 0;
   if((err=OCIDescriptorAlloc(os->envhp,(dvoid **)&cl_out, OCI_DTYPE_LOB, 0, 0) )) {
       os->checkerr(STDLOG,err);
       throw EXCEPTIONS::Exception("OCIDescriptorAlloc");
   }
   if (err = OCILobCreateTemporary(os->svchp, os->errhp, cl_out,  (ub2)0,
   SQLCS_IMPLICIT,
   OCI_TEMP_BLOB,
   OCI_ATTR_NOCACHE,
   OCI_DURATION_SESSION))
   {
       if(os->checkerr(STDLOG,err))
        throw EXCEPTIONS::Exception("OCILobCreateTemporary out");
   }
   OCIBind* bin_out=NULL;
   if((err=OCIBindByName(sth.stmthp, &bin_out, os->errhp, (const text*)":clob_out", -1,
                                   (dvoid*)&cl_out, (sb4)-1, SQLT_CLOB, 0, 0, 0, 0, 0, OCI_DEFAULT)) ) {
       os->checkerr(STDLOG,err);
     throw EXCEPTIONS::Exception("OCIBindByName clob_out");
  }

   if ((err = OCIStmtExecute(os->svchp, sth.stmthp, os->errhp, 1, 0, 0, 0,
                             OCI_DEFAULT)))
   {
       os->checkerr(STDLOG, err);
        throw EXCEPTIONS::Exception("OCIStmtExecute");
   }
   ub1 bufp[MAXBUFLEN];

   /* Setting amt = 0 will read till the end of LOB*/
   amt = 0;
   ub4 buflen = sizeof(bufp);

  /* Process the data in pieces */
   ub4 offset = 1;
   memset(bufp, '\0', MAXBUFLEN);
   boolean done = FALSE;
   while (!done)
   {
     err = OCILobRead(os->svchp, os->errhp, cl_out, &amt, offset, (dvoid *) bufp,
                         buflen, (dvoid *)0,
                         nullptr,
                         (ub2) 0, (ub1) SQLCS_IMPLICIT);
     switch (err)
     {
     case OCI_SUCCESS:           /* Only one piece or last piece*/
       /* Process the data in bufp. amt will give the amount of data just read in
          bufp. This is in bytes for BLOBs and in characters for fixed
          width CLOBS and in bytes for variable width CLOBs
        */
         clob_out += std::string((char*)bufp, amt );
         ProgTrace(TRACE5, "done clob=%s, amt=%d", clob_out.c_str(), amt );
       done = TRUE;
       break;
     case OCI_ERROR:
       os->checkerr(STDLOG, err);
       done = TRUE;
       break;
     case OCI_NEED_DATA:         /* There are 2 or more pieces */
       /* Process the data in bufp. amt will give the amount of data just read in
          bufp. This is in bytes for BLOBs and in characters for fixed
          width CLOBS and in bytes for variable width CLOBs
        */
         clob_out += std::string((char*)bufp, amt );
         ProgTrace(TRACE5, "need clob=%s, amt=%d", clob_out.c_str(), amt );
       break;
     default:
       os->checkerr(STDLOG, err);
       done = TRUE;
       break;
     }
   } /* while */
   /* Free resources held by the locators*/
    OCIDescriptorFree((dvoid *) cl_in, (ub4) OCI_DTYPE_LOB);
    OCIDescriptorFree((dvoid *) cl_out, (ub4) OCI_DTYPE_LOB);
   //cur1.bindClob(":clob_in", clob_in);
   //cur1.bindOut(":clob_out", &clob_out);
   //cur1.bindClob(":clob_out", clob_out);
/*   OCILobLocator *ClobDataLocator;
   int err = OCIDescriptorAlloc((dvoid *)instance()->envhp,(dvoid **)&ClobDataLocator,(ub4)OCI_DTYPE_LOB, (size_t) 0, (dvoid **) 0);
   if ( err ) {
       throw EXCEPTIONS::Exception("OCIDescriptorAlloc");
   }
   OCIBind* bin=NULL;
   OCIStmt* stmt = instance()->getStHandle().stmthp;
   if(err=OCIBindByName( stmt, &bin, instance()->errhp, (const text*)":clob_out", -1,
                              (dvoid*)&ClobDataLocator, (sb4)-1, SQLT_BLOB, 0, 0, 0, 0, 0, (ub4) OCI_DEFAULT) ) {
       throw EXCEPTIONS::Exception("OCIBindByName %d %d %d", err, instance()->errhp, instance()->err() );
   }*/
//   OCIDefine *defnp1;
//   sword err = OCIDefineByPos(instance()->getStHandle(),&defnp1,instance()->errhp,(ub4) 1,
//                              (dvoid *)&RawDataLocator,  0,
//                              (ub2) SQLT_BLOB, (dvoid *) &RawDataInd,
//                              (ub2 *) 0, (ub2 *) 0, (ub4) OCI_DEFAULT);
//   cur1.bindClob(":clob_out", clob_out );
//   bindlob(name, createLob(os, data, OCI_TEMP_BLOB), SQLT_BLOB);
   //cur1.defClob( clob_out );
  // cur1.exec();
   //ProgTrace(TRACE4, "clob_in=%s, RawDataInd=%d", clob_in.c_str(), RawDataInd);
   ProgTrace(TRACE4, "clob_in=%s, clob_out=%s", clob_in.c_str(), clob_out.c_str());
   NewTextChild( resNode, "data", clob_out );
}
