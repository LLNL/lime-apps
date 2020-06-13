#!/usr/bin/env perl

sub usage {
print <<USAGE;
Usage: mktab_spill.pl <input file A>
USAGE
exit(1);
}

usage() if $#ARGV != 0 or $ARGV[0] =~ /^-/;

# open input file A
open($fhi_a, "<", $ARGV[0]) or die(" -- error: could not open input $ARGV[0]: $!");

sub numeric { $a <=> $b }

sub scan {
	my ($fhi, $tref) = @_;
	my $entries; # entries
	my $load; # load factor
	my $psl; # probe sequence length
	while (<$fhi>) {
		# print $_;
		if (/psl: (\d+) entries: (\d+) load: \.(\d+)/) {
			$entries = $2;
			$load = $3;
			$psl = $1;
			next;
		}
		if (/spill size: (\d+)/) {
			$tref->{$entries}{$load}{$psl} = $1;
			next;
		}
	}
}

my %tab;

scan($fhi_a, \%{$tab{_A}});
close($fhi_a);

print "\n";
print "\"RTB Spill Factor - $ARGV[0]\"\n";

my $tref = \%{$tab{_A}};
foreach my $_i (sort numeric keys %{$tref}) { # entries

	print "\n\"Entries: $_i Mi\"\n";
	my $ref = \%{$tref->{$_i}};

	# header
	print "\"Load Factor / Max PSL\"";
	foreach my $_y (sort numeric keys %{$ref->{(sort numeric keys %{$ref})[0]}}) { # psl
		print ", $_y";
	}
	print "\n";

	# table
	foreach my $_x (sort numeric keys %{$ref}) { # load
		print $_x;
		foreach my $_y (sort numeric keys %{$ref->{$_x}}) { # psl
			my $entries = $_i * 1024 * 1024;
			# my $tmp = $ref->{$_x}{$_y};
			my $tmp = $ref->{$_x}{$_y} / $entries * 100; # spill % (spills / table size)
			print ", ", $tmp;
		}
		print "\n";
	}

}
