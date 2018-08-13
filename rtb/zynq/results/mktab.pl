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
	my $mtime; # memory channel timing, <read latency ns:write latency ns>
	my $load; # load factor
	my $hit; # hit ratio
	my $zipf; # distribution, 0 = uniform, !0 = Zipf skew
	while (<$fhi>) {
		# print $_;
		if (/W:(\d+) R:(\d+)/) {
			$mtime = sprintf "%03u,%03u", $2, $1;
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
			$tref->{$mtime}{$zipf}{$load}{$hit}{LRATE} = $1;
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
foreach my $_i (sort keys %{$tref}) { # mtime
	foreach my $_j (sort keys %{$tref->{$_i}}) { # zipf

		my $str = ($_j == 0.0) ? "Uniform" : "Zipf=$_j";
		print "\n\"Memory Latency: $_i Distribution: $str\"\n";
		print "\"Load Factor / Hit Rate\"";
		my $ref = \%{$tref->{$_i}{$_j}};
		foreach my $_y (sort keys %{$ref->{(sort keys %{$ref})[0]}}) { # hit
			print ", hit $_y%";
		}
		print "\n";
		foreach my $_x (sort keys %{$ref}) { # load
			print $_x;
			foreach my $_y (sort keys %{$ref->{$_x}}) { # hit
				print ", ", $ref->{$_x}{$_y}{LRATE};
			}
			print "\n";
		}

	}
}
