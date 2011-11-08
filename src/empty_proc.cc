//---------------------------------------------------------------------------
#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "arx_daily.h"
#include "tlg/tlg_parser.h"
#include "tclmon/tcl_utils.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

const int sleepsec = 25;


int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("log1");


    for( ;; )
    {
      sleep( sleepsec );
      InitLogTime(NULL);
      ProgTrace( TRACE0, "empty_proc: Next iteration");
    };
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

#include "oralib.h"
#include "basic.h"
#include "astra_utils.h"

using namespace ASTRA;
using namespace BASIC;
using namespace std;

void alter_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5)
{
  static time_t start_time=time(NULL);
  if (time(NULL)-start_time>=work_secs)
  {
    if (commit_before_sleep) OraSession.Commit();
    printf("%d iterations processed. sleep...", processed);
    fflush(stdout);
    sleep(sleep_secs);
    printf("go!\n");
    start_time=time(NULL);
  };
};

int alter_db(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(point_id) AS min_point_id FROM crs_data_stat";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_point_id")) return 0;
  int min_point_id=Qry.FieldAsInteger("min_point_id");

  Qry.SQLText="SELECT MAX(point_id) AS max_point_id FROM crs_data_stat";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_point_id")) return 0;
  int max_point_id=Qry.FieldAsInteger("max_point_id");
  
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "   UPDATE crs_data SET system='CRS' WHERE point_id>=:low_point_id AND point_id<:high_point_id; "
    "   :processed:=:processed+SQL%ROWCOUNT; "
    "   UPDATE crs_pnr SET system='CRS' WHERE point_id>=:low_point_id AND point_id<:high_point_id; "
    "   :processed:=:processed+SQL%ROWCOUNT; "
    "END;";
  Qry.CreateVariable("processed", otInteger, (int)0);
  Qry.DeclareVariable("low_point_id", otInteger);
  Qry.DeclareVariable("high_point_id", otInteger);
    
  
  int iter=0;
  for(int curr_point_id=min_point_id; curr_point_id<=max_point_id; curr_point_id+=100, iter++)
  {
    alter_wait(iter,true,5,10);
    Qry.SetVariable("low_point_id", curr_point_id);
    Qry.SetVariable("high_point_id", curr_point_id+100);
    Qry.Execute();
    if (Qry.GetVariableAsInteger("processed")>=10000)
    {
      OraSession.Commit();
      Qry.SetVariable("processed", (int)0);
    }
  };
  OraSession.Commit();

  puts("Database altered successfully");
  return 0;
};

bool alter_arx(void)
{
 /* static time_t prior_exec=0;
  
  if (time(NULL)-prior_exec<ARX_SLEEP()) return false;

  time_t time_finish=time(NULL)+ARX_DURATION();
  
  prior_exec=time(NULL);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="UPDATE tasks SET pr_denial=1 WHERE name='alter_arx'";
  Qry.Execute();*/
  return true;
};

