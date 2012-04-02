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
  
  Qry.SQLText="SELECT cycle_tid__seq.nextval AS tid FROM dual";
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
  Qry.SQLText="SELECT cycle_tid__seq.nextval AS tid FROM dual";
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
    
    if (!ParseDOCSRem(tlg,NoExists,rem_text,doc)) continue;
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

    if (!TypeB::ParseDOCSRem(tlg,NoExists,rem_text,doc)) continue;
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

/*
    CREATE TABLE drop_pax_doc_errors
    (part_key DATE,
     pax_id NUMBER(9) NOT NULL,
     country VARCHAR2(3) NOT NULL,
     column_name VARCHAR2(20) NOT NULL);

    CREATE TABLE drop_crs_pax_doc_errors
    (part_key DATE,
     pax_id NUMBER(9) NOT NULL,
     country VARCHAR2(3) NOT NULL,
     column_name VARCHAR2(20) NOT NULL);
  */

int alter_pax_doc3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");
  
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_pax_doc(vpax_id IN pax_doc.pax_id%TYPE, "
    "                                          vissue_country IN pax_doc.issue_country%TYPE, "
    "                                          vnew_issue_country IN pax_doc.issue_country%TYPE, "
    "                                          vnationality IN pax_doc.nationality%TYPE, "
    "                                          vnew_nationality IN pax_doc.nationality%TYPE) "
    "IS "
    "BEGIN "
    "  IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL OR "
    "     vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vissue_country,'issue_country'); "
    "    END IF; "
    "    IF vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vnationality,'nationality'); "
    "    END IF; "
    "  ELSE "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NOT NULL AND vissue_country<>vnew_issue_country OR "
    "       vnationality IS NOT NULL AND vnew_nationality IS NOT NULL AND vnationality<>vnew_nationality THEN "
    "      UPDATE pax_doc SET issue_country=vnew_issue_country, nationality=vnew_nationality WHERE pax_id=vpax_id; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_id, issue_country, nationality "
    "FROM pax_doc "
    "WHERE pax_id>=:low_pax_id AND pax_id<:high_pax_id";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);
  
  
  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_pax_doc(:pax_id, :issue_country, :new_issue_country, :nationality, :new_nationality); "
    "END; ";
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("issue_country", otString);
  UpdQry.DeclareVariable("new_issue_country", otString);
  UpdQry.DeclareVariable("nationality", otString);
  UpdQry.DeclareVariable("new_nationality", otString);
  

  int processed=0;
  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    alter_wait(processed);
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("issue_country", Qry.FieldAsString("issue_country"));
      UpdQry.SetVariable("new_issue_country", GetPaxDocCountryCode(Qry.FieldAsString("issue_country")));
      UpdQry.SetVariable("nationality", Qry.FieldAsString("nationality"));
      UpdQry.SetVariable("new_nationality", GetPaxDocCountryCode(Qry.FieldAsString("nationality")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };
  
  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_pax_doc";
  Qry.Execute();
  
  return 0;
};

