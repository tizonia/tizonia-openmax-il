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

use Tie::File;
use Getopt::Long;
use vars qw/ %opt /;

sub usage()
{
    print ("\nCopyright (C) 2012 Aratelia Limited - Juan A. Rubio\n");
    print ("Creation of skeleton components from a template.\n");
    print "\nUsage: $0 -l <library_name> -n <comp_name> -r <comp_role> -a <component_acronym>\n\n";
    print "\nE.g.: perl $0 -l libtizvp8d -n \"VP8 Decoder\" -r video_decoder.vp8 -a vp8d\n\n";
    print "\nE.g.: perl $0 -l libtizsdlivr -n \"SDL Video Renderer\" -r iv_renderer.yuv.overlay -a sdlivr\n\n";
    exit;
}

sub init()
{
    use Getopt::Std;
    if (!@ARGV || 8 != $#ARGV+1)
    {
        usage();
    }
    $opt_string = 'l:n:r:a:';
    getopts( "$opt_string", \%opt ) or usage();
    # for $key (keys %opt)
#     {
#         print "$key => $opt{$key}\n";
    #}
    usage() if $opt{h};
    process_configure_ac($opt{l});
    process_top_makefile_am($opt{l});
    process_src_makefile_am($opt{l}, $opt{a});
    rename_src_files($opt{a});
    process_src_fr_c($opt{n}, $opt{r}, $opt{a});
    process_src_frprc_c($opt{n}, $opt{r}, $opt{a});
    process_src_frprc_defs_h($opt{n}, $opt{r}, $opt{a});
    process_src_frprc_h($opt{n}, $opt{r}, $opt{a});
    create_m4_folder();
    rename_folder($opt{r});
}

sub process_configure_ac
{
    $lib = $_[0];

    $file = "template/configure.ac";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";
    foreach ( @lines )
    {
        s/libtizfr/$lib/g;
    }
    untie @lines;

    print "configure.ac : \t\tDone.\n";
}

sub process_top_makefile_am
{
    print "Makefile.am : \t\tDone.\n";
}

sub process_src_makefile_am
{
    $lib = $_[0];
    $acr = $_[1];

    $file = "template/src/Makefile.am";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    $acrprc = $acr . "prc";

    foreach ( @lines )
    {
        s/libtizfr/$lib/g;
        s/frprc/$acrprc/g;
        s/fr.c/$acr.c/g;
    }
    untie @lines;

    print "src/Makefile.am : \tDone.\n";
}

sub rename_src_files
{
    $acr = $_[0];

    $fr_c = "template/src/fr.c";
    $new_fr_c = "template/src/" . "$acr" . ".c";

    if (-e $fr_c)
    {
        rename $fr_c => $new_fr_c;
    }

    $frprc_c = "template/src/frprc.c";
    $new_frprc_c = "template/src/" . "$acr" . "prc.c";

    if (-e $frprc_c)
    {
        rename $frprc_c => $new_frprc_c;
    }

    $frprc_defs_h = "template/src/frprc_defs.h";
    $new_frprc_defs_h = "template/src/" . "$acr" . "prc_defs.h";

    if (-e $frprc_defs_h)
    {
        rename $frprc_defs_h => $new_frprc_defs_h;
    }

    $frprc_h = "template/src/frprc.h";
    $new_frprc_h = "template/src/" . "$acr" . "prc.h";

    if (-e $frprc_h)
    {
        rename $frprc_h => $new_frprc_h;
    }

    print "src files rename : \tDone.\n";
}

sub process_src_fr_c
{
    $name = $_[0];
    $role = $_[1];
    $acr  = $_[2];

    @data = split(/\./, $role);
    $category = $data[1];
    @data = split(/_/, $data[0]);
    $category = $category . "_" . "$data[1]";

    $file = "template/src/" . "$acr" . ".c";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    $acrprc = $acr . "prc";

    $category_uc = uc $category;

    foreach ( @lines )
    {
        s/fr.c/$acr.c/g;
        s/frprc/$acrprc/g;
        s/file_reader.binary/$role/g;
        s/file_reader/$category/g;
        s/FILE_READER/$category_uc/g;
        s/File Reader/$name/g;
    }
    untie @lines;

    $file = "src/" . "$acr" . "c";
    print "$file : \t\tDone.\n";
}

sub process_src_frprc_c
{
    $name = $_[0];
    $role = $_[1];
    $acr  = $_[2];

    @data = split(/\./, $role);
    $category = $data[1];
    @data = split(/_/, $data[0]);
    $category = $category . "_" . "$data[1]";

    $file = "template/src/" . "$acr" . "prc.c";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    $acrprc = $acr . "prc";
    $category_uc = uc $category;
    $fr_ = $acr . "_";

    foreach ( @lines )
    {
        s/fr.c/$acr.c/g;
        s/fr_/$fr_/g;
        s/frprc/$acrprc/g;
        s/file_reader.binary/$role/g;
        s/file_reader/$category/g;
        s/FILE_READER/$category_uc/g;
        s/File Reader/$name/g;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.c";
    print "$file : \t\tDone.\n";
}

sub process_src_frprc_defs_h
{
    $name = $_[0];
    $role = $_[1];
    $acr  = $_[2];

    @data = split(/\./, $role);
    $category = $data[1];
    @data = split(/_/, $data[0]);
    $category = $category . "_" . "$data[1]";

    $file = "template/src/" . "$acr" . "prc_defs.h";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    $acrprc = $acr . "prc";
    $acrprc_uc = uc $acrprc;
    $category_uc = uc $category;
    $fr_ = $acr . "_";

    foreach ( @lines )
    {
        s/fr.c/$acr.c/g;
        s/fr_/$fr_/g;
        s/frprc/$acrprc/g;
        s/file_reader.binary/$role/g;
        s/file_reader/$category/g;
        s/FILE_READER/$category_uc/g;
        s/FRPRC/$acrprc_uc/g;
        s/File Reader/$name/g;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc_defs.h";
    print "$file : \t\tDone.\n";
}

sub process_src_frprc_h
{
    $name = $_[0];
    $role = $_[1];
    $acr  = $_[2];

    @data = split(/\./, $role);
    $category = $data[1];
    @data = split(/_/, $data[0]);
    $category = $category . "_" . "$data[1]";

    $file = "template/src/" . "$acr" . "prc.h";

    tie @lines, 'Tie::File', "$file" or die "Can't read file: $!\n";

    $acrprc = $acr . "prc";
    $acrprc_uc = uc $acrprc;
    $category_uc = uc $category;
    $fr_ = $acr . "_";

    foreach ( @lines )
    {
        s/fr.c/$acr.c/g;
        s/fr_/$fr_/g;
        s/frprc/$acrprc/g;
        s/file_reader.binary/$role/g;
        s/file_reader/$category/g;
        s/FILE_READER/$category_uc/g;
        s/FRPRC/$acrprc_uc/g;
        s/File Reader/$name/g;
        s/File Reader/$name/g;
    }
    untie @lines;

    $file = "src/" . "$acr" . "prc.h";
    print "$file : \t\tDone.\n";
}

sub rename_folder
{
    $role = $_[0];

    @data = split(/\./, $role);
    $tech = $data[1];
    @data = split(/_/, $data[0]);
    $new_folder_name = $tech . "_" . "$data[1]";

    if (-e "template")
    {
        rename "template" => $new_folder_name;
    }

    print "Folder rename : \tDone.\n";
}

sub create_m4_folder
{
    mkdir "template/m4", 0755;
    print "create m4 folder : \tDone.\n";
}

init();
