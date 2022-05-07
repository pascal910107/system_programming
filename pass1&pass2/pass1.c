/***********************************************************************/
/*  Program Name: 3-asm_pass1_u.c                                      */
/*  This program is the part of SIC/XE assembler Pass 1. */
/*  The program only identify the symbol, opcode and operand 		   */
/*  of a line of the asm file. The program do not build the            */
/*  SYMTAB. */
/*  2019.12.13                                                         */
/*  2021.03.26 Process error: format 1 & 2 instruction use + 		   */
/***********************************************************************/
#include <stdlib.h>
#include <string.h>

#include "2-optable.c"

/* Public variables and functions */
#define ADDR_SIMPLE 0x01
#define ADDR_IMMEDIATE 0x02
#define ADDR_INDIRECT 0x04
#define ADDR_INDEX 0x08

#define LINE_EOF (-1)
#define LINE_COMMENT (-2)
#define LINE_ERROR (0)
#define LINE_CORRECT (1)

typedef struct {
  char symbol[LEN_SYMBOL];
  char op[LEN_SYMBOL];
  char operand1[LEN_SYMBOL];
  char operand2[LEN_SYMBOL];
  unsigned code;
  unsigned fmt;
  unsigned addressing;
} LINE;

typedef struct symtab SYMTAB;
typedef SYMTAB *SymPtr;
struct symtab {
  char symbol[LEN_SYMBOL];
  unsigned pc;
  SymPtr nextPtr;
};
int process_line(LINE *line);
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT and Instruction
 * information in *line*/

/* Private variable and function */

void init_LINE(LINE *line) {
  line->symbol[0] = '\0';
  line->op[0] = '\0';
  line->operand1[0] = '\0';
  line->operand2[0] = '\0';
  line->code = 0x0;
  line->fmt = 0x0;
  line->addressing = ADDR_SIMPLE;
}

int process_line(LINE *line)
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT */
{
  char buf[LEN_SYMBOL];
  int c;
  int state;
  int ret;
  Instruction *op;

  c = ASM_token(buf); /* get the first token of a line,len or EOF */
  if (c == EOF)
    return LINE_EOF;
  else if ((c == 1) && (buf[0] == '\n')) /* blank line */
    return LINE_COMMENT;
  else if ((c == 1) && (buf[0] == '.')) /* a comment line */
  {
    do {
      c = ASM_token(buf);
    } while ((c != EOF) && (buf[0] != '\n'));
    return LINE_COMMENT;
  } else {
    init_LINE(line);
    ret = LINE_ERROR;
    state = 0;
    while (state < 8) {
      switch (state) {
        case 0:
        case 1:
        case 2:
          op = is_opcode(buf);                // buf在optab的位址 or null
          if ((state < 2) && (buf[0] == '+')) /* + */
          {
            line->fmt = FMT4;
            state = 2;
          } else if (op != NULL) /* INSTRUCTION */
          {
            strcpy(line->op, op->op);
            line->code = op->code;
            state = 3;
            if (line->fmt != FMT4) {
              line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
            } else if ((line->fmt == FMT4) &&
                       ((op->fmt & FMT4) ==
                        0)) /* INSTRUCTION is FMT1 or FMT 2*/
            {               /* ERROR 20210326 added */
              printf("ERROR at token %s, %s cannot use format 4 \n", buf, buf);
              ret = LINE_ERROR;
              state = 7; /* skip following tokens in the line */
            }
          } else if (state == 0) /* SYMBOL */
          {
            strcpy(line->symbol, buf);
            state = 1;
          } else /* ERROR */
          {
            printf("ERROR at token %s\n", buf);
            ret = LINE_ERROR;
            state = 7; /* skip following tokens in the line */
          }
          break;
        case 3:
          if (line->fmt == FMT1 || line->code == 0x4C) /* no operand needed */
          {
            if (c == EOF || buf[0] == '\n') {
              ret = LINE_CORRECT;
              state = 8;
            } else /* COMMENT */
            {
              ret = LINE_CORRECT;
              state = 7;
            }
          } else {
            if (c == EOF || buf[0] == '\n') {
              ret = LINE_ERROR;
              state = 8;
            } else if (buf[0] == '@' || buf[0] == '#') {
              line->addressing =
                  (buf[0] == '#') ? ADDR_IMMEDIATE : ADDR_INDIRECT;
              state = 4;
            } else /* get a symbol */
            {
              op = is_opcode(buf);
              if (op != NULL) {
                printf("Operand1 cannot be a reserved word\n");
                ret = LINE_ERROR;
                state = 7; /* skip following tokens in the line */
              } else {
                strcpy(line->operand1, buf);
                state = 5;
              }
            }
          }
          break;
        case 4:
          op = is_opcode(buf);
          if (op != NULL) {
            printf("Operand1 cannot be a reserved word\n");
            ret = LINE_ERROR;
            state = 7; /* skip following tokens in the line */
          } else {
            strcpy(line->operand1, buf);
            state = 5;
          }
          break;
        case 5:
          if (c == EOF || buf[0] == '\n') {
            ret = LINE_CORRECT;
            state = 8;
          } else if (buf[0] == ',') {
            state = 6;
          } else /* COMMENT */
          {
            ret = LINE_CORRECT;
            state = 7; /* skip following tokens in the line */
          }
          break;
        case 6:
          if (c == EOF || buf[0] == '\n') {
            ret = LINE_ERROR;
            state = 8;
          } else /* get a symbol */
          {
            op = is_opcode(buf);
            if (op != NULL) {
              printf("Operand2 cannot be a reserved word\n");
              ret = LINE_ERROR;
              state = 7; /* skip following tokens in the line */
            } else {
              if (line->fmt == FMT2) {
                strcpy(line->operand2, buf);
                ret = LINE_CORRECT;
                state = 7;
              } else if ((c == 1) && (buf[0] == 'x' || buf[0] == 'X')) {
                line->addressing = line->addressing | ADDR_INDEX;
                strcpy(line->operand2, buf);
                ret = LINE_CORRECT;
                state = 7; /* skip following tokens in the line */
              } else {
                printf("Operand2 exists only if format 2  is used\n");
                ret = LINE_ERROR;
                state = 7; /* skip following tokens in the line */
              }
            }
          }
          break;
        case 7: /* skip tokens until '\n' || EOF */
          if (c == EOF || buf[0] == '\n') state = 8;
          break;
      }
      if (state < 8) c = ASM_token(buf); /* get the next token */
    }
    return ret;
  }
}

