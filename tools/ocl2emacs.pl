#!/usr/bin/perl

# sudo apt-get install libfile-find-rule-perl

use strict;
use warnings;
use v5.18;
use File::Find::Rule;

my @ignoredfiles = ('tizcoretc.c', 'tiztcproc.c');

while (<>) {
    chomp $_;
    my ($filename, $other) = split (/:/, $_, 2);
    if (length $filename) {
        next if ($filename ~~ @ignoredfiles);
        my ($ext) = $filename =~ /(\.[^.]+)$/;
        given ($ext) {
            when ('.c') {
                my $dir = '/home/joni/work/tizonia' ;
                my @files = File::Find::Rule->file()
                    ->name($filename)
                    ->in($dir);

                say "$files[0]:$other" if length $files[0];
            }
        }
    }
}
