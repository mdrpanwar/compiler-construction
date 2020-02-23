#include "utils/hash.h"
#include "parser/parserDef.h"
#include "parser/parser.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "utils/set.h"
#include "config.h"
#include "lexer/lexer.h"


extern grammarNode *grammarArr;
extern struct hashTable *mt;
extern intSet* firstSet;
extern intSet* followSet;
extern hashTable *keyword_ht;
extern char *inverseMappingTable[];
extern int **parseTable;

extern unsigned int fp;
extern unsigned int bp;
extern unsigned int line_number;

extern int status;
extern int count;


int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Usage: %s <source code file> <parse tree output file>\n", argv[0]);
        exit(1);
    }

    printf("\t FIRST and FOLLOW set automated \n");
    printf("\t Both lexical and syntax analysis modules implemented \n");
    printf("\t Modules work with all testcases \n");
    printf("\t Parse tree is being printed in output file \n\n");

    mt = createHashTable(SYMBOL_HT_SIZE); // 131 is the nearest prime > 114 (# of symbols (NT + T))
    fillHashTable(inverseMappingTable,mt);

    // Create and populate hash table for keywords
    keyword_ht = createHashTable(KEYWORD_HT_SIZE);

    char* keywords[] = {
        #define K(a,b,c) b,
        #include "data/keywords.txt"
        #undef K
        "#"
    };

    fillHashTable(keywords,keyword_ht);

    populateGrammarStruct("../data/grammar.txt");

//     printGrammar();

    populateFirstSet();
    populateFollowSet();
    populateParseTable();

//    printParseTable();

    int userInput = 1; // Non zero initial value, will be overwritten

    while(userInput != 0) {
        printf("\t Press 1 to remove comments.\n\t Press 2 to print all tokens.\n\t Press 3 to parse source code. \n\t Press 4 to print time taken.\n");
        scanf(" %d", &userInput);

        switch(userInput) {

            case 0:
                break;

            case 1:
                removeComments(argv[1], NULL);
                break;

            case 2:
            {
                FILE *fp_arg = fopen(argv[1], "r");
                tokenInfo *tk;

                printf("%12s %20s %20s \n", "Line_number", "lexeme", "Token_name");

                // Initialise lexer every time.
                fp = 0; bp = 0; line_number = 1; status = 1; count = 0;

                while((tk = getNextToken(fp_arg)) != NULL) {
                    printf("%12d %20s %20s\n", tk->lno, tk->lexeme, inverseMappingTable[tk->type]);

                    free(tk);
                    tk = getNextToken(fp_arg);
                }
                free(fp_arg);
            } break;

            case 3:
            {
                // Initialise lexer every time.
                fp = 0; bp = 0; line_number = 1; status = 1; count = 0;

                treeNode *root = parseInputSourceCode(argv[1]);
                printTree(root, argv[2]);
            } break;

            case 4:
            {
                clock_t start_time, end_time;
                double total_CPU_time, total_CPU_time_in_seconds;

                start_time = clock();

                // Initialise lexer every time.
                fp = 0; bp = 0; line_number = 1; status = 1; count = 0;

                treeNode *root = parseInputSourceCode(argv[1]);

                end_time = clock();

                total_CPU_time  =  (double) (end_time - start_time);
                total_CPU_time_in_seconds =   total_CPU_time / CLOCKS_PER_SEC;

                printf("Total CPU time = %lf \nTotal CPU time in secs = %lf \n", total_CPU_time, total_CPU_time_in_seconds);
            } break;

            default:
                printf("Invalid Input \n");
        }
    }

    


}