int alter_pax_doco3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_pax_doco(vpax_id IN pax_doco.pax_id%TYPE, "
    "                                          vapplic_country IN pax_doco.applic_country%TYPE, "
    "                                          vnew_applic_country IN pax_doco.applic_country%TYPE) "
    "IS "
    "BEGIN "
    "  IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vapplic_country,'applic_country'); "
    "    END IF; "
    "  ELSE "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NOT NULL AND vapplic_country<>vnew_applic_country THEN "
    "      UPDATE pax_doco SET applic_country=vnew_applic_country WHERE pax_id=vpax_id; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_id, applic_country "
    "FROM pax_doco "
    "WHERE pax_id>=:low_pax_id AND pax_id<:high_pax_id";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);


  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_pax_doco(:pax_id, :applic_country, :new_applic_country); "
    "END; ";
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("applic_country", otString);
  UpdQry.DeclareVariable("new_applic_country", otString);

  int processed=0;
  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    alter_wait(processed);
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("applic_country", Qry.FieldAsString("applic_country"));
      UpdQry.SetVariable("new_applic_country", GetPaxDocCountryCode(Qry.FieldAsString("applic_country")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_pax_doco";
  Qry.Execute();

  return 0;
};

int alter_crs_pax_doc3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM crs_pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM crs_pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_crs_pax_doc(vpax_id IN crs_pax_doc.pax_id%TYPE, "
    "                                          vissue_country IN crs_pax_doc.issue_country%TYPE, "
    "                                          vnew_issue_country IN crs_pax_doc.issue_country%TYPE, "
    "                                          vnationality IN crs_pax_doc.nationality%TYPE, "
    "                                          vnew_nationality IN crs_pax_doc.nationality%TYPE) "
    "IS "
    "BEGIN "
    "  IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL OR "
    "     vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL THEN "
    "      INSERT INTO drop_crs_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vissue_country,'issue_country'); "
    "    END IF; "
    "    IF vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "      INSERT INTO drop_crs_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vnationality,'nationality'); "
    "    END IF; "
    "  ELSE "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NOT NULL AND vissue_country<>vnew_issue_country OR "
    "       vnationality IS NOT NULL AND vnew_nationality IS NOT NULL AND vnationality<>vnew_nationality THEN "
    "      UPDATE crs_pax_doc SET issue_country=vnew_issue_country, nationality=vnew_nationality WHERE pax_id=vpax_id; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_id, issue_country, nationality "
    "FROM crs_pax_doc "
    "WHERE pax_id>=:low_pax_id AND pax_id<:high_pax_id";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);


  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_crs_pax_doc(:pax_id, :issue_country, :new_issue_country, :nationality, :new_nationality); "
    "END; ";
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("issue_country", otString);
  UpdQry.DeclareVariable("new_issue_country", otString);
  UpdQry.DeclareVariable("nationality", otString);
  UpdQry.DeclareVariable("new_nationality", otString);


  int processed=0;
  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    alter_wait(processed);
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("issue_country", Qry.FieldAsString("issue_country"));
      UpdQry.SetVariable("new_issue_country", GetPaxDocCountryCode(Qry.FieldAsString("issue_country")));
      UpdQry.SetVariable("nationality", Qry.FieldAsString("nationality"));
      UpdQry.SetVariable("new_nationality", GetPaxDocCountryCode(Qry.FieldAsString("nationality")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_crs_pax_doc";
  Qry.Execute();

  return 0;
};

int alter_crs_pax_doco3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM crs_pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM crs_pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_crs_pax_doco(vpax_id IN crs_pax_doco.pax_id%TYPE, "
    "                                          vapplic_country IN crs_pax_doco.applic_country%TYPE, "
    "                                          vnew_applic_country IN crs_pax_doco.applic_country%TYPE) "
    "IS "
    "BEGIN "
    "  IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "      INSERT INTO drop_crs_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(NULL,vpax_id,vapplic_country,'applic_country'); "
    "    END IF; "
    "  ELSE "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NOT NULL AND vapplic_country<>vnew_applic_country THEN "
    "      UPDATE crs_pax_doco SET applic_country=vnew_applic_country WHERE pax_id=vpax_id; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_id, applic_country "
    "FROM crs_pax_doco "
    "WHERE pax_id>=:low_pax_id AND pax_id<:high_pax_id";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);


  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_crs_pax_doco(:pax_id, :applic_country, :new_applic_country); "
    "END; ";
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("applic_country", otString);
  UpdQry.DeclareVariable("new_applic_country", otString);

  int processed=0;
  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    alter_wait(processed);
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("applic_country", Qry.FieldAsString("applic_country"));
      UpdQry.SetVariable("new_applic_country", GetPaxDocCountryCode(Qry.FieldAsString("applic_country")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_crs_pax_doco";
  Qry.Execute();

  return 0;
};

int alter_pax_doc4(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("country", otString);

  int processed=0;
  for(int pass=0;pass<=1;pass++)
  {

    Qry.Clear();
    if (pass==0)
      Qry.SQLText="SELECT pax_id, country, column_name, "
                  "       DECODE( column_name, 'applic_country', 'pax_doco', 'pax_doc') AS table_name "
                  "FROM drop_pax_doc_errors WHERE part_key IS NULL";
    else
      Qry.SQLText="SELECT pax_id, country, column_name, "
                  "       DECODE( column_name, 'applic_country', 'crs_pax_doco', 'crs_pax_doc') AS table_name "
                  "FROM drop_crs_pax_doc_errors WHERE part_key IS NULL";
    
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      alter_wait(processed);
      ostringstream sql;
      sql << "UPDATE " << Qry.FieldAsString("table_name") << " "
          << "SET " << Qry.FieldAsString("column_name") << "=NULL "
          << "WHERE pax_id=:pax_id AND " << Qry.FieldAsString("column_name") << "=:country";
      UpdQry.SQLText=sql.str().c_str();
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("country", Qry.FieldAsString("country"));
      UpdQry.Execute();
      processed+=UpdQry.RowsProcessed();
      OraSession.Commit();
    };
  };
  
  printf("%d iterations processed. stop!\n", processed);
  
  return 0;
};

int alter_arx_pax_doc4(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.DeclareVariable("part_key", otDate);
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("country", otString);

  int processed=0;

  Qry.Clear();
  Qry.SQLText="SELECT part_key, pax_id, country, column_name, "
              "       DECODE( column_name, 'applic_country', 'arx_pax_doco', 'arx_pax_doc') AS table_name "
              "FROM drop_pax_doc_errors WHERE part_key IS NOT NULL";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    alter_wait(processed);
    ostringstream sql;
    sql << "UPDATE " << Qry.FieldAsString("table_name") << " "
        << "SET " << Qry.FieldAsString("column_name") << "=NULL "
        << "WHERE part_key=:part_key AND pax_id=:pax_id AND " << Qry.FieldAsString("column_name") << "=:country";
    UpdQry.SQLText=sql.str().c_str();
    UpdQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
    UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
    UpdQry.SetVariable("country", Qry.FieldAsString("country"));
    UpdQry.Execute();
    processed+=UpdQry.RowsProcessed();
    OraSession.Commit();
  };

  printf("%d iterations processed. stop!\n", processed);

  return 0;
};

