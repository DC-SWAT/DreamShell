require 'test/unit/assertions'

include Test::Unit::Assertions

def get_read_available()
  return @n_readable
end

def get_write_available()
  (MAX_BUFFERS - @n_readable - @n_busy)
end

def get_writable_buffer()
#  if @buffers[@wp] == :busy
#    # check if buffer is really busy
#    @buffers[@wp] = :empty
#  end

  if get_write_available > 0
    buffer = @wp
    @wp = (@wp + 1) % MAX_BUFFERS
    @n_readable += 1
    return buffer
  else
    return false
  end
end

def get_readable_buffer()
  if get_read_available > 0
    buffer = @rp
    @rp = (@rp + 1) % MAX_BUFFERS
    @n_readable -= 1
    return buffer
  else
    return false
  end
end

def show()
  puts "wp: #{@wp}"
  puts "rp: #{@rp}"
  puts "n_readable: #{@n_readable}"
  puts "n_busy: #{@n_busy}"
  puts "wa: #{get_write_available()}"
  puts "ra: #{get_read_available()}"
  puts
end

MAX_BUFFERS = 4
@buffers = [:empty] * MAX_BUFFERS
@wp = 0
@rp = 0
@n_readable = 0
@n_busy = 0

puts "START"

show

buffer = get_writable_buffer()
show

buffer = get_writable_buffer()
show

buffer = get_readable_buffer()
show

buffer = get_readable_buffer()
show

buffer = get_readable_buffer()
show
assert_equal(false, buffer)

buffer = get_writable_buffer()
show

buffer = get_writable_buffer()
show

buffer = get_writable_buffer()
show

buffer = get_writable_buffer()
show

buffer = get_writable_buffer()
show
assert_equal(false, buffer)

get_readable_buffer()
@n_busy = 1
show

get_readable_buffer()
@n_busy = 2
show

@n_busy = 0
show
