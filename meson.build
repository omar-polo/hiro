project('hiro', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

add_global_arguments('-Wno-unused-parameter', language : 'c')

event = dependency('libevent')

executable('hirod',
           'hirod.c',
           'cmd.c',
           'util.c',
           'log.c',
           'can.c',
           dependencies : event,
           install : true)

executable('hiroctl',
           'hiroctl.c',
           'cmd.c',
           'util.c',
           dependencies : event,
           install : true)
