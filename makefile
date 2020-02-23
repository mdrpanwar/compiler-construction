all:    driver.c lexer/lexer.c utils/hash.c utils/util.c parser/parser.c utils/treeNodePtr_stack.c utils/treeNodePtr_stack_config.c utils/set.c utils/set.h parser/parser.h parser/parserDef.h lexer/lexer.h lexer/lexerDef.h utils/hash.h utils/util.h utils/treeNodePtr_stack.h utils/treeNodePtr_stack_config.h
	mkdir OUTPUT -p && gcc driver.c lexer/lexer.c utils/hash.c utils/util.c parser/parser.c utils/treeNodePtr_stack.c utils/treeNodePtr_stack_config.c utils/set.c -o OUTPUT/stage1.out
