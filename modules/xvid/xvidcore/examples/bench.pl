#!/usr/bin/perl

###############################################################################
# 
#   XVID MPEG-4 VIDEO CODEC
#   - Unit tests and benches -
# 
#   Copyright(C) 2005 Pascal Massimino <skal@planet-d.net>
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
# 
# ***************************************************************************/
#
#  Automated bench script.
#
#  1st draft: Skal / April 24th 2005
#
######################################################

$enc_bin   = "xvid_encraw";
$dec_bin   = "xvid_decraw";
$bench_bin = "xvid_bench";

if (-r "BENCH_CONF.pl") {
  require "BENCH_CONF.pl";
}
else
{
  $bin_dir = ".";
  $log_dir = ".";
  $data_dir = ".";
  $bench_list = "./bench_list.pl";
}

require $bench_list;

#########################################

$arch = "";
$verbose = 0;
$use_valgrind = 0;
$output_command_only = 0;
$extra_arg = "";

#$log_name;
#$log_file;
#$input_dir;
#$bin;
#$bitstream;
#$refstream;
#@raw_options;
#@ToDo;
#$filter

#########################################
# helper funcs
#########################################

sub check_bin
{
  # force re-build of binary (better safe than sorry)
  my_system( "rm -f $_[0]" );
  my_system( "make $_[0]" ); 
}
sub check_file
{
  if (-r $_[0]) {
    my_system( "mv $_[0] $_[0]\_OLD" );
    my_system( "touch $_[0]" );
  }
}

sub setup
{
  my $action = $_[0];
  $log_name  = $_[1];
  $log_file  = "$log_dir/$log_name";
  $input_dir = "$data_dir/$_[2]";
  $bin       = $_[3];

  check_bin $bin;
  check_file $log_file;

  printf "\nBench ...... $action\n";
  printf "Binary ..... $bin $extra_arg\n";
  printf "Log ........ $log_name\n";
  printf "Data dir ... $input_dir\n\n";

  $bin = "$bin_dir/$bin $extra_arg";
  if ($use_valgrind)
  {
    $bin = "valgrind --tool=memcheck $bin";
    $arch = "";
  }
  if (not $arch eq "") { $bin .= "-$arch"; }
}

sub parse_bench
{
  @raw_options = split;

  $bitstream = $raw_options[0];
  if (defined($bench_filter) && !($bitstream =~ /(.*)$bench_filter(.*)/))
  {
    printf "Filtering out bitstream [$bitstream]\n" if ($verbose>1);
    return 0;
  }
  shift @raw_options;

  printf "Checking bitstream: $bitstream [@raw_options]\n" if ($verbose>0);

  return 1;
}

#########################################
# debug
#########################################

sub my_system
{
  if ($output_command_only) { printf "system: $_[0]\n"; }
  else                      { system($_[0]); }
  return ($? >> 8);
}

#########################################
## decoding benches
#########################################

sub Do_Dec_Benches
{
  setup( "decoding", "DEC_LOG", "./data_dec", $bench_bin );
  my $n = 0, $Err = 0;
  foreach (@Dec_Benches)
  {
    if (parse_bench($_, 0))
    {
      $n++;
      my $launch = "$bin 9 $input_dir/$bitstream @raw_options";
      my $output = `$launch`;
      if (open( LOG_FILE, ">$log_file" )) {
        print LOG_FILE $output;
        close LOG_FILE;
      }
      else { printf "can't open core bench log file '$log_file'\n"; };
      if ($output =~ /ERROR/) {
        print "ERROR detected in ouput, while decoding [$bitstream]:\n   $output\n";
        $Err++;
        next;
      }
      if ($output =~ /FPS:(.*) Checksum*/) { print "[$bitstream]: $1 fps\n"; }
    }
  }
  die "*** $Err error(s) detected!!\n" if  $Err;
}

#########################################
## Core benches
#########################################

sub Do_Core_Benches
{
  setup( "Core benches", "CORE_BENCH_LOG", ".", $bench_bin );
  my $output = `$bin`;
  if (open( LOG_FILE, ">$log_file" )) {
    print LOG_FILE $output;
    close LOG_FILE;
  }
  else { printf "can't open core bench log file '$log_file'\n"; };
  
  
  my $warning = 0;
  foreach (split('\n', $output))
  {
    if (/ERROR/) {
      print "ERROR detected in ouput: [$_]\n";
      if (/quant_mpeg/) {
        if (!$warning) {
          print "\n";
          print "NB: MMX mpeg4 quantization is known to have very small errors (+/-1 magnitude)\n";
          print "for 1 or 2 coefficients a block. This is mainly caused by the fact the unit\n";
          print "test goes far behind the usual limits of real encoding. Please do not report\n";
          print "this error to the developers.\n";
          print "\n";
          $warning = 1;
        }
      }
    }
  }
}

#########################################
## Help
#########################################

sub Do_Help
{
  printf "\n         -= Options =-\n\n";
  printf "-h ................... this help\n";
  printf "-v ................... Verbose++\n";
  printf "-n ................... Check 'system' commands (no action performed)\n";
  printf "-vlg ................. Use valgrind\n";
  printf "-dec ................. perform decoding benches.\n";
  printf "-core ................ perform core benches (using 'xvid_bench').\n";
  printf "-all ................. perform all benches\n";
  printf "-cpu <CPU> ........... CPU to select (one of [c|mmx|mmxext|sse2|3dnow|3dnowe|altivec]).\n";
  printf "-extra <arg> ......... Append extra argument 'arg' to binary commands.\n";
  printf "\n";
  exit;
}

#########################################
##               main                  ##
#########################################

while(@ARGV)
{
  my $command = shift @ARGV;
  if    ($command eq "-h") { Do_Help; }
  elsif ($command eq "-v") { $verbose++; }
  elsif ($command eq "-n") { $output_command_only = 1; }
  elsif ($command eq "-vlg")  { $use_valgrind = 1;  }
  elsif ($command eq "-dec")  { push @ToDo, $command; }
  elsif ($command eq "-core") { push @ToDo, $command; }
  elsif ($command eq "-all")  { push @ToDo, ("-dec", "-core"); }
  elsif ($command eq "-cpu")
  {
    die "missing argument after $option\n" if (!defined($ARGV[0]));
    if ($ARGV[0] eq "c" || $ARGV[0] eq "mmx" || $ARGV[0] eq "mmxext" || $ARGV[0] eq "sse2" || $ARGV[0] eq "3dnow"
      || $ARGV[0] eq "3dnowe" || $ARGV[0] eq "altivec")
    { 
      $arch = shift @ARGV;
    }
    else {
      die "Unrecognized cpu option '$ARGV[0]'.\n";
    }
  }
  elsif ($command eq "-extra")
  {
    die "missing argument after $option\n" if (!defined($ARGV[0]));
    $extra_arg = "$extra_arg $ARGV[0]";
    shift @ARGV;
  }
  elsif ($command =~ /^\-/)
  {
    printf "Unrecognized option [$command]\n";
    Do_Help;
  }
  else { $bench_filter = $command; }
}

if (@ToDo==0) { push @ToDo, "help"; }

printf "Filtering bench name with [$bench_filter]\n" if ($verbose>2 && defined($bench_filter));

foreach (@ToDo) {
  if    ($_ eq "help" ) { Do_Help; }
  elsif ($_ eq "-dec")  { Do_Dec_Benches; }
  elsif ($_ eq "-core") { Do_Core_Benches; }
}
  
#########################################
