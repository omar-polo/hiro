project('hiro', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

add_global_arguments('-Wno-unused-parameter', language : 'c')

event = dependency('libevent')
openssl = dependency('openssl')

compat = []

cc = meson.get_compiler('c')

functions = [['arc4random', ['compat/arc4random.c'], 'HAVE_ARC4RANDOM', 'arc4random.h'],
             ['err', ['compat/err.c'], 'HAVE_ERR', 'err.h'],
             ['strtonum', ['compat/strtonum.c'], 'HAVE_STRTONUM', 'strtonum.h']]
foreach f : functions
	t = cc.has_function(f[0])
	conf = configuration_data()
	conf.set(f[2], t)
	configure_file(input : 'compat/' + f[3] + '.in',
	               output : f[3],
		       configuration : conf)
	if not t
		compat += f[1]
	endif
endforeach

conf_data = configuration_data()
conf_data.set('version', '0.1')
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

queue_compat = configuration_data()
queue_compat.set('HAVE_SYS_QUEUE_H', cc.has_header('sys/queue.h'))
configure_file(input : 'compat/queue.h.in',
               output : 'queue.h',
	       configuration : queue_compat)

executables = [['hirod',
                ['hirod.c', 'cmd.c', 'util.c', 'log.c', 'can.c'],
                [openssl, event]],
               ['hiroctl',
                ['hiroctl.c', 'cmd.c', 'util.c'],
                [openssl, event]]] # TODO: drop arc4random.c as compat
                                   # for this so we can avoid linking openssl
foreach e : executables
	srcs = e[1]
        srcs += compat
	executable(e[0], srcs, dependencies : e[2], install : true)
endforeach
