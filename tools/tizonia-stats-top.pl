#!/usr/bin/perl
#
# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# Tizonia's buffer exchange information
# usage: e.g:
#       $ tail -f /home/user/temp/tizonia.log.0 | ./tizonia-stats-top.pl

use Time::Local;

$etb  = 0;
$ftb  = 0;
$to = "";
$from = "";

$comp1    = "";
$comp2    = "";
$comp3    = "";
@comps    = ();
$comps[0] = \$comp1;
$comps[1] = \$comp2;
$comps[2] = \$comp3;

$comp1_headers_lst = "";
$comp2_headers_lst = "";
$comp3_headers_lst = "";

$headers[0] = \$comp1_headers_lst;
$headers[1] = \$comp2_headers_lst;
$headers[2] = \$comp3_headers_lst;

$t0   = time();
$bpm = 0;
$bps = 0;

$exe_count = 0;

$test1 = "";
$test2 = "";

%header_lists;

format STATS_TOP =
COMPONENT NAME                                  ETB     FTB       BPM      BPS    EXE  ELAPSED TIME              CURRENT TIME
                                            @###### @######  @####.## @####.##  @####       @######              @<<<<<<<<<<<<<<<<<<<<<
$etb, $ftb, $bpm, $bps, $exe_count, $elapsed, $date
====================================================================================================================================
.

format STATS =
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$comp1, $comp1_headers_lst
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$comp2, $comp2_headers_lst
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$comp3, $comp3_headers_lst
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$test1
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$test2
Press Ctr-C to exit.
.
$^ = 'STATS_TOP';
$~ = 'STATS';
$= = 2;
$| = 1;
$t0 = 0;
while (<STDIN>) {
    $line = $_;
    chomp $line;
    # Date-time format is: 02-12-2015 15:26:02.234
    $date = substr($line, 0, 23);
    ($dd, $mm, $yyyy, $hh, $mn, $sc, $ms) = ($date =~ /(\d+)\-(\d+)\-(\d+) (\d+)\:(\d+)\:(\d+)\.(\d+)/);

    if ($mm > 0 && $mm <= 12) {
      $epoch_seconds = timelocal ($sc, $mn, $hh, $dd, $mm - 1, $yyyy);
    }

    if ($t0 == 0) {
        $t0 = $epoch_seconds;
    }

    if ($line =~ m/\[OMX_FillThisBuffer\]/ || $line =~ m/\[OMX_EmptyThisBuffer\]/) {
      if ($line =~ m/\[OMX_FillThisBuffer\]/) {
        $ftb++;
      }
      else {
        $etb++;
      }
      @tokens = split / /, $line;
      $to = @tokens[-1];
      $watchers = @tokens[-2];
      $info = @tokens[-3];
      $header = @tokens[-6];
      $from = @tokens[-12];
      if (exists $header_lists{$from}{$header}) {
        delete $header_lists{$from}{$header};
      }
      $header_lists{$to}{$header} = $header . ":" . $info . ":" . $watchers;
    }
    elsif ($line =~ m/\[OMX_EventCmdComplete\]\s\[OMX_CommandStateSet\]\s\[OMX_StateIdle\]/) {
      @tokens = split / /, $line;
      $from = @tokens[-6];
      $header_lists{$from} = ();
    }
    elsif ($line =~ m/\[OMX_EventCmdComplete\]\s\[OMX_CommandPortDisable\]/) {
      @tokens = split / /, $line;
      $from = @tokens[5];
      $header_lists{$from} = ();
    }
    elsif ($line =~ m/\[OMX_EventCmdComplete\]\s\[OMX_CommandPortEnable\]/) {
      if ($line =~ m/renderer/) {
        $exe_count++;
      }
    }

    $i = 0;
    foreach $key (keys %header_lists) {
      ${$comps[$i]} = $key;
      ${$headers[$i]} = join ', ', values( %{$header_lists{$key}} );
      $i += 1;
    }

    system 'clear'; write();

    $t1 = $epoch_seconds;
    $elapsed = $t1 - $t0;
    if ($elapsed > 0)
    {
       $bps = (($etb + $ftb) / $elapsed);
       $bpm = $bps * 60;
    }
}
