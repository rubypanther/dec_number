Gem::Specification.new do |s| # -*-ruby-*-
  s.name        = "dec_number"
  s.version     = '0.1.2'
  s.authors     = ["Paris Sinclair"]
  s.email       = ["paris@rubypanther.com"]
  s.summary     = "ICU-decNumber wrapper for arbitrary precision math with context objects"
  s.description = "Wrapper for ICU-decNumber library"
  s.homepage    = "http://github.com/rubypanther/dec_number"
  s.extensions << 'ext/dec_number/extconf.rb'
  s.rubyforge_project = "dec-number"
  s.files = Dir['**/**']
  s.test_files  = Dir.glob("test/**/*.rb")
end
