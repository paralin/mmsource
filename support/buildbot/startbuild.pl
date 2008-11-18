#!/usr/bin/perl

use File::Basename;

our (@LIBRARIES);
my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

#Get to top of source tree
chdir('..');
chdir('..');

#	   Folder			.vcproj				Engine			Binary				Suffix type
Build('loader', 		'mm_loader', 		'', 			'server', 			'full');
Build('core-legacy',	'mm_core-legacy', 	'', 			'metamod.1.ep1', 	'');
Build('core', 			'mm_core', 			'OrangeBox', 	'metamod.2.ep2', 	'');
Build('core', 			'mm_core', 			'Left4Dead', 	'metamod.2.l4d', 	'');

#Structure our output folder
mkdir('OUTPUT');
mkdir(Build::PathFormat('OUTPUT/addons'));
mkdir(Build::PathFormat('OUTPUT/addons/metamod'));
mkdir(Build::PathFormat('OUTPUT/addons/metamod/bin'));

my ($i);
for ($i = 0; $i <= $#LIBRARIES; $i++)
{
	my $library = $LIBRARIES[$i];
	Copy($library, Build::PathFormat('OUTPUT/addons/metamod/bin'));
}
Copy(Build::PathFormat('support/metaplugins.ini'),
	 Build::PathFormat('OUTPUT/addons/metamod'));
Copy(Build::PathFormat('support/README.txt'),
	 Build::PathFormat('OUTPUT/addons/metamod'));

sub Copy
{
	my ($a, $b) = (@_);

	die "Could not copy $a to $b!\n" if (!Build::Copy($a, $b));
}

sub Build
{
	my ($srcdir, $vcproj, $objdir, $binary, $suffix) = (@_);

	if ($^O eq "linux")
	{
		if ($suffix eq 'full')
		{
			$binary .= '_i486.so';
		}
		else
		{
			$binary .= '.so';
		}
		BuildLinux($srcdir, $objdir, $binary);
	}
	else
	{
		$binary .= '.dll';
		BuildWindows($srcdir, $vcproj, $objdir, $binary);
	}
}

sub BuildWindows
{
	my ($srcdir, $vcproj, $build, $binary) = (@));
	my ($dir, $file, $param, $vcbuilder, $cmd);

	$dir = getcwd();
	chdir($srcdir);

	$param = "Release";
	if ($build eq "Orange Box")
	{
		$param = "Release - Orange Box";
	}
	elsif ($build eq "Left4Dead")
	{
		$param = "Release - Left 4 Dead";
	}

	print "Clean building $srcdir...\n";
	$vcbuilder = $ENV{'VC9BUILDER'};
	$cmd = "\"$vcbuilder\" /rebuild \"$vcproj.vcproj\" \"$param\"";
	print "$cmd\n";
	system($cmd);
	CheckFailure();

	$file = "$param\\$binary";

	die "Output library not found: $file\n" if (!-f $file);

	chdir($dir);

	push(@LIBRARIES, "$srcdir\\$file");
}

sub BuildLinux
{
	my ($srcdir, $build, $binary) = (@_);
	my ($dir, $file, $param);

	$dir = getcwd();
	chdir($srcdir);

	$param = "";
	$file = "Release";
	if ($build eq "OrangeBox")
	{
		$param = "ENGINE=orangebox";
		$file .= '.orangebox';
	}
	elsif ($build eq "Left4Dead")
	{
		$param = "ENGINE=left4dead";
		$file .= '.left4dead';
	}
	$file .= '/' . $binary;

	print "Cleaning $srcdir...\n";
	system("make $param clean");
	CheckFailure();

	print "Building $srcdir for $binary...\n";
	print "$param\n";
	system("make $param");
	CheckFailure();

	die "Output library not found: $file\n" if (!-f $file);

	chdir($dir);

	push(@LIBRARIES, $srcdir . '/' . $file);
}

sub CheckFailure
{
	die "Build failed: $!\n" if $? == -1;
	die "Build died :(\n" if $^O eq "linux" and $? & 127;
	die "Build failed with exit code: " . ($? >> 8) . "\n" if ($? >> 8 != 0);
}

