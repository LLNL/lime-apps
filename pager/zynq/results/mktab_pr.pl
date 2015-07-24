#!/usr/bin/env perl

# mktab_pr.pl <input file base>

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
		if (/page rank time:(\d+\.\d+)/) { # page rank
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
print "PageRank Run Time (seconds) - Stock Algorithm\n";
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
print "PageRank Run Time (seconds) - View Buffer Algorithm\n";
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
print "PageRank Speedup\n";
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
print "PageRank Speedup Upper Bound\n";
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
print "PageRank CPU Data Transferred (bytes) - Stock Algorithm\n";
my $ST_CPU_Tran = 0;
my $ST_count = 0;
foreach my $qd (sort keys %{$tab{ST}}) {
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		$ST_CPU_Tran += ($tab{ST}{$qd}{$td}{CPU_TranW} + $tab{ST}{$qd}{$td}{CPU_TranR}) * 32; # CPU cache line = 32 bytes
		$ST_count++;
	}
}
print "Average: ", int($ST_CPU_Tran / $ST_count), "\n";

print "\n";
print "PageRank CPU Data Transferred (bytes) - View Buffer Algorithm\n";
my $VB_CPU_Tran = 0;
my $VB_count = 0;
foreach my $qd (sort keys %{$tab{VB}}) {
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		$VB_CPU_Tran += ($tab{VB}{$qd}{$td}{CPU_TranW} + $tab{VB}{$qd}{$td}{CPU_TranR}) * 32; # CPU cache line = 32 bytes
		$VB_count++;
	}
}
print "Average: ", int($VB_CPU_Tran / $VB_count), "\n";

print "\n";
print "PageRank Energy (Joules) - Stock Algorithm\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{ST}{(sort keys %{$tab{ST}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{ST}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		my $CPU_DRAM_BytesW = $tab{ST}{$qd}{$td}{CPU_TranW} * 32; # CPU cache line = 32 bytes
		my $CPU_DRAM_BytesR = $tab{ST}{$qd}{$td}{CPU_TranR} * 32;

		my $dram_far  = $CPU_DRAM_BytesW + $CPU_DRAM_BytesR;

		my $energy =
			$dram_far  * 30.0e-12 * 8;
		print ", ", $energy;
		$tab{ST}{$qd}{$td}{ENERGY} = $energy;
	}
	print "\n";
}

print "\n";
print "PageRank Energy (Joules) - View Buffer Algorithm\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{VB}{(sort keys %{$tab{VB}})[0]}}) {
	print ", ", $td;
}
print "\n";
foreach my $qd (sort keys %{$tab{VB}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		my $MCU_DRAM_BytesW = 0;
		my $MCU_DRAM_BytesR = ($tab{VB}{$qd}{$td}{ACC_TranR} - $tab{VB}{$qd}{$td}{ACC_TranW}) * 16; # MCU cache line = 16 bytes

		my $LSU_SRAM_BytesW = $tab{VB}{$qd}{$td}{ACC_TranW} * 8; # double = 8 bytes
		my $LSU_DRAM_BytesW = 0;
		my $LSU_SRAM_BytesR = 0;
		my $LSU_DRAM_BytesR = $tab{VB}{$qd}{$td}{ACC_TranW} * 16; # LSU_TransR == LSU_TransW, HMC minimum = 16 bytes

		my $CPU_SRAM_BytesW = 0;
		my $CPU_DRAM_BytesW = $tab{VB}{$qd}{$td}{CPU_TranW} * 32; # CPU cache line = 32 bytes
		my $CPU_SRAM_BytesR = int($LSU_SRAM_BytesW * 1.0270825380417175 / 32) * 32;
		my $CPU_DRAM_BytesR = $tab{VB}{$qd}{$td}{CPU_TranR} * 32 - $CPU_SRAM_BytesR;

		my $sram_near = $LSU_SRAM_BytesW + $LSU_SRAM_BytesR;
		my $sram_far  = $CPU_SRAM_BytesW + $CPU_SRAM_BytesR;
		my $dram_near = $MCU_DRAM_BytesW + $MCU_DRAM_BytesR + $LSU_DRAM_BytesW + $LSU_DRAM_BytesR;
		my $dram_far  = $CPU_DRAM_BytesW + $CPU_DRAM_BytesR;

		my $energy =
			$sram_near *  1.0e-12 * 8 +
			$sram_far  * 21.0e-12 * 8 +
			$dram_near * 10.0e-12 * 8 +
			$dram_far  * 30.0e-12 * 8;
		print ", ", $energy;
		$tab{VB}{$qd}{$td}{ENERGY} = $energy;
	}
	print "\n";
}
