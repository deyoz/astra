#!/usr/bin/perl

use Socket;



sub msg
{
    my($n)=shift @_;
    my($msg)=shift @_;	
    return pack("NN",$n,length($msg)) . $msg; 
}

socket(SOCK,PF_INET,SOCK_STREAM,0);
my $sin=sockaddr_in(9000,inet_aton("10.1.9.136"));

connect (SOCK,$sin) or die $!;


my ($M)=msg( 100 ,"BŒŽ‚Š‘…aaaaaaaaaaaaaaaaaaaaaaaaaaaE") ;
print "len=" . length($M) . "\n";
send (SOCK,substr($M,0,1),0) or die $!;
send (SOCK,substr($M,1,1),0) or die $!;
send (SOCK,substr($M,2,1),0) or die $!;
send (SOCK,substr($M,3,1),0) or die $!;
send (SOCK,substr($M,4),0) or die $!;
recv (SOCK,$buf,100,0) or die $!;
my ($id,$len,$answ)=unpack("NNA*",$buf);
print "$id $len $answ\n";

