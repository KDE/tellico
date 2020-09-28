#!/usr/bin/env perl

my $group;
my $igdbgroup;

while( $input = <STDIN> )
{
    chop $input;

    if( $input =~ /^\[/)
    {
        $group=$input;
    } elsif( $input =~ /^APIv3 Key/)
    {
        $igdbgroup=$group;
    }
}

if( $igdbgroup ne '')
{
    print "# DELETE ${igdbgroup}APIv3 Key\n";
}
