SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   )

add_library(cite STATIC ${cite_STAT_SRCS})

ADD_DEPENDENCIES(cite tellico_config)
