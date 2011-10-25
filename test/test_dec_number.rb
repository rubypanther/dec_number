require 'test/unit'
$: << File.join(File.dirname(__FILE__), '..', 'ext', 'dec_number')
require 'dec_number'

class TestDecNumber < Test::Unit::TestCase
  def test_silly_example
    assert_equal 1.to_dec_number, 1
  end
end
