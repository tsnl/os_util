# cpp_base configuration: do not edit this section.
MAKEFLAGS+=--always-make 	# we use bazel as the build system, so we want to always build. Equal to '.PHONY: *'.
SHELL=/bin/bash
.SHELLFLAGS="-O extglob -c"

# repo configuration:
# ...

#
# cpp_base targets: do not edit this section.
#

default: test

compile_commands.json:
	bazelisk run refresh_compile_commands

clean:
	bazelisk clean --expunge

format:
	bash "script/clang_format.sh"

#
# targets:
#
