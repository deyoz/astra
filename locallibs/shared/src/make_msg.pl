#!/usr/bin/perl -w
use strict;

sub get_line
{
    my $fh = shift;
LINE: while(<$fh>) {
          next if /^$/;
          last LINE;
      }
      if($_) {
# trim spaces
          s/^\s+//;
          s/\s+$//;
          return $_;
      }
}

#my $shared_path = $ENV{SHARED_PATH};
#die "SHARED_PATH environment undefined\n" unless $shared_path;
my $datfile = "msg.dat";
my $msgfile = "msg.h1";


open(DAT, $datfile) || die "Cannot open $datfile: $!";

my $index = 0;
my $field = 0;
my $enum;
my $msg;

CYCLE: while(1) {
           $_ = get_line(\*DAT);
           last CYCLE unless $_;
# only word chars and uppercase
           (/\U$_/ and /^\s*\w+\s*$/) or die "Wrong error code format: $_\n";
           $index++;
           $enum .= "$_, ";
# 4 codes per line
           $enum .= "\n" unless $index % 4;

           $_ = get_line(\*DAT);
           die "Wrong $datfile format.\n" unless $_;
           die "Unescaped quote: $_" if /[^\\]"/;
           $msg .= "\"$_\",\n";
       }

open(MSG, ">$msgfile") or die "Cannot open $msg: $!";

print MSG <<EOF;
#ifndef __MSG_H__
#define __MSG_H__

/* Коды ошибок */
enum ErrorCodes
{
$enum
};

static const char *err_msg[] = {
$msg
};

#endif /* __MSG_H__ */
EOF
