#!/usr/bin/env ruby -w
#
# This was an attempt to use mkmfmf and core-source but I failed. Thought I'd throw it in anyways.
#
require "rubygems"
require 'mkmfmf'

decNumber_dir = './decNumber/'

if RUBY_VERSION >= "1.9"
  begin
    require "core-source"
  rescue LoadError
    require 'rubygems/user_interaction' # for 1.9.1
    require 'rubygems/dependency_installer'
    installer = Gem::DependencyInstaller.new
    installer.install 'core-source'

    Gem.refresh
    begin
      Specification.activate('core-source') # for 1.9.2-p290+
    rescue NameError
      Gem.activate('core-source') # 1.9.2-p180
    end

    require "core-source"
  end
end

Dir.chdir(decNumber_dir) do
  system('./configure --with-pic=yes') if %w! Makefile decNumber.o decContext.o libdecNumber.a !.find { |fn|not File.exists? fn }

  system("make") and
    system('ar rs libdecNumber.a decNumber.o decContext.o')
end

dir_config('dec_number', decNumber_dir, decNumber_dir)
have_library('decNumber','decNumberVersion','decNumber.h') or raise 'decNumber library not found :('

if RUBY_VERSION >= "1.9"
  $defs.push("-DRUBY19")
  ensure_core_headers %w"vm_core.h"
else
  $defs.push("-DRUBY18")
end

unless create_makefile('dec_number')
  STDERR.puts "\n\ncreate_makefile ate your cat!"
  exit(1)
end
