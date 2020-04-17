// nasm -felf64 code.asm && gcc code.o && ./a.out
// nasm -felf64 code.asm && gcc -no-pie code.o && ./a.out

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeGen.h"
#include "symbolTable.h"
#include "typeCheck.h"
#include "lexerDef.h"

extern char *inverseMappingTable[];


void printLeaf(ASTNode* leaf, FILE* fp) {

}


char *baseRegister[2] = {"RBP", "RSI"};

char *expreg[3] = {"r8","r9","r10"};
char *expSizeStr = "word";
char *expSizeRegSuffix = "";    //"d","w",""
int scale = 2;

void setExpSize(gSymbol etype){
    switch(etype){
        case g_INTEGER:
            expSizeStr = "dword";
            expSizeRegSuffix = "d";
            break;
        case g_BOOLEAN:
            expSizeStr = "word";
            expSizeRegSuffix = "w";
            break;
        case g_REAL:
            expSizeStr = "qword";
            expSizeRegSuffix = "";
            break;
        default:
            expSizeStr = "word";
            expSizeRegSuffix = "w";
            break;
    }
}

void genExpr(ASTNode *astNode, FILE *fp, bool firstCall, gSymbol expType){
    if(firstCall){
        if(astNode == NULL)
            return;
        else if(astNode->gs != g_assignmentStmt){
            //            printf("%d \n",astNode->gs);
            genExpr(astNode->next,fp,true,expType);
            genExpr(astNode->child,fp,true,expType);
            return;
        }
        //assignmentStatement Node will be passed
        ASTNode *idNode = astNode->child->child->child;
        expType = idNode->stNode->info.var.vtype.baseType;
        setExpSize(expType);
//        printf("%s \n",idNode->tkinfo->lexeme);
        if(idNode->next->next != NULL && idNode->next->gs == g_NUM){
            //array element and static index
            genExpr(idNode->next->next,fp,false,expType);
            //TODO: Handle Array elements with static index
        }
        else if(idNode->next->next != NULL && idNode->next->gs == g_ID){
            //TODO: handle array element with dynamic index

        }
        else{
            //array or variable
            if(idNode->stNode->info.var.vtype.vaType == VARIABLE){
                //variable
                genExpr(idNode->next,fp,false,expType);
                bool isIOlistVar = idNode->stNode->info.var.isIOlistVar;
                int toSub = scale * (idNode->stNode->info.var.offset + idNode->stNode->info.var.vtype.width);
                fprintf(fp,"\t xor %s, %s \n",expreg[1],expreg[1]);
                fprintf(fp,"\t pop %s \n",expreg[1]);
                fprintf(fp, "\t mov %s[%s-%d], %s%s \n", expSizeStr, baseRegister[isIOlistVar], toSub, expreg[1],expSizeRegSuffix);
                return;
            }
            else{
                //array
                //TODO: just put data from right array to left array
            }
        }
    }
    else{
        if(astNode->gs == g_u){
            //TODO: Handle unary
        }
        else if(astNode->gs == g_var_id_num){
            astNode = astNode->child;
            switch(astNode->gs){
                case g_NUM:
                    fprintf(fp,"\t push %d \n",astNode->tkinfo->value.num);
                    break;
                case g_RNUM:
                    //TODO: Mov real constant to stack
                    break;
                case g_ID:
                {
                    if(astNode->stNode->info.var.vtype.vaType == VARIABLE){
                        bool isIOlistVar = astNode->stNode->info.var.isIOlistVar;
                        int toSub = scale * (astNode->stNode->info.var.offset + astNode->stNode->info.var.vtype.width);
                        switch(astNode->stNode->info.var.vtype.baseType){
                            case g_REAL:
                                //TODO: Handle real variables
                                break;
                            case g_INTEGER:
                            case g_BOOLEAN:
                                fprintf(fp,"\t xor %s,%s \n",expreg[0],expreg[0]);
                                fprintf(fp,"\t mov %s%s, %s[%s-%d] \n",expreg[0],expSizeRegSuffix,expSizeStr,baseRegister[isIOlistVar],toSub);
                                fprintf(fp,"\t push %s \n",expreg[0]);
                                break;
                        }
                    }
                    else{
                        //TODO: Array element handling
                    }
                }
                break;
            }
        }
        else if(astNode->gs == g_TRUE){
            fprintf(fp,"\t push 1 \n");
        }
        else if(astNode->gs == g_FALSE){
            fprintf(fp,"\t push 0 \n");
        }
        else{
            //astnode is an operator
            genExpr(astNode->child,fp,false,expType);
            genExpr(astNode->child->next,fp,false,expType);
            if(expType == g_REAL){
                //handle real operations
            }
            else{
                fprintf(fp,"\t xor %s,%s \n",expreg[1],expreg[1]);
                fprintf(fp,"\t pop %s \n",expreg[1]);
                fprintf(fp,"\t xor %s,%s \n",expreg[0],expreg[0]);
                fprintf(fp,"\t pop %s \n",expreg[0]);
                switch(astNode->gs){
                    case g_PLUS:
                        fprintf(fp,"\t add %s, %s \n",expreg[0],expreg[1]);
                        break;
                    case g_MINUS:
                        fprintf(fp,"\t sub %s, %s \n",expreg[0],expreg[1]);
                        break;
                        //Many more cases to come...
                }
                fprintf(fp,"\t push %s \n",expreg[0]);
            }
            return;
        }
    }
}


