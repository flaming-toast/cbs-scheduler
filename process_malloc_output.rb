#!/usr/bin/ruby

def main
        puts "*================================================*"
        puts "Test for 1KB output"
        process('/home/user/cs194-24/malloc_output_1KB.txt')
        puts "*================================================*"
        puts "Test for 100KB output"
        process('/home/user/cs194-24/malloc_output_100KB.txt')
        puts "*================================================*"
        puts "Test for 500KB output"
        process('/home/user/cs194-24/malloc_output_500KB.txt')
        puts "*================================================*"
end

def process(file)
        f = File.open(file, 'r')

        all_requested = []
        all_padded = []

        f.each_line do |line|
                if line.include?('requested size: ') and line.include?('new size: ')
                        line = line.strip.split(',')
                        requested = line[0].split(': ')[1]
                        padded = line[1].split(': ')[1]

                        all_requested.push(requested.to_i)
                        all_padded.push(padded.to_i)
                end
        end

        puts "Min/Max requested size: #{all_requested.minmax}"
        puts "Average requested size: #{all_requested.inject { |sum, el| sum + el }.to_f / all_requested.size}"
        puts "Highest frequency value requested: #{mode(all_requested)}"
        puts "Median value requested: #{median(all_requested)}"
        puts "-----------------------------------------------"
        puts "Min/Max padded size: #{all_padded.minmax}"
        puts "Average padded size: #{all_padded.inject { |sum, el| sum + el }.to_f / all_padded.size}"
        puts "Highest frequency value padded: #{mode(all_padded)}"
        puts "Median value padded: #{median(all_padded)}"

        f.close
end

def median(array)
        sorted = array.sort
        len = sorted.length
        return (sorted[(len - 1) / 2] + sorted[len / 2]) / 2.0
end

def mode(array)
        mode = {}
        array.each do |val|
                if mode[val].nil?
                        mode[val] = 1
                else
                        mode[val] += 1
                end
        end
        return mode.key(mode.values.max)
end

main