int alter_arx_pax_doc3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_pax_doc";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");
  
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_arx_pax_doc(vpart_key IN arx_pax_doc.part_key%TYPE, "
    "                                          vpax_id IN arx_pax_doc.pax_id%TYPE, "
    "                                          vissue_country IN arx_pax_doc.issue_country%TYPE, "
    "                                          vnew_issue_country IN arx_pax_doc.issue_country%TYPE, "
    "                                          vnationality IN arx_pax_doc.nationality%TYPE, "
    "                                          vnew_nationality IN arx_pax_doc.nationality%TYPE) "
    "IS "
    "BEGIN "
    "  IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL OR "
    "     vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(vpart_key,vpax_id,vissue_country,'issue_country'); "
    "    END IF; "
    "    IF vnationality IS NOT NULL AND vnew_nationality IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(vpart_key,vpax_id,vnationality,'nationality'); "
    "    END IF; "
    "  ELSE "
    "    IF vissue_country IS NOT NULL AND vnew_issue_country IS NOT NULL AND vissue_country<>vnew_issue_country OR "
    "       vnationality IS NOT NULL AND vnew_nationality IS NOT NULL AND vnationality<>vnew_nationality THEN "
    "      UPDATE arx_pax_doc SET issue_country=vnew_issue_country, nationality=vnew_nationality "
    "      WHERE pax_id=vpax_id AND part_key=vpart_key; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, pax_id, issue_country, nationality "
    "FROM arx_pax_doc "
    "WHERE part_key>=:low_part_key AND part_key<:high_part_key";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);


  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_arx_pax_doc(:part_key, :pax_id, :issue_country, :new_issue_country, :nationality, :new_nationality); "
    "END; ";
  UpdQry.DeclareVariable("part_key", otDate);
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("issue_country", otString);
  UpdQry.DeclareVariable("new_issue_country", otString);
  UpdQry.DeclareVariable("nationality", otString);
  UpdQry.DeclareVariable("new_nationality", otString);
  
  
  int processed=0;

  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=0.2)
  {
    alter_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+0.2);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("issue_country", Qry.FieldAsString("issue_country"));
      UpdQry.SetVariable("new_issue_country", GetPaxDocCountryCode(Qry.FieldAsString("issue_country")));
      UpdQry.SetVariable("nationality", Qry.FieldAsString("nationality"));
      UpdQry.SetVariable("new_nationality", GetPaxDocCountryCode(Qry.FieldAsString("nationality")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };
  
  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_arx_pax_doc";
  Qry.Execute();
  
  return 0;
};

