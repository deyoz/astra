#!/usr/bin/perl

use Socket;



sub msg
{
    my($n)=shift @_;
    my($msg)=shift @_;	
    return pack("NN",$n,length($msg)) . $msg; 
}

socket(SOCK,PF_INET,SOCK_STREAM,0);
my $sin=sockaddr_in(9000,inet_aton("127.0.0.1"));

connect (SOCK,$sin) or die $!;


my ($M)=msg( 100 ,"BŒŽ‚Š‘…aaaagggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggaaaaaaaaaaaaaaaaaaaaaaE") ;
print "len=" . length($M) . "\n";
send (SOCK,$M,0) or die $!;
my($buf);
recv (SOCK,$buf,100,0) or die $!;
my ($id,$len,$answ)=unpack("NNA*",$buf);
print "$id $len $answ\n";

