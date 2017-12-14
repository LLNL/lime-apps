#!/usr/bin/env perl

sub usage {
print <<USAGE;
Usage: mktab_overflow.pl <input file A>
USAGE
exit(1);
}

usage() if $#ARGV != 0 or $ARGV[0] =~ /^-/;

# open input file A
open($fhi_a, "<", $ARGV[0]) or die(" -- error: could not open input $ARGV[0]: $!");

sub scan {
	my ($fhi, $tref) = @_;
	my $entries; # entries
	my $load; # load factor
	my $histo = 0; # flag for histogram capture
	while (<$fhi>) {
		# print $_;
		if (/PSL End/) {
			$histo = 0;
			next;
		}
		if ($histo && /(\d+) (\d+)/) {
			$tref->{$entries}{$load}[$1] = $2;
			next;
		}
		if (/^entries: (\d+)/) {
			$entries = sprintf "%02u", $1;
			# $entries = $1;
			next;
		}
		if (/load_factor \(elem\): (\d+\.\d+)/) {
			$load = sprintf "%.2f", $1;
			next;
		}
		if (/PSL Count/) {
			$histo = 1;
			next;
		}
	}
}

my %tab;

scan($fhi_a, \%{$tab{_A}});
close($fhi_a);

print "\n";
print "\"RTB PSL - $ARGV[0]\"\n";

my $tref = \%{$tab{_A}};
foreach my $_e (sort keys %{$tref}) { # entries

	print "\n\"Entries: $_e Mi\"\n";
	my $eref = \%{$tref->{$_e}};
	my $entries = $_e * 1024 * 1024;
	foreach my $_l (sort keys %{$eref}) { # load
		print "\"Load Factor: $_l\"\n";
		my $lref = \@{$eref->{$_l}};
		my @sz;
		my $sum = 0;
		for my $_p (reverse 1 .. $#{$lref}) { # PSL
			$sum += $lref->[$_p];
			$sz[$_p] = $sum / $entries * 100;
		}
		for my $_p (1 .. $#{$lref}) { # PSL
			print "$_p $sz[$_p]\n";
		}
	}

}