SymPtr creatNode(char *symbol, unsigned pc) {
  SymPtr newNode = malloc(sizeof(SYMTAB));
  strcpy(newNode->symbol, symbol);
  newNode->pc = pc;
  newNode->nextPtr = NULL;
  return newNode;
}
SymPtr insert(SymPtr rootPtr, char *symbol, unsigned pc) {
  if (rootPtr == NULL) {
    rootPtr = creatNode(symbol, pc);
  } else {
    rootPtr->nextPtr = insert(rootPtr->nextPtr, symbol, pc);
  }
  return rootPtr;
}

// int main(int argc, char *argv[]) {
//   int i, c, line_count;
//   char buf[LEN_SYMBOL];
//   LINE line;
//   unsigned pc = 0x0;
//   SymPtr sptr = NULL;

//   if (argc < 2) {
//     printf("Usage: %s fname.asm\n", argv[0]);
//   } else {
//     if (ASM_open(argv[1]) == NULL)
//       printf("File not found!!\n");
//     else {
//       for (line_count = 1; (c = process_line(&line)) != LINE_EOF;
//            line_count++) {
//         if (c == LINE_ERROR)
//           printf("%03d : Error\n", line_count);
//         else if (c == LINE_COMMENT)
//           printf("Comment line\n");
//         else {
//           if(!strcmp(line.op, "START")){
//             pc += atoi(line.operand1);
//           }
//           if (line.code != OP_END || line.code != OP_BASE) {
//             printf("%06x ", pc);
//           } else {
//             printf("       ");
//           }
//           printf("%12s ", line.symbol);
//           if (strcmp("\0", line.symbol) != 0) {
//             sptr = insert(sptr, line.symbol, pc);
//           }
//           if (line.fmt == FMT4) {
//             char str[] = "";
//             strcat(str, "+");
//             strcat(str, line.op);
//             printf("%12s ", str);
//           } else {
//             printf("%12s ", line.op);
//           }
//           if ((line.addressing & ADDR_IMMEDIATE) == ADDR_IMMEDIATE) {
//             char str[] = "";
//             strcat(str, "#");
//             strcat(str, line.operand1);
//             printf("%12s", str);
//           } else if ((line.addressing & ADDR_INDIRECT) == ADDR_INDIRECT) {
//             char str[] = "";
//             strcat(str, "@");
//             strcat(str, line.operand1);
//             printf("%12s", str);
//           } else {
//             printf("%12s", line.operand1);
//           }
//           if (((line.addressing & ADDR_INDEX) == ADDR_INDEX) ||
//               line.fmt == FMT2) {
//             printf(",%12s", line.operand2);
//           }
//           printf("\n");
//           switch (line.code) {
//             case 0x101:
//               line.fmt = FMT1;
//               if(((line.operand1[0] == 'C') || (line.operand1[0] == 'X')) && (line.operand1[1] = '\'')){
//                 line.fmt = FMT1 * (strlen(line.operand1) - 3);
//               }
//               break;
//             case 0x103:
//               line.fmt = FMT1 * atoi(line.operand1);
//               break;
//             case 0x102:
//               line.fmt = FMT3;
//               break;
//             case 0x104:
//               line.fmt = FMT3 * atoi(line.operand1);
//               break;
//             default:
//               break;
//           }
//           pc += line.fmt;
//         }
//       }
//       printf("Program length = %x\n", pc);
//       while (sptr != NULL) {
//         printf("%9s:    %06x\n", sptr->symbol, sptr->pc);
//         sptr = sptr->nextPtr;
//       }
//       ASM_close();
//     }
//   }
// }
SymPtr pass1(int argc, char *argv[]) {
  int i, c, line_count;
  char buf[LEN_SYMBOL];
  LINE line;
  unsigned pc = 0x0;
  SymPtr sptr = NULL;

  if (argc < 2) {
    // printf("Usage: %s fname.asm\n", argv[0]);
  } else {
    if (ASM_open(argv[1]) == NULL) {
      // printf("File not found!!\n");
    } else {
      for (line_count = 1; (c = process_line(&line)) != LINE_EOF;
           line_count++) {
        if (c == LINE_ERROR) {
          // printf("%03d : Error\n", line_count);
        } else if (c == LINE_COMMENT) {
          // printf("Comment line\n");
        } else {
          if(!strcmp(line.op, "START")) {
            pc += strtoul(line.operand1, NULL, 16);
          }
          if (line.code != OP_END && line.code != OP_BASE) {
            // printf("%06x ", pc);
          } else {
            // printf("       ");
          }
          // printf("%12s ", line.symbol);
          if (strcmp("\0", line.symbol) != 0) {
            sptr = insert(sptr, line.symbol, pc);
          }
          if (line.fmt == FMT4) {
            char str[] = "";
            strcat(str, "+");
            strcat(str, line.op);
            // printf("%12s ", str);
          } else {
            // printf("%12s ", line.op);
          }
          if ((line.addressing & ADDR_IMMEDIATE) == ADDR_IMMEDIATE) {
            char str[] = "";
            strcat(str, "#");
            strcat(str, line.operand1);
            // printf("%12s", str);
          } else if ((line.addressing & ADDR_INDIRECT) == ADDR_INDIRECT) {
            char str[] = "";
            strcat(str, "@");
            strcat(str, line.operand1);
            // printf("%12s", str);
          } else {
            // printf("%12s", line.operand1);
          }
          if (((line.addressing & ADDR_INDEX) == ADDR_INDEX) ||
              line.fmt == FMT2) {
            // printf(",%12s", line.operand2);
          }
          // printf("\n");
          switch (line.code) {
            case OP_BYTE:
              line.fmt = FMT1;
              if((line.operand1[0] == 'C') && (line.operand1[1] = '\'')){
                line.fmt = FMT1 * (strlen(line.operand1) - 3);
              } else if ((line.operand1[0] == 'X') && (line.operand1[1] = '\'')) {
                line.fmt = FMT1 * (strlen(line.operand1) - 3) / 2;
              }
              break;
            case OP_RESB:
              line.fmt = FMT1 * atoi(line.operand1);
              break;
            case OP_WORD:
              line.fmt = FMT3;
              break;
            case OP_RESW:
              line.fmt = FMT3 * atoi(line.operand1);
              break;
            default:
              break;
          }
          pc += line.fmt;
        }
      }
      // printf("Program length = %x\n", pc);
      // while (sptr != NULL) {
      //   printf("%9s:    %06x\n", sptr->symbol, sptr->pc);
      //   sptr = sptr->nextPtr;
      // }
      sptr = insert(sptr, "PROGRAM_LENGTH", pc);
      ASM_close();
      return sptr;
    }
  }
}
