#!/usr/bin/perl -w
#
# Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
use Term::ANSIColor;
use Term::ANSIColor qw(:constants);

local $Term::ANSIColor::AUTORESET = 1;
our $VERSION = qv('0.1.0');

sub usage() {
    print BOLD GREEN, "Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors\n";
    print BOLD BRIGHT_RED, "Produce skeleton components from a template.\n";
    print WHITE, "Usage: $0 -l <library_name> -n <comp_name> -r <comp_role> -a <component_acronym>\n";
    print BRIGHT_YELLOW, "Example : cp -R filter template && perl $0 -l libtizvp8d -n \"VP8 Decoder\" -r video_decoder.vp8 -a vp8d\n";
    print BRIGHT_YELLOW, "Example : cp -R filter template && perl $0 -l libtizsdlivr -n \"SDL Video Renderer\" -r iv_renderer.yuv.overlay -a sdlivr\n";
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
    print BOLD RED, "Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors\n";
    print BOLD WHITE, "Creating skeleton component: $opt{n}\n";
    if (-d "template") {
        process_configure_ac( $opt{l} );
        process_top_makefile_am( $opt{l} );
        process_src_makefile_am( $opt{l}, $opt{a} );
        rename_src_files( $opt{a} );
        process_src_fr_c( $opt{n}, $opt{r}, $opt{a} );
        process_src_fr_h( $opt{n}, $opt{r}, $opt{a} );
        process_src_frprc_c( $opt{n}, $opt{r}, $opt{a} );
        process_src_frprc_decls_h( $opt{n}, $opt{r}, $opt{a} );
        process_src_frprc_h( $opt{n}, $opt{r}, $opt{a} );
        create_m4_folder();
        rename_folder( $opt{r} );
        print BOLD YELLOW, "The skeleton component is ready.\n";
    }
    else {
        print BOLD BRIGHT_RED, "Error: directory './template' not found. ",
        "Please provide a 'template' folder by copying one of ",
        "the existing template folders.\n";
    }

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

    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " configure.ac\n";

    return;
}

sub process_top_makefile_am {
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " Makefile.am\n";
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
        s/fr.h/$acr.h/gxm;
    }
    untie @lines;

    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " src/Makefile.am\n";
    return;
}

sub rename_src_files {
    my $acr      = $_[0];
    my $fr_c     = "template/src/fr.c";
    my $new_fr_c = "template/src/" . "$acr" . ".c";

    if ( -e $fr_c ) {
        rename $fr_c => $new_fr_c;
    }

    my $fr_h     = "template/src/fr.h";
    my $new_fr_h = "template/src/" . "$acr" . ".h";

    if ( -e $fr_h ) {
        rename $fr_h => $new_fr_h;
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

    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " src files rename\n";
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

    my $acrprc      = $acr . "prc";
    my $category_uc = uc $category;
    my $fr_         = $acr . "_";

    foreach (@lines) {
        s/fr.h/$acr.h/gxm;
        s/fr.c/$acr.c/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/File\ Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . ".c";
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " $file\n";
    return;
}

sub process_src_fr_h {
    my $name = $_[0];
    my $role = $_[1];
    my $acr  = $_[2];
    my @data = ();

    @data = split( /\./xm, $role );
    my $category = $data[1];
    @data = split( /_/xm, $data[0] );
    $category = $category . "_" . "$data[1]";

    my $file = "template/src/" . "$acr" . ".h";

    my @lines = ();
    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    my $acr_uc      = uc $acr;
    my $acrprc      = $acr . "prc";
    my $category_uc = uc $category;
    my $fr_         = $acr . "_";

    foreach (@lines) {
        s/fr.h/$acr.h/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/FR/$acr_uc/gxm;
        s/File\ Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . ".h";
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " $file\n";
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
        s/fr.h/$acr.h/gxm;
        s/fr.c/$acr.c/gxm;
        s/fr_/$fr_/gxm;
        s/frprc/$acrprc/gxm;
        s/file_reader.binary/$role/gxm;
        s/file_reader/$category/gxm;
        s/FILE_READER/$category_uc/gxm;
        s/File\ Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.c";
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " $file\n";
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
        s/File\ Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc_decls.h";
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " $file\n";
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
        s/File\ Reader/$name/gxm;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.h";
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " $file\n";
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

    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " Folder rename\n";
    return;
}

sub create_m4_folder {
    mkdir "template/m4", 0755;
    print BOLD GREEN, "[DONE]", BOLD BRIGHT_BLUE, " create m4 folder\n";
    return;
}

init();
