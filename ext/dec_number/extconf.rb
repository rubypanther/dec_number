#!/usr/bin/env ruby -w
require 'mkmf'

decNumber_dir = './decNumber/'

if RUBY_VERSION >= "1.9"
  begin
    require "ruby_core_source"
  rescue LoadError
    require 'rubygems/user_interaction' # for 1.9.1
    require 'rubygems/dependency_installer'
    installer = Gem::DependencyInstaller.new
    installer.install 'ruby_core_source'

    Gem.refresh
    Gem::Specification.find_by_name('ruby_core_source').activate # for 1.9.1

    require "ruby_core_source"
  end
end

Dir.chdir(decNumber_dir) do
  system('./configure --with-pic=yes') if %w! Makefile decNumber.o decContext.o libdecNumber.a !.find { |fn|not File.exists? fn }

  system("make") and
    system('ar rs libdecNumber.a decNumber.o decContext.o')
end

$LIBPATH << Dir.pwd
$defs += ["-I#{decNumber_dir}"]
dir_config('dec_number', decNumber_dir, decNumber_dir)
have_library('decNumber','decNumberVersion','decNumber.h') or raise 'decNumber library not found :('

if RUBY_VERSION >= "1.9"
  $defs.push("-DRUBY19")
  hdrs = Proc.new do
    have_header("vm_core.h")
  end
  dir_config("ruby")
  unless Ruby_core_source::create_makefile_with_core(hdrs, "dec_number")
    STDERR.puts "\n\nRuby_core_source::create_makefile_with_core ate your cat!"
    exit(1)
  end

else
  $defs.push("-DRUBY18")
  unless create_makefile('dec_number')
    STDERR.puts "\n\ncreate_makefile ate your cat!"
    exit(1)
  end
end

