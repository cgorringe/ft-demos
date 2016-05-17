#!/usr/bin/env perl

# schedule.pl
# Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)

use strict;
use warnings;

{
  if (scalar @ARGV < 1) {
    print STDERR "Schedule Flaschen-Taschen Demos\n";
    print STDERR "Usage:\n  $0 playlist.txt\n";
    exit(1);
  }

  my $file = $ARGV[0];
  open my $info, $file or die "Could not open $file: $!";

  while (my $line = <$info>)  {   
    print $line;
    my ($beg_time, $end_time, $cmd) = split(' ', $line);
    print "$beg_time to $end_time : $cmd\n";
  }

close $info;
}

