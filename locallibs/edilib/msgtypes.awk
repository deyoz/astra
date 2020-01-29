#!/usr/bin/awk -f

#query/response: QUERY,RESPONSE
#msg type:       EDINOMES, ...
# @if or @end - defined

function set_files()
{
	fmtypes=sprintf("%s/%s%s",  BASEDIR, basename, ".etp");
	fmhead= sprintf("%s/%s%s%s",BASEDIR, "edi_", basename, ".h");
	manstr="/* This file was generated automatically. Don't change it manually */";

	print manstr>fmtypes;
	print manstr>fmhead;

	print "#ifndef _EDI_MSG_TYPES_H_\n#define _EDI_MSG_TYPES_H_\n#include \"edilib/edi_types.h\"\n#ifdef __cplusplus\nextern \"C\" {\n#endif">fmhead;
}

BEGIN {
	FS=",";
	error=0;
    if(length(BASEDIR) == 0)
    {
	   BASEDIR=".";
    }
    basename = BASEFILENAME
    
    print "Creating edi_types files...";
#    print "BASEDIR = " BASEDIR;
#msg_types
	i_mt=0;
	i_mn=0;
#   rtypes - array
#	rfuncs - array

#flags
	i_mf=0;
#   rflags - array

#proc func
	i_pf=0;
#send func
	i_sf=0;
	fmtypes="";
	fmhead="";
}

substr($0,1,3)=="@if" || substr($0,1,4)=="@end"{
    if(length(fmtypes)==0){
        set_files();
    }
    printf ("#%s\n", substr($0,2,length($0)-1))>fmtypes;
}

substr($0,0,1)!="#" && ($1 ~ /[A-Z_"]/) && NF==8{
    if(length(fmtypes)==0){
        set_files();
    }
    gsub(" ","",$1);
    if (length($1)!=6){
	printf ("\n#error Message name length is %d. Mast be 6 %s:%s\n",length($1),FILENAME,FNR)>fmhead;
#        delete fmtypes;
	exit(1);
    }
    for(i=1;i<=i_mt;i++){
	if(rtypes[i]==$1){
	    printf("\n#error Dublicate in types detected! Type is %s %s:%s\n", $1,FILENAME,FNR)>fmhead;
            exit(1);
	}
    }
    rtypes[++i_mt]=$1;

    gsub(" ","",$3);
    if($3!="QUERY" && $3!="RESPONSE"){
	printf("\n#error Request type must be QUERY or RESPONSE only. %s in %s:%s",$3,FILENAME,FNR)>fmhead;
        exit(1);
    }
    gsub(" ","",$4);
    gsub(" ","",$5);
    if($4 != "NULL"){
	rfuncs[++i_mn]=$4;
    }
    if($5 != "NULL"){
	rfuncs[++i_mn]=$5;
    }

    printf("{\"%s\",%s,%s,%s,%s,%s,%s,%s,%s,NULL},\n", $1,$1,$2,$3,$4,$5,$6,$7,$8)>fmtypes;

    gsub(" ","",$8);
    if((nf=split($8,flags,"|"))){
	for(i=1;i<=nf;i++){
	    if(flags[i] !~ /[A-Za-z]/){
		if(nf==1){
		    break;
		}else {
		    printf ("\n#error Flags must be with alpha characters %s:%s\n",FILENAME,FNR)>fmhead;
                    exit(1);
		}
	    }
	    if(i_mf>0){
		for (j=1;j<=i_mf;j++){
		    if(flags[i]==rflags[j]){
                        break;
		    }
		}
		if(j>i_mf){
                    rflags[++i_mf]=flags[i];
		}
	    } else {
                rflags[++i_mf]=flags[i];
	    }
	}
    }
}

substr($0,0,1)!="#" && ($0 ~ /[[:print:]]/) && NF!=8 && !(substr($0,1,3)=="@if" || substr($0,1,4)=="@end"){
    if(length(fmtypes)==0){
        set_files();
    }
    printf ("#error Format error! Line: %s:%d\n", FILENAME,FNR)>fmhead;
    exit(1);
}

END{
if(error==0){
  if(i_mf>0){
    print "\ntypedef enum {" > fmhead;
    flagnum=1;
    for (i=1;i<=i_mf;i++){
	printf ("\t%s\t=0x%04X,\n",rflags[i],flagnum)>fmhead;
        flagnum*=2;
    }
    print "} edi_msg_flags;\n" > fmhead;

  }
  if(i_mt>0){
    print "typedef enum {" > fmhead;
    for (i=1;i<=i_mt;i++)
	printf("\t%s = %d,\n",rtypes[i],i)>fmhead;
    print "} edi_msg_types;\n" > fmhead;
  }

  if(i_mn>0){
    for (i=1;i<=i_mn;i++)
	printf(" int %s (edi_mes_head *pHead, void *udata, void *data, int *err);\n",rfuncs[i])>fmhead;
  }
  print "#ifdef __cplusplus\n}\n#endif /* __cplusplus */\n#endif /*_EDI_MSG_TYPES_H_*/">fmhead;

  print "done.";
}
}
