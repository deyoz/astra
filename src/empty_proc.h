//---------------------------------------------------------------------------
#ifndef _EMPTY_PROC_H_
#define _EMPTY_PROC_H_

int get_events_stat(int argc,char **argv);
int get_events_stat2(int argc,char **argv);
int get_sirena_rozysk_stat(int argc,char **argv);
int season_to_schedules(int argc,char **argv);
int create_tlg(int argc,char **argv);
int get_basel_aero_stat(int argc,char **argv);
int test_trfer_exists(int argc,char **argv);
int bind_trfer_trips(int argc,char **argv);
int unbind_trfer_trips(int argc,char **argv);
int test_typeb_utils(int argc,char **argv);
int test_typeb_utils2(int argc,char **argv);
int compare_apis(int argc,char **argv);
int test_sopp_sql(int argc,char **argv);
int test_file_queue(int argc,char **argv);
int mobile_stat(int argc,char **argv);
int fill_counters_by_subcls(int argc,char **argv);
int convert_codeshare(int argc,char **argv);
int verifyHTTP(int argc,char **argv);

#endif
