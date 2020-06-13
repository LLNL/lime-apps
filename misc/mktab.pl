#!/usr/bin/env perl

sub usage {
print <<USAGE;
Usage: mktab.pl <input file>
USAGE
exit(1);
}

usage() if $#ARGV != 0 or $ARGV[0] =~ /^-/;

# open input file
open($fhi_st, "<", $ARGV[0]) or die(" -- error: could not open input $ARGV[0]: $!");

my %tab;

sub numeric { $a <=> $b }

sub scan {
	my ($fhi, $tref) = @_;
	my $r_ns; # read latency ns
	my $w_ns; # write latency ns
	my $app;  # application
	while (<$fhi>) {
		# print $_;
		if (/W:(\d+) R:(\d+)/) {
			$r_ns = $2; $w_ns = $1;
			# print $r_ns, ":", $w_ns, "\n";
			next;
		}
		if (/Begin of SingleDGEMM/) {
			$app = DGEMM;
			# print $app, "\n";
			next;
		}
		if (/Begin of SingleRandomAccess/) {
			$app = RANDA;
			# print $app, "\n";
			next;
		}
		if (/BFS time:(\d+\.\d+)/) { # bfs
			$tref->{BFS}{$r_ns}{$w_ns}{RUN} = $1;
			# print "BFS:", $r_ns, ":", $r_ns, "\n";
			next;
		}
		if (/overall time: (\d+\.\d+)/) { # image
			$tref->{IMAGE}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/page rank time:(\d+\.\d+)/) { # pager
			$tref->{PAGER}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/Real time used = (\d+\.\d+)/) { # dgemm, randa
			$tref->{$app}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/Lookup  time: (\d+\.\d+)/) { # rtb
			$tref->{RTB}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/Sort time: (\d+\.\d+)/) { # sort
			$tref->{SORT}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/SpMV time: (\d+\.\d+)/) { # spmv
			$tref->{SPMV}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/Triad:\s+\d+\.\d+\s+(\d+\.\d+)/) { # strm
			$tref->{STRM}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
		if (/Runtime:\s+(\d+\.\d+)/) { # xsb
			$tref->{XSB}{$r_ns}{$w_ns}{RUN} = $1;
			next;
		}
	}
}

scan($fhi_st, \%{$tab{ST}});
close($fhi_st);

print "CONFIG:STOCK\n";
foreach my $app (sort keys %{$tab{ST}}) {
	print "\n", $app, "\n";
	print "Latency";
	foreach my $wf (1, 2, 4, 8) {
		print ", R:W 1:", $wf;
	}
	print "\n";
	foreach my $r_ns (sort numeric keys %{$tab{ST}{$app}}) {
		print $r_ns;
		foreach my $wf (1, 2, 4, 8) {
			print ", ", $tab{ST}{$app}{$r_ns}{$r_ns*$wf}{RUN};
		}
		print "\n";
	}
}