int alter_pax_doc(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");
  
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
  Qry.Execute();
  int tid=Qry.FieldAsInteger("tid");
  
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_pax_doc(low_pax_id IN pax.pax_id%TYPE, high_pax_id IN pax.pax_id%TYPE, "
    "                                          vtid pax.tid%TYPE, processed IN OUT BINARY_INTEGER) "
    "IS "
    "  CURSOR cur(vlow_pax_id IN pax.pax_id%TYPE, vhigh_pax_id IN pax.pax_id%TYPE) IS "
    "    SELECT pax.pax_id,drop_document AS document "
    "    FROM pax, pax_doc "
    "    WHERE pax.pax_id=pax_doc.pax_id(+) AND "
    "          pax.pax_id>=vlow_pax_id AND pax.pax_id<vhigh_pax_id AND "
    "          drop_document IS NOT NULL AND (pax_doc.no IS NULL OR pax_doc.no<>drop_document) FOR UPDATE; "
    "  vtype              pax_doc.type%TYPE; "
    "  vissue_country     pax_doc.issue_country%TYPE; "
    "  vno                pax_doc.no%TYPE; "
    "  i                  BINARY_INTEGER; "
    "  CURSOR cur1(vpax_id IN pax.pax_id%TYPE, vdocument IN VARCHAR2) IS "
    "    SELECT type,issue_country,no,nationality, "
    "           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi "
    "    FROM crs_pax_doc "
    "    WHERE pax_id=vpax_id AND no=vdocument "
    "    ORDER BY DECODE(type,'P',0,NULL,2,1), DECODE(rem_code,'DOCS',0,1), no; "
    "  row1 cur1%ROWTYPE; "
    "BEGIN "
    "  i:=0; "
    "  FOR curRow IN cur(low_pax_id, high_pax_id) LOOP "
    "    IF normalize_pax_doc(curRow.document, vtype, vissue_country, vno) THEN "
    "      DELETE FROM pax_doc WHERE pax_id=curRow.pax_id; "
    "      OPEN cur1(curRow.pax_id, vno); "
    "      FETCH cur1 INTO row1; "
    "      IF cur1%FOUND THEN "
    "        INSERT INTO pax_doc "
    "          (pax_id,type,issue_country,no,nationality, "
    "           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "        VALUES "
    "          (curRow.pax_id,row1.type,row1.issue_country,row1.no,row1.nationality, "
    "           row1.birth_date,row1.gender,row1.expiry_date,row1.surname,row1.first_name,row1.second_name,row1.pr_multi); "
    "      ELSE "
    "        INSERT INTO pax_doc(pax_id, type, issue_country, no, pr_multi) "
    "        VALUES(curRow.pax_id, vtype, vissue_country, vno, 0); "
    "      END IF; "
    "      CLOSE cur1; "
    "      UPDATE pax SET tid=vtid WHERE pax_id=curRow.pax_id; "
    "      i:=i+2; "
    "    ELSE "
    "      INSERT INTO drop_alter_doc_errors(part_key, pax_id, document) "
    "      VALUES(NULL, curRow.pax_id, curRow.document ); "
    "      i:=i+1; "
    "    END IF; "
    "    IF i>=10000 THEN "
    "      COMMIT; "
    "      i:=0; "
    "    END IF; "
    "    processed:=processed+1; "
    "  END LOOP; "
    "  IF i>0 THEN "
    "    COMMIT; "
    "  END IF; "
    "END;";
  Qry.Execute();
  
  Qry.SQLText=
    "BEGIN "
    "  alter_pax_doc(:low_pax_id, :high_pax_id, :tid, :processed); "
    "END; ";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.CreateVariable("processed", otInteger, (int)0);
  

  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    alter_wait(Qry.GetVariableAsInteger("processed"));
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
  };
  
  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_pax_doc";
  Qry.Execute();

  return 0;
};
  
int alter_pax_doc2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
  Qry.Execute();
  int tid=Qry.FieldAsInteger("tid");
  
  TypeB::TTlgParser tlg;
  TypeB::TDocItem doc;
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
    "  INSERT INTO pax_doc "
    "    (pax_id,type,issue_country,no,nationality, "
    "     birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "  VALUES "
    "    (:pax_id,:type,:issue_country,:no,:nationality, "
    "     :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi); "
    "  UPDATE pax SET tid=:tid WHERE pax_id=:pax_id; "
    "  UPDATE drop_alter_doc_errors SET processed=1 WHERE pax_id=:pax_id AND part_key IS NULL; "
    "END;";
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("issue_country",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("nationality",otString);
  Qry.DeclareVariable("birth_date",otDate);
  Qry.DeclareVariable("gender",otString);
  Qry.DeclareVariable("expiry_date",otDate);
  Qry.DeclareVariable("surname",otString);
  Qry.DeclareVariable("first_name",otString);
  Qry.DeclareVariable("second_name",otString);
  Qry.DeclareVariable("pr_multi",otInteger);
  Qry.CreateVariable("tid", otInteger, tid);
  
  TQuery DocQry(&OraSession);
  DocQry.Clear();
  DocQry.SQLText=
    "SELECT pax.pax_id, pax.drop_document AS document FROM drop_alter_doc_errors, pax "
    "WHERE pax.pax_id=drop_alter_doc_errors.pax_id AND "
    "      (drop_alter_doc_errors.document like '_/__%' OR drop_alter_doc_errors.document like 'DOCS/_/__%') AND "
    "      drop_alter_doc_errors.part_key IS NULL "
    "FOR UPDATE";
  DocQry.Execute();

  for(;!DocQry.Eof;DocQry.Next())
  {
    string rem_text=DocQry.FieldAsString("document");
    if (rem_text.substr(0,5)=="DOCS/") rem_text.erase(0,5);
    rem_text="DOCS HK1/"+rem_text;
    
    if (!ParseDOCSRem(tlg,rem_text,doc)) continue;
    if (doc.Empty()) continue;
    if (*doc.no==0) continue;
      
    Qry.SetVariable("pax_id",DocQry.FieldAsInteger("pax_id"));
    Qry.SetVariable("type",doc.type);
    Qry.SetVariable("issue_country",doc.issue_country);
    Qry.SetVariable("no",doc.no);
    Qry.SetVariable("nationality",doc.nationality);
    if (doc.birth_date!=NoExists)
      Qry.SetVariable("birth_date",doc.birth_date);
    else
      Qry.SetVariable("birth_date",FNull);
    Qry.SetVariable("gender",doc.gender);
    if (doc.expiry_date!=NoExists)
      Qry.SetVariable("expiry_date",doc.expiry_date);
    else
      Qry.SetVariable("expiry_date",FNull);
    if (doc.surname.size()>64) doc.surname.erase(64);
    Qry.SetVariable("surname",doc.surname);
    if (doc.first_name.size()>64) doc.first_name.erase(64);
    Qry.SetVariable("first_name",doc.first_name);
    if (doc.second_name.size()>64) doc.second_name.erase(64);
    Qry.SetVariable("second_name",doc.second_name);
    Qry.SetVariable("pr_multi",(int)doc.pr_multi);
    Qry.Execute();
  };
  OraSession.Commit();
  
  return 0;
};

