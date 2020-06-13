#!/usr/bin/env perl

# mktab_sp.pl <input stock file> <input view buf file>

my $fni_st = $ARGV[0];
my $fni_vb = $ARGV[1];

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
		if (/SpMV time: (\d+\.\d+)/) { # run time
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
print "SpMV Run Time (seconds) - Stock Algorithm\n";
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
print "SpMV Run Time (seconds) - View Buffer Algorithm\n";
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
print "SpMV Speedup\n";
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
print "SpMV Speedup Upper Bound\n";
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
print "SpMV CPU Data Transferred (bytes) - Stock Algorithm\n";
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
print "SpMV CPU Data Transferred (bytes) - View Buffer Algorithm\n";
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

print "\n";
print "SpMV Energy (Joules) - Stock Algorithm, Narrow and HMC\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{ST}{(sort keys %{$tab{ST}})[0]}}) {
	print ", ", $td;
}
print "\n";
my $ST_energy = 0;
my $ST_count = 0;
foreach my $qd (sort keys %{$tab{ST}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{ST}{$qd}}) {
		# CPU cache line = 32 bytes
		my $transport_far_bytes = ($tab{ST}{$qd}{$td}{CPU_TranW} + $tab{ST}{$qd}{$td}{CPU_TranR}) * 32;
		my $access_dram_bytes = $transport_far_bytes;
		my $energy =
			#$transport_near_bytes *  0.0e-12 * 8 +
			$transport_far_bytes  * 10.3e-12 * 8 +
			#$access_sram_bytes    *  1.0e-12 * 8 +
			$access_dram_bytes    * 19.4e-12 * 8;
		print ", ", $energy;
		#$tab{ST}{$qd}{$td}{ENERGY} = $energy;
		$ST_energy += $energy;
		$ST_count++;
	}
	print "\n";
}
print "Average, ", $ST_energy / $ST_count, "\n";

print "\n";
print "SpMV Energy (Joules) - View Buffer Algorithm, Narrow Access Memory\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{VB}{(sort keys %{$tab{VB}})[0]}}) {
	print ", ", $td;
}
print "\n";
my $VB_NAM_energy = 0;
my $VB_NAM_count = 0;
foreach my $qd (sort keys %{$tab{VB}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		my %reqs;
		$reqs{MCU}{SRAM}{W} = 0;
		$reqs{MCU}{SRAM}{R} = 0;
		$reqs{MCU}{DRAM}{W} = 0;
		$reqs{MCU}{DRAM}{R} = $tab{VB}{$qd}{$td}{ACC_TranR} - $tab{VB}{$qd}{$td}{ACC_TranW};

		$reqs{LSU}{SRAM}{W} = 0;
		$reqs{LSU}{SRAM}{R} = 0;
		$reqs{LSU}{DRAM}{W} = $tab{VB}{$qd}{$td}{ACC_TranW};
		$reqs{LSU}{DRAM}{R} = $tab{VB}{$qd}{$td}{ACC_TranW}; # LSU_TransR == LSU_TransW

		$reqs{CPU}{SRAM}{W} = 0;
		$reqs{CPU}{SRAM}{R} = 0;
		$reqs{CPU}{DRAM}{W} = $tab{VB}{$qd}{$td}{CPU_TranW};
		$reqs{CPU}{DRAM}{R} = $tab{VB}{$qd}{$td}{CPU_TranR};
		my %length = (CPU => 32, LSU => 8, MCU => 16);

		my $transport_near_bytes = 0;
		my $transport_far_bytes = 0;
		my $access_sram_bytes = 0;
		my $access_dram_bytes = 0;
		foreach my $mast (keys %reqs) {
			foreach my $ram (keys %{$reqs{$mast}}) {
				foreach my $type (keys %{$reqs{$mast}{$ram}}) {
					my $transport_bytes = $length{$mast};
					my $access_bytes = $length{$mast};
					my $requests = $reqs{$mast}{$ram}{$type};
					# Narrow Access Memory: transport min. 8 bytes, access multiple 8 bytes
					if ($ram eq "DRAM") {
						if ($transport_bytes < 8) {$transport_bytes = 8;}
						$access_bytes = int(($access_bytes+7)/8) * 8;
					}
					if ($mast eq "CPU") {
						$transport_far_bytes  += $requests * $transport_bytes;
					} else {
						$transport_near_bytes += $requests * $transport_bytes;
					}
					if ($ram eq "SRAM") {
						$access_sram_bytes += $requests * $access_bytes;
					} else {
						$access_dram_bytes += $requests * $access_bytes;
					}
				}
			}
		}
		my $energy =
			#$transport_near_bytes *  0.0e-12 * 8 +
			$transport_far_bytes  * 10.3e-12 * 8 +
			$access_sram_bytes    *  1.0e-12 * 8 +
			$access_dram_bytes    * 19.4e-12 * 8;
		print ", ", $energy;
		#$tab{VB_NAM}{$qd}{$td}{ENERGY} = $energy;
		$VB_NAM_energy += $energy;
		$VB_NAM_count++;
	}
	print "\n";
}
print "Average, ", $VB_NAM_energy / $VB_NAM_count, "\n";

