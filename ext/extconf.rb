require 'mkmf'

dir_config('peg_markdown')

$objs = %w[markdown.o markdown_lib.o markdown_output.o markdown_parser.o]

unless pkg_config('glib-2.0')
  fail "glib2 not found"
end

create_header
create_makefile('peg_markdown')
