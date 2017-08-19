#!/usr/bin/env perl

my $input;
my $oldvalue;

while( $input = <STDIN> )
{
	chop $input;
	
	if( $input =~ /^Write Images In File\=(.*)/)
	{
		$oldvalue=$1;
		if( $oldvalue eq "true" )
		{
			print "Image Location=ImagesInFile\n";
		}
		else
		{
            print "Image Location=ImageAppDir\n";
		}
	}
}

print "# DELETE [General Options]Write Images In File\n";