print "\n";
print "SpMV Energy (Joules) - View Buffer Algorithm, HMC\n";
print "Queue Delay / Link Delay";
foreach my $td (sort keys %{$tab{VB}{(sort keys %{$tab{VB}})[0]}}) {
	print ", ", $td;
}
print "\n";
my $VB_HMC_energy = 0;
my $VB_HMC_count = 0;
foreach my $qd (sort keys %{$tab{VB}}) {
	print $qd;
	foreach my $td (sort keys %{$tab{VB}{$qd}}) {
		my %reqs;
		$reqs{MCU}{SRAM}{W} = 0;
		$reqs{MCU}{SRAM}{R} = 0;
		$reqs{MCU}{DRAM}{W} = 0;
		$reqs{MCU}{DRAM}{R} = $tab{VB}{$qd}{$td}{ACC_TranR} - $tab{VB}{$qd}{$td}{ACC_TranW};

		$reqs{LSU}{SRAM}{W} = 0;
		$reqs{LSU}{SRAM}{R} = 0;
		$reqs{LSU}{DRAM}{W} = $tab{VB}{$qd}{$td}{ACC_TranW};
		$reqs{LSU}{DRAM}{R} = $tab{VB}{$qd}{$td}{ACC_TranW}; # LSU_TransR == LSU_TransW

		$reqs{CPU}{SRAM}{W} = 0;
		$reqs{CPU}{SRAM}{R} = 0;
		$reqs{CPU}{DRAM}{W} = $tab{VB}{$qd}{$td}{CPU_TranW};
		$reqs{CPU}{DRAM}{R} = $tab{VB}{$qd}{$td}{CPU_TranR};
		my %length = (CPU => 32, LSU => 8, MCU => 16);

		my $transport_near_bytes = 0;
		my $transport_far_bytes = 0;
		my $access_sram_bytes = 0;
		my $access_dram_bytes = 0;
		foreach my $mast (keys %reqs) {
			foreach my $ram (keys %{$reqs{$mast}}) {
				foreach my $type (keys %{$reqs{$mast}{$ram}}) {
					my $transport_bytes = $length{$mast};
					my $access_bytes = $length{$mast};
					my $requests = $reqs{$mast}{$ram}{$type};
					# HMC: transport min. 16 bytes, access multiple 32 bytes
					if ($ram eq "DRAM") {
						if ($transport_bytes < 16) {$transport_bytes = 16;}
						$access_bytes = int(($access_bytes+31)/32) * 32;
					}
					if ($mast eq "CPU") {
						$transport_far_bytes  += $requests * $transport_bytes;
					} else {
						$transport_near_bytes += $requests * $transport_bytes;
					}
					if ($ram eq "SRAM") {
						$access_sram_bytes += $requests * $access_bytes;
					} else {
						$access_dram_bytes += $requests * $access_bytes;
					}
				}
			}
		}
		my $energy =
			#$transport_near_bytes *  0.0e-12 * 8 +
			$transport_far_bytes  * 10.3e-12 * 8 +
			$access_sram_bytes    *  1.0e-12 * 8 +
			$access_dram_bytes    * 19.4e-12 * 8;
		print ", ", $energy;
		#$tab{VB_HMC}{$qd}{$td}{ENERGY} = $energy;
		$VB_HMC_energy += $energy;
		$VB_HMC_count++;
	}
	print "\n";
}
print "Average, ", $VB_HMC_energy / $VB_HMC_count, "\n";
