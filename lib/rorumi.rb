#!/usr/bin/env ruby

require 'rubygems'
require 'base64'
require 'json'
require 'fileutils'
require 'pathname'

READING_INTERVAL = 15
READING_TIMESPAN = 100
STRUCT_FMT = 'ddddd'
# TODO: why the shit can't I do this in ruby?
STRUCT_SZ  = `python -c 'import struct; print struct.calcsize("#{STRUCT_FMT}")'`.strip.to_i
DEV_DIR = File.join(ENV['HOME'], '.rorumi')

FileUtils.mkdir(DEV_DIR) rescue nil
raise unless File.exist?(DEV_DIR)

def store_location
  real_loc = Pathname.new(__FILE__).realpath
  require File.join(File.dirname(real_loc), 'lib/location_finder')
  username = `defaults read com.apple.Mail MailAccounts | grep ".*@me.com" | sed -e 's/.* //' -e 's/[";]//g' | head -n 1`.strip
  password = `security find-generic-password -ga #{username} 2>&1 | grep password | sed -e 's/.*://' -e 's/"//g'`.strip
  password = ''
  data = {}

  LocationFinder::FindMyIphone.find(username, password).each do |device|
    name = device['name']
    fn = File.join(DEV_DIR, Base64.encode64(device['name']).strip)

    File.open(fn + '.idx', 'a') do |index|
      File.open(fn + '.db', 'a') do |db|
        if device['location']
          # write data to db file
          db.seek(0, IO::SEEK_END)
          offset = db.tell
          db << Marshal.dump(device)
          size = db.tell - offset

          # create the struct record
          lat = device['location']['latitude']
          long = device['location']['longitude']
          time = device['location']['timeStamp'].to_f / 1000
          record = [offset, size, lat, long, time].pack(STRUCT_FMT)

          # tell index where the data is
          offset = index.tell
          index.seek(0, IO::SEEK_END)
          index << record
          size = index.tell - offset

          data[name] ||= []
          data[name] << { :lat => lat, :long => long, :time => Time.at(time) }
        end
      end
    end
  end

  data
end

def read_data
  data = {}
  dups = []

  Dir[File.join(DEV_DIR, '*.idx')].each do |device_index|
    name = Base64.decode64(device_index.sub(DEV_DIR + '/', '').sub('.idx', ''))
    device_db = device_index.sub('.idx', '.db')

    File.open(device_index, 'r') do |index|
      File.open(device_db, 'r') do |db|
        # read each record
        while record = index.read(STRUCT_SZ)
          #puts "#{device_index}: read #{STRUCT_SZ} bytes, now at #{index.tell}"
          offset, size, lat, long, time = record.unpack(STRUCT_FMT)

          # in case we want the serialised json
          #db.seek(offset, IO::SEEK_SET)
          #data = Marshal.load(db.read(size))

          unless dups.include?(time)
            dups << time
            data[name] ||= []
            data[name] << { :lat => lat, :long => long, :time => Time.at(time) }
          end
        end
      end
    end
  end

  puts JSON.pretty_generate(data)
end

if ARGV.length == 0
  data = store_location
  puts JSON.pretty_generate(data)
  data.keys.each do |device|
    url = "http://maps.google.com.au/?ll=#{data[device][0][:lat]},#{data[device][0][:long]}"
    puts "#{device}: #{url}"
  end
end

if ARGV.include?('-c')
  store_location
end

if ARGV.include?('-r')
  read_data
end
