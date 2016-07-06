raise "arg plz" if ARGV.size <= 0

def start_thread(dirname)
  Thread.new do
    glob = "#{dirname}/**/*14x16.png"
    # puts "GLOB: #{glob}"

    Dir.glob(glob) do |filename|
      next if File.exists?(filename.gsub('14x16.png', '15x18.png'))
      next if filename.include?("_IGNORE_")

      command = %(bin/amazon-resize \"#{filename}\")
      puts "---> #{command}"
      system(command) ? puts("✓") : puts("×")
    end
  end
end

before = Time.now

threads = []
concurrency = ENV['THREADS'] || 5
concurrency = concurrency.to_i

puts "Concurrency: #{concurrency}"
puts "Operating in #{ARGV[0]}"

Dir.new(ARGV[0]).each do |name|
  dir = File.join(ARGV[0], name)

  next if name == ".." || name == "."
  next unless File.directory?(dir)

  while threads.size >= concurrency
    threads.select!(&:alive?)
    sleep 1
  end

  threads << start_thread(dir)
end
puts threads.size
threads.each(&:join)

after = Time.now

puts "|====================== DONE =====================|"
puts "Took #{after - before}.round(4) seconds"