int alter_arx_pax_doco3(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_pax_doco";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_arx_pax_doco(vpart_key IN arx_pax_doco.part_key%TYPE, "
    "                                          vpax_id IN arx_pax_doco.pax_id%TYPE, "
    "                                          vapplic_country IN arx_pax_doco.applic_country%TYPE, "
    "                                          vnew_applic_country IN arx_pax_doco.applic_country%TYPE) "
    "IS "
    "BEGIN "
    "  IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NULL THEN "
    "      INSERT INTO drop_pax_doc_errors(part_key,pax_id,country,column_name) "
    "      VALUES(vpart_key,vpax_id,vapplic_country,'applic_country'); "
    "    END IF; "
    "  ELSE "
    "    IF vapplic_country IS NOT NULL AND vnew_applic_country IS NOT NULL AND vapplic_country<>vnew_applic_country THEN "
    "      UPDATE arx_pax_doco SET applic_country=vnew_applic_country "
    "      WHERE pax_id=vpax_id AND part_key=vpart_key; "
    "    END IF; "
    "  END IF; "
    "END;";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, pax_id, applic_country "
    "FROM arx_pax_doco "
    "WHERE part_key>=:low_part_key AND part_key<:high_part_key";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);


  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
    "BEGIN "
    "  alter_arx_pax_doco(:part_key, :pax_id, :applic_country, :new_applic_country); "
    "END; ";
  UpdQry.DeclareVariable("part_key", otDate);
  UpdQry.DeclareVariable("pax_id", otInteger);
  UpdQry.DeclareVariable("applic_country", otString);
  UpdQry.DeclareVariable("new_applic_country", otString);

  int processed=0;

  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=0.2)
  {
    alter_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+0.2);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      UpdQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
      UpdQry.SetVariable("pax_id", Qry.FieldAsInteger("pax_id"));
      UpdQry.SetVariable("applic_country", Qry.FieldAsString("applic_country"));
      UpdQry.SetVariable("new_applic_country", GetPaxDocCountryCode(Qry.FieldAsString("applic_country")));
      UpdQry.Execute();
      processed++;
    };
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_arx_pax_doco";
  Qry.Execute();

  return 0;
};

class TArxMoveFltExt : public TArxMoveFlt
{
  public:
    TArxMoveFltExt(BASIC::TDateTime utc_date);
    bool GetPartKey(int move_id, BASIC::TDateTime& part_key, double &date_range);
};

TArxMoveFltExt::TArxMoveFltExt(TDateTime utc_date):TArxMoveFlt(utc_date)
{
  PointsQry->Clear();
  PointsQry->SQLText =
    "SELECT act_out,est_out,scd_out,act_in,est_in,scd_in,pr_del "
    "FROM arx_points "
    "WHERE part_key=:part_key AND move_id=:move_id "
    "ORDER BY point_num";
  PointsQry->DeclareVariable("part_key",otDate);
  PointsQry->DeclareVariable("move_id",otInteger);
};

bool TArxMoveFltExt::GetPartKey(int move_id, TDateTime& part_key, double &date_range)
{
  PointsQry->SetVariable("part_key",part_key);
  return TArxMoveFlt::GetPartKey(move_id, part_key, date_range);
};

