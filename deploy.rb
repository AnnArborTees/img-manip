#!/usr/bin/ruby
require 'json'
require 'byebug'

def color(c)
  s = "\033[#{c}m"
  s += yield
  s += "\033[39m\033[49m"
  s
end

#
# Make sure AWS CLI is installed
#

if `which aws`.empty?
  puts "You'll need the AWS CLI (https://aws.amazon.com/cli/) to deploy"
  exit 1
end

#
# Find instances to deploy to
#
instances = JSON.parse(`aws --output json ec2 describe-instances`)['Reservations'].flat_map { |r| r['Instances'] }

should_deploy = proc do |inst|
  ip = inst['PublicIpAddress']
  next if !ip || ip.empty?

  inst['Tags'].any? do |tag|
    tag['Key'] == 'Roles' && tag['Value'].split(',').include?('mockbot')
  end
end
deploy_to = instances.select(&should_deploy)

if deploy_to.empty?
  puts "No AWS instances with the \"mockbot\" role were found."
  exit(2)
end

#
# Need github creds unfortunately
#
puts "This currently requires GitHub account credentials to pull the repository"

$stdout.write "GitHub Username: "
github_username = $stdin.gets.chomp

$stdout.write "GitHub Password: "
begin
  system "stty -echo > /dev/null 2> /dev/null"
  github_password = $stdin.gets.chomp
ensure
  system "stty echo > /dev/null 2> /dev/null"
  $stdout.puts ''
end

#
# Deploy each instance
#
deploy_to.each do |inst|
  begin
    ip_address = inst['PublicIpAddress']

    # The remote username is assumed to be ubuntu
    command = proc do |pw|
      pw = pw.gsub('$', "\\\\\\$")
      "ssh ubuntu@#{ip_address} \""\
      "cd img-manip && "\
      "git pull https://#{github_username}:#{pw}@github.com/AnnArborTees/img-manip && "\
      "sh update.sh"\
      "\""
    end

    puts "Executing `#{color(34) { command['*********']} }`"
    success = system command[github_password]

    if success
      puts color(32) { "DONE! #{ip_address}\n" }
    else
      puts color(31) { "FAILED! #{ip_address}\n" }
    end

  rescue StandardError => e
    $stderr.puts "Error while deploying on #{ip_address}:"
    $stderr.puts "#{e.class.name} #{e.message}"
    $stderr.puts e.backtrace
    $stderr.puts ''
  end
end