void generateCode(ASTNode* root, symbolTable* symT, FILE* fp) {
    if(root == NULL) return;

    gSymbol gs = root->gs;
    printf("%s  \n", inverseMappingTable[gs]);
    switch(gs) {
        case g_program:
        {
            fprintf(fp, "section .bss \n");
            fprintf(fp, "\t stack_top: resb 8 \n");
            fprintf(fp, "\t inta: resb 4 \n");
            fprintf(fp, "\t floatb: resb 8 \n");
            fprintf(fp, "\t boolc: resb 2 \n");

            fprintf(fp, "section .data \n");

            // To be removed
            fprintf(fp, "\t sampleInt: db 5,0 \n");
            fprintf(fp, "\t sampleFloat: db -5.2,0 \n");


            fprintf(fp,"\t msgBoolean: db \"Input: Enter a boolean value:\", 10, 0 \n");
            fprintf(fp,"\t inputBoolean: db \"%%hd\", 0 \n");

            fprintf(fp,"\t msgInt: db \"Input: Enter an integer value:\", 10, 0 \n");
            fprintf(fp,"\t inputInt: db \"%%d\", 0 \n");

            fprintf(fp,"\t msgFloat: db \"Input: Enter a float value:\", 10, 0 \n");
            fprintf(fp,"\t inputFloat: db \"%%lf\",0 \n");

            fprintf(fp,"\t outputBooleanTrue: db \"Output: true\", 10, 0, \n");
            fprintf(fp,"\t outputBooleanFalse: db \"Output: false\", 10, 0, \n");

            fprintf(fp,"\t outputInt: db \"Output: %%d\", 10, 0, \n");

            fprintf(fp,"\t outputFloat: db \"Output: %%lf\", 10, 0, \n");

            fprintf(fp, " \nsection .text \n");
            fprintf(fp, "\t global main \n");
            fprintf(fp, "\t extern scanf \n");
            fprintf(fp, "\t extern printf \n");

            ASTNode* ASTChild = root->child;

            // Might need to change its position.
            while(ASTChild) {
                generateCode(ASTChild, symT, fp);
                ASTChild = ASTChild->next;
            }

            return;
        }

        // TODO(apb7): Pull common functionality out and separate these cases if needed.
        case g_moduleDeclarations:
        case g_otherModules:
        case g_statements:
        case g_module:
        {
            // moduleDeclarations -> ID moduleDeclarations

            ASTNode* ASTChild = root->child;

            while(ASTChild) {
                generateCode(ASTChild, symT, fp);
                ASTChild = ASTChild->next;
            }

            return;
        }

        case g_DRIVER:
        {
            fprintf(fp, " \nmain: \n");
            fprintf(fp, "\t mov rbp, rsp \n");
            fprintf(fp, "\t mov QWORD[stack_top], rsp \n");
            fprintf(fp, "\t sub rsp, 192 \n"); // to fix this! AR space needed

            generateCode(root->child, symT, fp);

            fprintf(fp, "\t mov rsp, rbp \n");

            return;
        }
        case g_moduleDef:
        case g_simpleStmt:
        case g_START:
        {
            generateCode(root->child, symT, fp);

            return;
        }

        case g_ID:
        {
            return;
        }

        case g_ioStmt:
        {
            generateCode(root->child, symT, fp);
            return;
        }

        case g_GET_VALUE:
        {

            ASTNode* siblingId = root->next;

            // <ioStmt> -> GET_VALUE BO ID BC SEMICOL

            if(! siblingId->stNode) {
                // printf("ERROR: Undeclared variable \n");
                // Already being handled.
                return;
            }

            varType idVarType = siblingId->stNode->info.var.vtype;
            symVarInfo idVar = siblingId->stNode->info.var;

            if(idVarType.vaType == VARIABLE) {
                // More registers need to me pushed to preserve
                // their values.
                // BEWARE: Number of pushes here should be odd.
                // push rbx
                fprintf(fp, "\t push rbp \n");

                if(idVarType.baseType == g_BOOLEAN) {
                    fprintf(fp, "\t mov rdi, msgBoolean  \n ");
                    fprintf(fp, "\t call printf  \n ");
                    fprintf(fp, "\t mov rdi, inputBoolean \n");
                    fprintf(fp, "\t mov rsi, %s \n", baseRegister[idVar.isIOlistVar]);
                    fprintf(fp, "\t sub rsi, %d \n", 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t call scanf \n");
                    // Scanned int goes to rax or rdx:rax.
                    // Scanned float goes to xmm0 or xmm1:xmm0.
                    // Note by Hasan: doesn't work with regs
                }
                else if(idVarType.baseType == g_INTEGER) {
                    fprintf(fp, "\t mov rdi, msgInt \n");
                    fprintf(fp, "\t call printf \n");
                    fprintf(fp, "\t mov rdi, inputInt \n");
                    fprintf(fp, "\t mov rsi, %s \n", baseRegister[idVar.isIOlistVar]);
                    fprintf(fp, "\t sub rsi, %d \n", 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t call scanf \n");

                    // Check the value being scanned
                    // fprintf(fp, "\t mov rdi, outputInt \n");
                    // fprintf(fp, "\t mov rsi, [inta] \n");
                    // fprintf(fp, "\t call printf \n");


                }
                else if(idVarType.baseType == g_REAL) {
                    fprintf(fp, "\t mov rdi, msgFloat \n");
                    fprintf(fp, "\t call printf \n");

                    fprintf(fp, "\t mov rdi, inputFloat \n");
                    fprintf(fp, "\t mov rsi, %s \n", baseRegister[idVar.isIOlistVar]);
                    fprintf(fp, "\t sub rsi, %d \n", 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t call scanf \n");

                    // Check the value being scanned
                    // fprintf(fp, "\t mov rdi, outputFloat \n");
                    // fprintf(fp, "\t mov xmm0, [floatb] \n");
                    // fprintf(fp, "\t call printf \n");

                }

                fprintf(fp, "\t pop rbp \n");
            }
            else if(idVarType.vaType == STAT_ARR) {
                fprintf(fp, "\t push rbp \n");
    
                // The only registers that the called function is required to preserve (the calle-save registers) are:
                // rbp, rbx, r12, r13, r14, r15. All others are free to be changed by the called function.
                if(idVarType.baseType == g_INTEGER) {

                    fprintf(fp, "\t mov rdi, msgInt \n");
                    fprintf(fp, "\t call printf \n");

                    fprintf(fp, "\t mov rsi, %s \n", baseRegister[idVar.isIOlistVar]); // isIOlistVar must be 0
                    fprintf(fp, "\t sub rsi, %d \n", 2 * (1 + idVar.offset));
                    fprintf(fp, "\t movsx rsi, word[rsi] \n"); // move base val
                    fprintf(fp, "\t add rsi, stack_top \n"); // address of first elem!
                    
                    fprintf(fp, "\t mov rdi, inputInt \n");

                    fprintf(fp, "\t mov r12, %d \n", idVarType.si.vt_num );
                    fprintf(fp, "statarr_%p: \n", siblingId);
                    
                    fprintf(fp, "\t push rsi \n");
                    fprintf(fp, "\t push rdi \n");
                    
                    fprintf(fp, "\t call scanf \n");

                    fprintf(fp, "\t pop rdi \n");
                    fprintf(fp, "\t pop rsi \n");

                    fprintf(fp, "\t cmp r12, %d \n", idVarType.ei.vt_num );
                    fprintf(fp, "\t jz statarrExit_%p \n", siblingId);

                    fprintf(fp, "\t inc r12 \n");
                    fprintf(fp, "\t sub rsi, -4 \n"); // address of n-th elem 

                    fprintf(fp, "\t jmp statarr_%p \n", siblingId);

                    fprintf(fp, "statarrExit_%p: \n", siblingId);


                    // Check the value being scanned
                    // fprintf(fp, "\t mov rdi, outputInt \n");
                    // fprintf(fp, "\t mov rsi, [inta] \n");
                    // fprintf(fp, "\t call printf \n");


                }

                fprintf(fp, "\t pop rbp \n");
            }
            else /* Arrays */ {

            }

            return;
        }

        case g_PRINT:
        {   
            // Need changes here!
            ASTNode* sibling = root->next;

            // <ioStmt> -> PRINT BO <var> BC SEMICOL
            // <boolConstt> -> TRUE | FALSE
            // <var_id_num> -> ID <whichId> | NUM | RNUM
            // <var> -> <var_id_num> | <boolConstt>
            // <whichId> -> SQBO <index> SQBC | ε
            // <index> -> NUM | ID



            if(sibling->gs == g_TRUE) {
                fprintf(fp, "\t push rbp \n");
                fprintf(fp, "\t mov rdi, outputBooleanTrue \n");
                fprintf(fp, "\t call printf \n");
                fprintf(fp, "\t pop rbp \n");
                return;
            }

            if(sibling->gs == g_FALSE) {
                fprintf(fp, "\t push rbp \n");
                fprintf(fp, "\t mov rdi, outputBooleanFalse \n");
                fprintf(fp, "\t call printf \n");
                fprintf(fp, "\t pop rbp \n");
                return;
            }

            ASTNode *siblingId = sibling->child;

            if(siblingId->gs == g_NUM) {
                fprintf(fp, "\t push rbp \n");
                fprintf(fp, "\t mov rdi, outputInt \n");
                fprintf(fp, "\t mov rsi, %d \n", siblingId->tkinfo->value.num);
                fprintf(fp, "\t call printf \n");
                fprintf(fp, "\t pop rbp \n");
                return;
            }

            // TODO: see how floating pt values can be assigned!

            if(siblingId->gs == g_RNUM) {
                fprintf(fp, "\t push rbp \n");
                fprintf(fp, "\t mov rdi, outputFloat \n");
                fprintf(fp, "\t mov rsi, __float64__(%s) \n", siblingId->tkinfo->lexeme);
                fprintf(fp, "\t movq xmm0, rsi  \n");
                fprintf(fp, "\t mov rax, 1  \n");
                // printf expects double but rsi has float. Therefore, output is 0.000
                // Need to find a way around this using fld instr but be careful with stack.
                fprintf(fp, "\t call printf \n");
                fprintf(fp, "\t pop rbp \n");
                return;
            }

            varType idVarType = siblingId->stNode->info.var.vtype;
            symVarInfo idVar = siblingId->stNode->info.var;

            if(idVarType.vaType == VARIABLE) {
                // More registers need to me pushed to preserve
                // their values.
                // BEWARE: Number of pushes here should be odd.
                // push rbx

                fprintf(fp, "\t push rbp \n");

                if(idVarType.baseType == g_BOOLEAN) {
                    fprintf(fp, "\t cmp word[%s - %d], 0 \n", baseRegister[idVar.isIOlistVar], 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t jz boolPrintFalse%d \n", siblingId->tkinfo->lno);

                    fprintf(fp, "boolPrintTrue%d: \n", siblingId->tkinfo->lno);
                    fprintf(fp, "\t mov rdi, outputBooleanTrue \n");
                    fprintf(fp, "\t jmp boolPrintEnd%d \n", siblingId->tkinfo->lno);

                    fprintf(fp, "boolPrintFalse%d: \n", siblingId->tkinfo->lno);
                    fprintf(fp, "\t mov rdi, outputBooleanFalse \n");

                    fprintf(fp, "boolPrintEnd%d: \n", siblingId->tkinfo->lno);
                    fprintf(fp, "\t call printf \n");
                }
                else if(idVarType.baseType == g_INTEGER) {

                    fprintf(fp, "\t mov rdi, outputInt \n");
                    fprintf(fp, "\t mov rsi, [%s - %d] \n", baseRegister[idVar.isIOlistVar], 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t call printf \n");
                }
                else if(idVarType.baseType == g_REAL) {
                    fprintf(fp, "\t mov rdi, outputFloat \n");
                    fprintf(fp, "\t mov rsi, [%s - %d] \n", baseRegister[idVar.isIOlistVar], 2 * (idVarType.width + idVar.offset));
                    fprintf(fp, "\t call printf \n");
                }

                fprintf(fp, "\t pop rbp \n");
            }
            else /* Arrays */ {
                // Use whichId AST Node here.
            }

            return;
        }

        case g_declareStmt:
        {
            ASTNode *idList = root->child;

            ASTNode *id = idList->child;

            while(id != NULL) {
                symVarInfo idVar = id->stNode->info.var;

                if(idVar.vtype.vaType == VARIABLE)
                    return;
                
                else if(idVar.vtype.vaType == STAT_ARR) {
                    fprintf(fp, "\t mov rsi, %s \n", baseRegister[idVar.isIOlistVar]); // isIOlistVar must be 0!
                    fprintf(fp, "\t mov rdi, rsi \n");
                    fprintf(fp, "\t sub rdi, %d \n", 2 * (idVar.offset + 1)); // Base address
                    fprintf(fp, "\t sub rsi, %d \n", 2 * (idVar.offset + 1 + getSizeByType(idVar.vtype.baseType))); // First elem
                    fprintf(fp, "\t sub rsi, [stack_top] \n"); // Find location relative to top of stack!
                    fprintf(fp, "\t mov word[rdi], si \n"); // only 1 location = 2B available!
                }

                id = id->next;
            }

        }
        
        case g_assignmentStmt:
            genExpr(root,fp,true,0);
            return;

        case g_conditionalStmt:{

        }

            return;
        default:
            printf("Default : %s \n", inverseMappingTable[gs]);
    }

}