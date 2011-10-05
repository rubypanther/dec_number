#!/bin/sh

# rm decNumber/Makefile
ruby extconf.rb && make clean && make && ruby -I. -e 'require "dec_number"; DecNumber.new(1.23)'
