
########### next target ###############

SET(btparse_STAT_SRCS
   bibtex_ast.c
   bibtex.c
   err.c
   ast.c
   scan.c
   util.c
   lex_auxiliary.c
   parse_auxiliary.c
   format_name.c
   string_util.c
   tex_tree.c
   names.c
   modify.c
   traversal.c
   sym.c
   macros.c
   error.c
   postprocess.c
   input.c
   init.c
   )

ADD_LIBRARY(btparse STATIC ${btparse_STAT_SRCS})