int alter_arx_pax_doc(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");
  
  
  //Qry.SQLText="CREATE TABLE arx_doc_stat(no VARCHAR2(50) NOT NULL, num NUMBER NOT NULL)";
  //Qry.Execute();
  
  //Qry.SQLText="CREATE UNIQUE INDEX arx_doc_stat__IDX ON arx_doc_stat(no)";
  //Qry.Execute();
  /*
  Qry.Clear();
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE get_arx_doc_stat(low_part_key IN DATE, high_part_key IN DATE) "
    "IS "
    "  CURSOR cur(vlow_part_key IN DATE, vhigh_part_key IN DATE) IS "
    "    SELECT TRANSLATE(TRIM(document),'0123456789', '**********') AS no "
    "    FROM arx_pax WHERE part_key>=vlow_part_key AND part_key<vhigh_part_key AND document IS NOT NULL; "
    "BEGIN "
    "  FOR curRow IN cur(low_part_key, high_part_key) LOOP "
    "    IF curRow.no IS NOT NULL THEN "
    "      UPDATE arx_doc_stat SET num=num+1 WHERE no=curRow.no; "
    "      IF SQL%ROWCOUNT=0 THEN "
    "        INSERT INTO arx_doc_stat(no, num) VALUES(curRow.no, 1); "
    "      END IF; "
    "    END IF; "
    "  END LOOP; "
    "  COMMIT; "
    "END; ";
  Qry.Execute();*/
  
  /*
  CREATE TABLE drop_alter_doc_errors
  (
    part_key DATE,
    pax_id   NUMBER(9) NOT NULL,
    document VARCHAR2(50) NOT NULL,
    processed NUMBER(1)
  );
  CREATE INDEX drop_alter_doc_errors__IDX on drop_alter_doc_errors(pax_id);
  */

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_arx_pax_doc(low_part_key IN DATE, high_part_key IN DATE) "
    "IS "
    "  CURSOR cur(vlow_part_key IN DATE, vhigh_part_key IN DATE) IS "
    "    SELECT part_key,pax_id,drop_document AS document "
    "    FROM arx_pax WHERE part_key>=vlow_part_key AND part_key<vhigh_part_key AND drop_document IS NOT NULL; "
    "  vtype              pax_doc.type%TYPE; "
    "  vissue_country     pax_doc.issue_country%TYPE; "
    "  vno                pax_doc.no%TYPE; "
    "  i                  BINARY_INTEGER; "
    "BEGIN "
    "  i:=0; "
    "  FOR curRow IN cur(low_part_key, high_part_key) LOOP "
    "    IF normalize_pax_doc(curRow.document, vtype, vissue_country, vno) THEN  "
    "      INSERT INTO arx_pax_doc(pax_id, type, issue_country, no, pr_multi, part_key) "
    "      VALUES(curRow.pax_id, vtype, vissue_country, vno, 0, curRow.part_key); "
    "    ELSE "
    "      INSERT INTO drop_alter_doc_errors(part_key, pax_id, document) "
    "      VALUES(curRow.part_key, curRow.pax_id, curRow.document ); "
    "    END IF; "
    "    i:=i+1; "
    "    IF i>=10000 THEN "
    "      COMMIT; "
    "      i:=0; "
    "    END IF; "
    "  END LOOP; "
    "  COMMIT; "
    "END;";
  Qry.Execute();
    
  Qry.SQLText=
    "BEGIN "
    "  alter_arx_pax_doc(:low_part_key, :high_part_key); "
    "END; ";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);
  
  int processed=0;

  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=1.0, processed++)
  {
    alter_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+1.0);
    Qry.Execute();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_arx_pax_doc";
  Qry.Execute();
  
  //Qry.SQLText="DROP TABLE arx_doc_stat";
  //Qry.Execute();

  return 0;
};

