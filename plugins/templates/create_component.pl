#!/usr/bin/perl -w
#
# Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
#
# This file is part of Tizonia
#
# Tizonia is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
# more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.

use strict;
use warnings;
use Tie::File;
use Getopt::Long;
use vars qw/ %opt /;
use version;

our $VERSION = qv('0.1.0');

sub usage() {
    print("\nCopyright (C) 2012 Aratelia Limited - Juan A. Rubio\n");
    print("Creation of skeleton components from a template.\n");
    print
"\nUsage: $0 -l <library_name> -n <comp_name> -r <comp_role> -a <component_acronym>\n\n";
    print
"\nE.g.: perl $0 -l libtizvp8d -n \"VP8 Decoder\" -r video_decoder.vp8 -a vp8d\n\n";
    print
"\nE.g.: perl $0 -l libtizsdlivr -n \"SDL Video Renderer\" -r iv_renderer.yuv.overlay -a sdlivr\n\n";
    exit;
}

sub init() {
    use Getopt::Std;
    if ( !@ARGV || 8 != $#ARGV + 1 ) {
        usage();
    }
    my $opt_string = 'l:n:r:a:';
    getopts( "$opt_string", \%opt ) or usage();

    # for $key (keys %opt)
    #     {
    #         print "$key => $opt{$key}\n";
    #}
    usage() if $opt{h};
    process_configure_ac( $opt{l} );
    process_top_makefile_am( $opt{l} );
    process_src_makefile_am( $opt{l}, $opt{a} );
    rename_src_files( $opt{a} );
    process_src_fr_c( $opt{n}, $opt{r}, $opt{a} );
    process_src_frprc_c( $opt{n}, $opt{r}, $opt{a} );
    process_src_frprc_decls_h( $opt{n}, $opt{r}, $opt{a} );
    process_src_frprc_h( $opt{n}, $opt{r}, $opt{a} );
    create_m4_folder();
    rename_folder( $opt{r} );

    return;
}

sub process_configure_ac {
    my $lib   = $_[0];
    my $file  = "template/configure.ac";
    my @lines = ();

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";
    foreach (@lines) {
        s/libtizfr/$lib/gm;
    }
    untie @lines;

    print "configure.ac : \t\tDone.\n";

    return;
}

sub process_top_makefile_am {
    print "Makefile.am : \t\tDone.\n";
    return;
}

sub process_src_makefile_am {
    my $lib    = $_[0];
    my $acr    = $_[1];
    my $file   = "template/src/Makefile.am";
    my @lines  = ();
    my $acrprc = $acr . "prc";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    foreach (@lines) {
        s/libtizfr/$lib/gm;
        s/frprc/$acrprc/gm;
        s/fr.c/$acr.c/gxm;
    }
    untie @lines;

    print "src/Makefile.am : \tDone.\n";
    return;
}

sub rename_src_files {
    my $acr      = $_[0];
    my $fr_c     = "template/src/fr.c";
    my $new_fr_c = "template/src/" . "$acr" . ".c";

    if ( -e $fr_c ) {
        rename $fr_c => $new_fr_c;
    }

    my $frprc_c     = "template/src/frprc.c";
    my $new_frprc_c = "template/src/" . "$acr" . "prc.c";

    if ( -e $frprc_c ) {
        rename $frprc_c => $new_frprc_c;
    }

    my $frprc_decls_h     = "template/src/frprc_decls.h";
    my $new_frprc_decls_h = "template/src/" . "$acr" . "prc_decls.h";

    if ( -e $frprc_decls_h ) {
        rename $frprc_decls_h => $new_frprc_decls_h;
    }

    my $frprc_h     = "template/src/frprc.h";
    my $new_frprc_h = "template/src/" . "$acr" . "prc.h";

    if ( -e $frprc_h ) {
        rename $frprc_h => $new_frprc_h;
    }

    print "src files rename : \tDone.\n";
    return;
}

sub process_src_fr_c {
    my $name = $_[0];
    my $role = $_[1];
    my $acr  = $_[2];
    my @data = ();

    @data = split( /\./xm, $role );
    my $category = $data[1];
    @data = split( /_/xm, $data[0] );
    $category = $category . "_" . "$data[1]";

    my $file = "template/src/" . "$acr" . ".c";

    my @lines = ();
    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    my $acrprc = $acr . "prc";

    my $category_uc = uc $category;

    foreach (@lines) {
        s/fr.c/$acr.c/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/File Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "c";
    print "$file : \t\tDone.\n";
    return;
}

sub process_src_frprc_c {
    my $name = $_[0];
    my $role = $_[1];
    my $acr  = $_[2];
    my @data = ();

    @data = split( /\./xm, $role );
    my $category = $data[1];
    @data = split( /_/xm, $data[0] );
    $category = $category . "_" . "$data[1]";

    my $file = "template/src/" . "$acr" . "prc.c";

    my @lines = ();
    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    my $acrprc      = $acr . "prc";
    my $category_uc = uc $category;
    my $fr_         = $acr . "_";

    foreach (@lines) {
        s/fr.c/$acr.c/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/File Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.c";
    print "$file : \t\tDone.\n";
    return;
}

sub process_src_frprc_decls_h {
    my $name = $_[0];
    my $role = $_[1];
    my $acr  = $_[2];
    my @data = ();

    @data = split( /\./xm, $role );
    my $category = $data[1];
    @data = split( /_/xm, $data[0] );
    $category = $category . "_" . "$data[1]";

    my $file = "template/src/" . "$acr" . "prc_decls.h";

    my @lines = ();
    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    my $acrprc      = $acr . "prc";
    my $acrprc_uc   = uc $acrprc;
    my $category_uc = uc $category;
    my $fr_         = $acr . "_";

    foreach (@lines) {
        s/fr.c/$acr.c/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/FRPRC/$acrprc_uc/gxm;
        s/File Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc_decls.h";
    print "$file : \t\tDone.\n";
    return;
}

sub process_src_frprc_h {
    my $name = $_[0];
    my $role = $_[1];
    my $acr  = $_[2];
    my @data = ();

    @data = split( /\./xm, $role );
    my $category = $data[1];
    @data = split( /_/xm, $data[0] );
    $category = $category . "_" . "$data[1]";

    my $file = "template/src/" . "$acr" . "prc.h";

    my @lines = ();
    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    my $acrprc      = $acr . "prc";
    my $acrprc_uc   = uc $acrprc;
    my $category_uc = uc $category;
    my $fr_         = $acr . "_";

    foreach (@lines) {
        s/fr.c/$acr.c/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/FRPRC/$acrprc_uc/gxm;
        s/File Reader/$name/gxm;
        s/File Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.h";
    print "$file : \t\tDone.\n";
    return;
}

sub rename_folder {
    my $role = $_[0];
    my @data = ();

    @data = split( /\./xm, $role );
    my $tech = $data[1];
    @data = split( /_/xm, $data[0] );
    my $new_folder_name = $tech . "_" . "$data[1]";

    if ( -e "template" ) {
        rename "template" => $new_folder_name;
    }

    print "Folder rename : \tDone.\n";
    return;
}

sub create_m4_folder {
    mkdir "template/m4", 0755;
    print "create m4 folder : \tDone.\n";
    return;
}

init();
