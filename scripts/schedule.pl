#!/usr/bin/env perl

# schedule.pl
# Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
# 5/21/2016

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

  my ($beg_time, $cmd, $cmd2, $time, $min, $sec);
  my @unsorted_playlist;

  while (<$info>)  {   
    chomp;
    ($beg_time, $cmd) = split(' ', $_, 2);

    # skip blanks
    next if (not defined $beg_time);
    next if ($beg_time eq '');
    next if (not defined $cmd);
    next if ($cmd eq '');
    
    # ignore lines where beg_time not of form #:#
    if ($beg_time =~ /^\d+\:\d+$/) {
      ($min, $sec) = split(':', $beg_time, 2);
      $time = $min * 60 + $sec;
      # print "$time : $cmd\n";  # debug

      if ($cmd eq "end") {
        push @unsorted_playlist, [$time, $cmd];
        last;
      }
      $cmd2 = "./" . $cmd . " &";
      push @unsorted_playlist, [$time, $cmd2];
    }
  }
  close $info;

  my @playlist = sort{ $a->[0] <=> $b->[0] } @unsorted_playlist;

  # print "Playing these demos:\n";
  # for my $item ( @playlist ) { print "  @$item\n"; }  # DEBUG sorted

  ## run playlist
  my $repeat = 0;
  while (1) {
    print "running playlist $repeat\n";
    my $prev_time = 0;
    for my $item ( @playlist ) {
      my $cur_time = $item->[0];
      my $sleep_time = $cur_time - $prev_time;

      # print "sleeping for $sleep_time seconds...\n";
      sleep($sleep_time);
      last if ($item->[1] eq "end");

      print "$item->[1]\n";
      system( $item->[1] );

      $prev_time = $cur_time;
    }
    $repeat = $repeat + 1;
  }

  # print "done.\n";
}

