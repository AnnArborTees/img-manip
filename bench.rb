MOCKBOT_ADD = "bin/mockbot test/blankshirt.png test/tcb_spies14x16.png 409 192 665 760 over test/out1.png"
MAGICK_ADD  = "convert -composite test/blankshirt.png test/tcb_spies14x16.png -alpha on -geometry 665x760+409+192 -depth 8 test/out.png"

MOCKBOT_MULT = "bin/mockbot test/blankshirt.png test/tcb_spies14x16.png 409 192 665 760 multiply test/out1.png"
MAGICK_MULT  = "convert -composite test/blankshirt.png test/tcb_spies14x16.png -compose multiply -alpha on -geometry 665x760+409+192 -depth 8 test/out.png"

ITERATIONS = 10.0

def bench(cmd)
  before = Time.now
  system(cmd)
  after = Time.now

  after - before
end

mockbot_add_total = 0.0
magick_add_total = 0.0
mockbot_mult_total = 0.0
magick_mult_total = 0.0

(0...ITERATIONS).each do |i|
  mockbot_add_total += bench(MOCKBOT_ADD)
  magick_add_total += bench(MAGICK_ADD)

  mockbot_mult_total += bench(MOCKBOT_MULT)
  magick_mult_total += bench(MAGICK_MULT)
end

mockbot_add_avg = mockbot_add_total / ITERATIONS;
magick_add_avg = magick_add_total / ITERATIONS;

mockbot_mult_avg = mockbot_mult_total / ITERATIONS;
magick_mult_avg = magick_mult_total / ITERATIONS;

puts "ADDITIVE:"
puts "=== MockBot:     #{mockbot_add_avg} seconds"
puts "=== ImageMagick: #{magick_add_avg} seconds"

puts "MULTIPLICATIVE:"
puts "=== MockBot:     #{mockbot_mult_avg} seconds"
puts "=== ImageMagick: #{magick_mult_avg} seconds"

puts "TOTAL:"
puts "=== MockBot:     #{(mockbot_add_avg + mockbot_mult_avg) / 2.0} seconds"
puts "=== ImageMagick: #{(magick_add_avg + magick_mult_avg) / 2.0} seconds"
