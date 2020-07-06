#!/usr/bin/env perl

sub usage {
print <<USAGE;
Usage: mktab.pl <input file A> [<input file B>]
USAGE
exit(1);
}

usage() if $#ARGV < 0 or $#ARGV > 1 or $ARGV[0] =~ /^-/;

# open input file A
open($fhi_a, "<", $ARGV[0]) or die(" -- error: could not open input $ARGV[0]: $!");
# open input file B
if ($#ARGV == 1) {
	open($fhi_b, "<", $ARGV[1]) or die(" -- error: could not open input $ARGV[1]: $!");
}

sub scan {
	my ($fhi, $tref) = @_;
	my $load; # load factor
	my $hit; # hit ratio
	my $zipf; # distribution, 0 = uniform, !0 = Zipf skew
	my $run; # run number
	while (<$fhi>) {
		# print $_;
		if (/run: (\d+)/) {
			$run = $1;
			next;
		}
		if (/load_factor \(elem\): (\d+\.\d+)/) {
			$load = sprintf "%.2f", $1;
			next;
		}
		if (/Lookup  hits: \d+ (\d+\.\d+)/) {
			$hit = sprintf "%.0f", $1;
			next;
		}
		if (/Lookup  zipf: (\d+\.\d+)/) {
			$zipf = $1;
			next;
		}
		if (/Lookup  rate: (\d+\.\d+)/) {
			$tref->{$zipf}{$load}{$hit}{LRATE}{$run} = $1;
			next;
		}
	}
}

my %tab;

scan($fhi_a, \%{$tab{_A}});
close($fhi_a);
if ($#ARGV == 1) {
	scan($fhi_b, \%{$tab{_B}});
	close($fhi_b);
}

print "\n";
print "\"RTB Lookup rate (ops/sec) - $ARGV[0]\"\n";

my $tref = \%{$tab{_A}};
foreach my $_i (sort keys %{$tref}) { # zipf

	my $str = ($_i == 0.0) ? "Uniform" : "Zipf=$_i";
	print "\n\"Distribution: $str\"\n";
	print "\"Load Factor / Hit Rate\"";
	my $ref = \%{$tref->{$_i}};
	foreach my $_y (sort keys %{$ref->{(sort keys %{$ref})[0]}}) { # hit
		print ", hit $_y%";
	}
	print "\n";
	foreach my $_x (sort keys %{$ref}) { # load
		print $_x;
		foreach my $_y (sort keys %{$ref->{$_x}}) { # hit
			my $tmp = 0.0;
			foreach my $_z (keys %{$ref->{$_x}{$_y}{LRATE}}) { # run
				$tmp += $ref->{$_x}{$_y}{LRATE}{$_z};
			}
			$tmp /= keys %{$ref->{$_x}{$_y}{LRATE}};
			print ", ", $tmp;
		}
		print "\n";
	}

}