int alter_arx_pax_doc2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  TypeB::TTlgParser tlg;
  TypeB::TDocItem doc;
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM arx_pax_doc WHERE part_key=:part_key AND pax_id=:pax_id; "
    "  INSERT INTO arx_pax_doc "
    "    (pax_id,type,issue_country,no,nationality, "
    "     birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi,part_key) "
    "  VALUES "
    "    (:pax_id,:type,:issue_country,:no,:nationality, "
    "     :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi,:part_key); "
    "  UPDATE drop_alter_doc_errors SET processed=1 WHERE pax_id=:pax_id AND part_key=:part_key; "
    "END;";
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("issue_country",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("nationality",otString);
  Qry.DeclareVariable("birth_date",otDate);
  Qry.DeclareVariable("gender",otString);
  Qry.DeclareVariable("expiry_date",otDate);
  Qry.DeclareVariable("surname",otString);
  Qry.DeclareVariable("first_name",otString);
  Qry.DeclareVariable("second_name",otString);
  Qry.DeclareVariable("pr_multi",otInteger);
  Qry.DeclareVariable("part_key",otDate);

  TQuery DocQry(&OraSession);
  DocQry.Clear();
  DocQry.SQLText=
    "SELECT arx_pax.part_key, arx_pax.pax_id, arx_pax.drop_document AS document "
    "FROM drop_alter_doc_errors, arx_pax "
    "WHERE arx_pax.part_key=drop_alter_doc_errors.part_key AND "
    "      arx_pax.pax_id=drop_alter_doc_errors.pax_id AND "
    "      (drop_alter_doc_errors.document like '_/__%' OR drop_alter_doc_errors.document like 'DOCS/_/__%') AND "
    "      drop_alter_doc_errors.part_key IS NOT NULL ";
  DocQry.Execute();

  int processed=0;

  for(;!DocQry.Eof;DocQry.Next())
  {
    alter_wait(processed);
  
    string rem_text=DocQry.FieldAsString("document");
    if (rem_text.substr(0,5)=="DOCS/") rem_text.erase(0,5);
    rem_text="DOCS HK1/"+rem_text;

    if (!TypeB::ParseDOCSRem(tlg,rem_text,doc)) continue;
    if (doc.Empty()) continue;
    if (*doc.no==0) continue;

    Qry.SetVariable("pax_id",DocQry.FieldAsInteger("pax_id"));
    Qry.SetVariable("part_key",DocQry.FieldAsDateTime("part_key"));
    Qry.SetVariable("type",doc.type);
    Qry.SetVariable("issue_country",doc.issue_country);
    Qry.SetVariable("no",doc.no);
    Qry.SetVariable("nationality",doc.nationality);
    if (doc.birth_date!=NoExists)
      Qry.SetVariable("birth_date",doc.birth_date);
    else
      Qry.SetVariable("birth_date",FNull);
    Qry.SetVariable("gender",doc.gender);
    if (doc.expiry_date!=NoExists)
      Qry.SetVariable("expiry_date",doc.expiry_date);
    else
      Qry.SetVariable("expiry_date",FNull);
    if (doc.surname.size()>64) doc.surname.erase(64);
    Qry.SetVariable("surname",doc.surname);
    if (doc.first_name.size()>64) doc.first_name.erase(64);
    Qry.SetVariable("first_name",doc.first_name);
    if (doc.second_name.size()>64) doc.second_name.erase(64);
    Qry.SetVariable("second_name",doc.second_name);
    Qry.SetVariable("pr_multi",(int)doc.pr_multi);
    Qry.Execute();
    processed++;
  };
  OraSession.Commit();

  return 0;
};