int put_move_arx_ext(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT part_key, move_id "
    "FROM arx_points "
    "WHERE part_key>=:low_part_key AND part_key<:high_part_key";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);
  
  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO move_arx_ext(part_key,move_id,date_range) "
    "VALUES(:part_key,:move_id,:date_range)";
  InsQry.DeclareVariable("part_key", otDate);
  InsQry.DeclareVariable("move_id", otInteger);
  InsQry.DeclareVariable("date_range", otInteger);
  
  TArxMoveFltExt arx(NowUTC()+ARX_MAX_DAYS()*2);
  int processed=0;
  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=1.0, processed++)
  {
    alter_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+1.0);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      int move_id=Qry.FieldAsInteger("move_id");
      TDateTime part_key=Qry.FieldAsDateTime("part_key");
      double date_range;
      if (arx.GetPartKey(move_id, part_key, date_range))
      {
        //в архив
        InsQry.SetVariable("move_id",move_id);
        if (part_key==NoExists)
        {
          printf("arx.GetPartKey: part_key=NoExists");
          continue;
        };
        if (date_range==NoExists)
        {
          printf("arx.GetPartKey: date_range=NoExists");
          continue;
        };
        if (part_key!=Qry.FieldAsDateTime("part_key"))
        {
          if (part_key>Qry.FieldAsDateTime("part_key"))
          {
            ProgError(STDLOG, "put_move_arx_ext: arx_move.part_key=%s GetPartKey.part_key=%s",
                              DateTimeToStr(Qry.FieldAsDateTime("part_key"), ServerFormatDateTimeAsString).c_str(),
                              DateTimeToStr(part_key, ServerFormatDateTimeAsString).c_str());
          };
          date_range=Qry.FieldAsDateTime("part_key")-part_key+date_range;
          part_key=Qry.FieldAsDateTime("part_key");
        };
        InsQry.SetVariable("part_key",part_key);
        if (date_range<0)
        {
          printf("arx.GetPartKey: date_range<0");
          continue;
        };
        if (date_range<1) continue;
        int date_range_int=(int)ceil(date_range);
        if (date_range_int>999)
        {
          printf("arx.GetPartKey: date_range_int>999");
          continue;
        };
        InsQry.SetVariable("date_range",date_range_int);
        InsQry.Execute();
      }
      else
      {
        printf("arx.GetPartKey: false");
      };
    };
    OraSession.Commit();
  };

  return 0;
};
/*
SELECT move_arx_ext.part_key, move_arx_ext.move_id
FROM arx_points, move_arx_ext
WHERE move_arx_ext.date_range>=5 AND
      move_arx_ext.part_key=arx_points.part_key AND
      move_arx_ext.move_id=arx_points.move_id
GROUP BY move_arx_ext.part_key, move_arx_ext.move_id
HAVING MAX(GREATEST(NVL(scd_in,TO_DATE('01.01.0001','DD.MM.YYYY')),
                    NVL(est_in,TO_DATE('01.01.0001','DD.MM.YYYY')),
                    NVL(act_in,TO_DATE('01.01.0001','DD.MM.YYYY')),
                    NVL(scd_out,TO_DATE('01.01.0001','DD.MM.YYYY')),
                    NVL(est_out,TO_DATE('01.01.0001','DD.MM.YYYY')),
                    NVL(act_out,TO_DATE('01.01.0001','DD.MM.YYYY'))))-
       MIN(LEAST(NVL(scd_in,TO_DATE('01.01.3000','DD.MM.YYYY')),
                 NVL(est_in,TO_DATE('01.01.3000','DD.MM.YYYY')),
                 NVL(act_in,TO_DATE('01.01.3000','DD.MM.YYYY')),
                 NVL(scd_out,TO_DATE('01.01.3000','DD.MM.YYYY')),
                 NVL(est_out,TO_DATE('01.01.3000','DD.MM.YYYY')),
                 NVL(act_out,TO_DATE('01.01.3000','DD.MM.YYYY'))))>=5

SELECT move_arx_ext.date_range,scd_in, est_in, act_in, scd_out, est_out, act_out
FROM arx_points, move_arx_ext
WHERE move_arx_ext.date_range>=5 AND
      move_arx_ext.part_key=arx_points.part_key AND
      move_arx_ext.move_id=arx_points.move_id
ORDER BY arx_points.part_key, arx_points.move_id*/
         
