#!/usr/bin/env ruby
require 'decimal'
require 'bigdecimal'
require 'dec_number'
require 'benchmark'
include Benchmark
n = 123.58132134
m = Math::PI
bm(6) do |x|
  x.report("dec_number") do
    a = n.to_dec_number
    b = m.to_dec_number
    100_000.times do
      ((a * b + DecNumber.new(rand().to_s) - b / a) ** b).to_s
    end
  end
  x.report("decimal") do
    a = Decimal.new n.to_s
    b = Decimal.new m.to_s
    100_000.times do
      ((a * b + Decimal.new(rand().to_s) - b / a) ** b).to_s
    end
  end
  x.report("bigdecimal") do
    a = BigDecimal.new n.to_s
    b = BigDecimal.new m.to_s
    100_000.times do
      (a * b + BigDecimal.new(rand().to_s) - b / a).to_s
    end
  end
  x.report("float") do
    a = n
    b = m
    100_000.times do
      ((a * b + rand() - b / a) ** b).to_s
    end
  end
end
