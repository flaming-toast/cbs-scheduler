#!/usr/bin/ruby


file = '/home/user/cs194-24/malloc_output_1KB.txt'
f = File.open(file, 'r')

all_requested = []
all_padded = []

f.each_line do |line|
        line = line.strip.split(',')
        requested = line[0].split(': ')[1]
        padded = line[1].split(': ')[1]

        all_requested.push(requested.to_i)
        all_padded.push(padded.to_i)
end

puts "Min/Max requested size: #{all_requested.minmax}"
puts "Average requested size: #{all_requested.inject { |sum, el| sum + el }.to_f / all_requested.size}"
puts "Highest frequency value requested: #{}"
puts "Min/Max padded size: #{all_padded.minmax}"
puts "Average padded size: #{all_padded.inject { |sum, el| sum + el }.to_f / all_padded.size}"
