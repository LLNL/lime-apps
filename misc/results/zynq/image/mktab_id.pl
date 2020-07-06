#!/usr/bin/env perl

# mktab_id.pl <input file base>

my $fbase = $ARGV[0];
my $fni_st = $fbase . ".ST.txt";
my $fni_vb = $fbase . ".VB.txt";

my %tab;

sub scan {
	my ($fhi, $tref) = @_;
	my $q_del; # queue delay
	my $t_del; # transport or link delay
	while (<$fhi>) {
		# print $_;
		if (/QUEUE:(\d+) TRANS:(\d+)/) {
			$q_del = $1; $t_del = $2; # nanoseconds
			next;
		}
		if (/QUEUE_W:(\d+) QUEUE_R:(\d+) TRANS:(\d+)/) {
			$q_del = ($1+$2)/2; $t_del = $3; # nanoseconds
			next;
		}
		if (/overall time: (\d+\.\d+)/) { # image difference
			$tref->{$q_del}{$t_del}{RUN} = $1; # seconds
			next;
		}
		if (/Oper\. time: (\d+\.\d+)/) {
			$tref->{$q_del}{$t_del}{OPER} = $1; # seconds
			next;
		}
		if (/Cache time: (\d+\.\d+)/) {
			$tref->{$q_del}{$t_del}{CACHE} = $1; # seconds
			next;
		}
		if (/CPU (\d+) (\d+)/) {
			$tref->{$q_del}{$t_del}{CPU_TranW} = $1; # count
			$tref->{$q_del}{$t_del}{CPU_TranR} = $2; # count
			next;
		}
		if (/ACC (\d+) (\d+)/) {
			$tref->{$q_del}{$t_del}{ACC_TranW} = $1; # count
			$tref->{$q_del}{$t_del}{ACC_TranR} = $2; # count
			next;
		}
	}
}

open(my $fhi_st, "<", $fni_st) or die(" -- error: could not open input: $!");
open(my $fhi_vb, "<", $fni_vb) or die(" -- error: could not open input: $!");

scan($fhi_st, \%{$tab{ST}});
close($fhi_st);
scan($fhi_vb, \%{$tab{VB}});
close($fhi_vb);

#foreach my $qd (sort keys %{$tab{ST}}) {
#	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
#		print "### QUEUE:", $qd, " ", "LINK:", $td, " ###\n";
#		foreach my $field (keys %{$tab{ST}{$qd}{$td}}) {
#			print "ST ", $field, ":", $tab{ST}{$qd}{$td}{$field}, "\n";
#			print "VB ", $field, ":", $tab{VB}{$qd}{$td}{$field}, "\n";
#		}
#	}
#}

print "\n";
print "ImageDifference Run Time (seconds) - Stock Algorithm\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{ST}{(sort keys %{$tab{ST}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{ST}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		print ", ", $tab{ST}{$qd}{$td}{RUN};
	}
	print "\n";
}

print "\n";
print "ImageDifference Run Time (seconds) - View Buffer Algorithm\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{VB}{(sort keys %{$tab{VB}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{VB}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		print ", ", $tab{VB}{$qd}{$td}{RUN};
	}
	print "\n";
}

print "\n";
print "ImageDifference Speedup\n";
print "ST_RUN/VB_RUN\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{ST}{(sort keys %{$tab{ST}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{ST}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		print ", ", $tab{ST}{$qd}{$td}{RUN} / $tab{VB}{$qd}{$td}{RUN};
	}
	print "\n";
}

print "\n";
print "ImageDifference Speedup Upper Bound\n";
print "ST_RUN/(VB_OPERATION+VB_CACHE)\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{ST}{(sort keys %{$tab{ST}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{ST}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		print ", ", $tab{ST}{$qd}{$td}{RUN} / ($tab{VB}{$qd}{$td}{OPER}+$tab{VB}{$qd}{$td}{CACHE});
	}
	print "\n";
}

print "\n";
print "ImageDifference CPU Data Transferred (bytes) - Stock Algorithm\n";
my $ST_CPU_bytes = 0;
my $ST_CPU_count = 0;
foreach my $qd (sort keys %{$tab{ST}}) {
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		# CPU cache line = 32 bytes
		$ST_CPU_bytes += ($tab{ST}{$qd}{$td}{CPU_TranW} + $tab{ST}{$qd}{$td}{CPU_TranR}) * 32;
		$ST_CPU_count++;
	}
}
print "Average, ", int($ST_CPU_bytes / $ST_CPU_count), "\n";

print "\n";
print "ImageDifference CPU Data Transferred (bytes) - View Buffer Algorithm\n";
my $VB_CPU_bytes = 0;
my $VB_CPU_count = 0;
foreach my $qd (sort keys %{$tab{VB}}) {
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		# CPU cache line = 32 bytes
		$VB_CPU_bytes += ($tab{VB}{$qd}{$td}{CPU_TranW} + $tab{VB}{$qd}{$td}{CPU_TranR}) * 32;
		$VB_CPU_count++;
	}
}
print "Average, ", int($VB_CPU_bytes / $VB_CPU_count), "\n";
