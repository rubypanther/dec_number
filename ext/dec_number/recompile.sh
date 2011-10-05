rm -f dec_number.o dec_number.so
gcc -fPIC -g -O2 -I/home/paris/.rvm/rubies/ruby-1.9.2-p290/include/ruby-1.9.1/x86_64-linux -I/home/paris/.rvm/rubies/ruby-1.9.2-p290/include/ruby-1.9.1/ -IdecNumber -c dec_number.c
gcc -shared -o dec_number.so dec_number.o decNumber.o decContext.o -lc
ruby -I. -e 'require "dec_number"; DecNumber.new(1.23)'
