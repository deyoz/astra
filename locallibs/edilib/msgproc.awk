#!/usr/bin/awk -f

BEGIN {
	header="edi_err.h";
	datfile="edimsgout.dat";
	FS="	"; 
	manstr="/* This file was generated automatically. Don't change it manually */";
	itype=0;
	count=0;
	print "Creating msg files...";
	print manstr >header;
	print manstr >datfile;
	print "#ifndef _EDI_ERR_H_\n#define _EDI_ERR_H_\n#ifdef __cplusplus\nextern \"C\" {\n#endif" > header;
	print "typedef enum {" > header;
}

substr($0,1,1)!="#" && ($0 ~ /[A-Z_"]/){
if(itype==0){
    if($0 !~ /[A-Z_]/){
	printf( "1)error in format! line %d\n",NR);
	exit(1);
    }
    printf ("\t%s=%d,\n",$1,count) > header;
    printf( "{%s,",$1)>datfile;
    itype++;
    count++;
} else {
    if(length($1)!=0){
	printf("2)error in format! line %d:'%s'\n",NR,$1);
	exit (1);
    }
    itype++;

    if(itype>2){
	printf("%s},\n",$2)>datfile;
	itype=0;
    } else {
	printf("%s,\n\t",$2)>datfile;
    }
}
}

END {
	print "} EdiErrSet_t;\n#ifdef __cplusplus\n}\n#endif /* __cplusplus */\n#endif /*_EDI_ERR_H_*/">header;
	print "done.";
}
