require 'test/unit'
$: << File.join(File.dirname(__FILE__), '..', 'ext', 'dec_number')
require 'dec_number'
require 'bigdecimal'

class TestDecNumber < Test::Unit::TestCase
  def test_simple_equality
    data = [ 0, 1, Math::PI, BigDecimal.new("123.5813") ]
    data.each do |n|
      d = n.to_dec_number
      assert_equal d, n
    end
  end

  [ ["addition", :+ ],
    ["subtraction", :- ]
  ].each do |name,op|
    define_method "test_#{name}" do
      data = [ 0, 1, Math::PI, BigDecimal.new("123.5813") ]
      data.each do |n|
        d = n.to_dec_number
        assert_equal d.send(op,d), n.send(op,n)
      end
    end
  end

end
