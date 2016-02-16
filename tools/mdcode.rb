#!/usr/bin/env ruby

cpp = false
while line = gets
  if cpp
    if /^~~~~$/ =~ line
      cpp = false
    else
      puts line
    end
  else
    if /^~~~~cpp$/ =~ line
      cpp = true
    end
  end
end
