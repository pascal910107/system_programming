#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pass1.c"

#define max 0xFFFFFFFF

typedef struct modification Modification;
typedef Modification *ModPtr;
struct modification {
  unsigned pc;
  unsigned len;
  ModPtr nextPtr;
};
ModPtr m_creatNode(unsigned pc, unsigned len) {
  ModPtr newNode = malloc(sizeof(Modification));
  newNode->pc = pc;
  newNode->len = len;
  newNode->nextPtr = NULL;
  return newNode;
}
ModPtr m_insert(ModPtr rootPtr, unsigned pc, unsigned len) {
  if (rootPtr == NULL) {
    rootPtr = m_creatNode(pc, len);
  } else {
    rootPtr->nextPtr = m_insert(rootPtr->nextPtr, pc, len);
  }
  return rootPtr;
}

char *charupper(char *c) {
  int i;
  for (i = 0; i < strlen(c); i++) {
    if (c[i] <= 'z' && c[i] >= 'a') {
      c[i] -= 32;
    }
  }
  return c;
}

char *SIC(unsigned line_code, int x, char *line_operand1, SymPtr sptr){
  unsigned disp = 0x0;
  char *str = malloc(10);
  while (strcmp(sptr->symbol, "PROGRAM_LENGTH")) {
    if (!strcmp(sptr->symbol, line_operand1)) {
      disp = sptr->pc;
      break;
    }
    sptr = sptr->nextPtr;
  }
  if (x) {
    disp += 0x8000;//0b1000000000000000
    sprintf(str, "%02x%04x", line_code, disp);
  } else {
    sprintf(str, "%02x%04x", line_code, disp);
  }
  return str;
}
char *SICXE(unsigned line_fmt, unsigned line_code,int x,unsigned b, unsigned line_addressing, char *line_operand1, char *line_operand2, SymPtr sptr, unsigned pcnow){
  unsigned disp, ta = 0;
  char *str = malloc(10);
  unsigned l_o1, l_o2, xbpe = 0x0;
  while (strcmp(sptr->symbol, "PROGRAM_LENGTH")) {
    if (!strcmp(sptr->symbol, line_operand1)) {
      ta = sptr->pc;
      break;
    }
    sptr = sptr->nextPtr;
  }
  switch (line_fmt) {
    case FMT0:
      return "";
    case FMT1:
      sprintf(str, "%02x", line_code);
      return str;
    case FMT2:
      switch (line_operand1[0]) {
        case 'A':
          l_o1 = 0;
          break;
        case 'X':
          l_o1 = 1;
          break;
        case 'L':
          l_o1 = 2;
          break;
        case 'B':
          l_o1 = 3;
          break;
        case 'S':
          l_o1 = 4;
          break;
        case 'T':
          l_o1 = 5;
          break;
        case 'F':
          l_o1 = 6;
          break;
        default:
          break;
      }
      switch (line_operand2[0]) {
        case 'A':
          l_o2 = 0;
          break;
        case 'X':
          l_o2 = 1;
          break;
        case 'L':
          l_o2 = 2;
          break;
        case 'B':
          l_o2 = 3;
          break;
        case 'S':
          l_o2 = 4;
          break;
        case 'T':
          l_o2 = 5;
          break;
        case 'F':
          l_o2 = 6;
          break;
        default:
          l_o2 = 0;
          break;
      }
      sprintf(str, "%02x%x%x", line_code, l_o1, l_o2);
      return str;
    case FMT3:
      if ((line_addressing & ADDR_IMMEDIATE) == ADDR_IMMEDIATE) {
        line_code += 1;
        char temp = line_operand1[0];
        if (temp >= '0' && temp <= '9') {
          disp = strtoul(line_operand1, NULL, 10);
          sprintf(str, "%02x%1x%03x", line_code, xbpe, disp);
          return str;
        }
      } else if ((line_addressing & ADDR_INDIRECT) == ADDR_INDIRECT) {
        line_code += 2;
      } else {
        line_code += 3;
      }
      if (!strlen(line_operand1)) {
        disp = ta;
      } else if (b) {
        disp = ta - b;
        if (x) {
          xbpe = 12;
        } else {
          xbpe = 4;
        }
      } else {
        disp = ta - pcnow;
        if (x) {
          xbpe = 10;
        } else {
          xbpe = 2;
        }
      }
      char temp[] = "";
      sprintf(str, "%03x", disp);
      int t = strlen(str) - 3;
      strncpy(temp, &str[t], 3);
      sprintf(str, "%02x%1x%03s", line_code, xbpe, temp);
      return str;
    case FMT4:
      disp = ta;
      xbpe = 1;
      if ((line_addressing & ADDR_IMMEDIATE) == ADDR_IMMEDIATE) {
        line_code += 1;
        char temp = line_operand1[0];
        if (temp >= '0' && temp <= '9') {
          disp = strtoul(line_operand1, NULL, 10);
          sprintf(str, "%02x%1x%05x", line_code, xbpe, disp);
          return str;
        }
      } else if ((line_addressing & ADDR_INDIRECT) == ADDR_INDIRECT) {
        line_code += 2;
      } else {
        line_code += 3;
      }
      if (x) {
        xbpe = 9;
      }
      sprintf(str, "%02x%1x%05x", line_code, xbpe, disp);
      return str;
      break;
    default:
      break;
  }
  return str;
}




int main(int argc, char *argv[]) {
  unsigned b = 0,tempb = 0;
  int c, line_count, j = 0, k, l = 0, m_l = 0,flag = 0;
  char buf[LEN_SYMBOL];
  char row[60] = "";
  LINE line;
  unsigned pc = 0x0, disp, pctemp = 0x0, pcfir, pcnow;
  SymPtr sptr = pass1(argc, argv);
  SymPtr ptr = sptr;
  ModPtr mptr = NULL;
  while (strcmp(ptr->symbol, "PROGRAM_LENGTH")) {
    ptr = ptr->nextPtr;
  }
  if (argc < 2) {
    printf("Usage: %s fname.asm\n", argv[0]);
  } else {
    if (ASM_open(argv[1]) == NULL)
      printf("File not found!!\n");
    else {
      //H
      c = process_line(&line);
      if (line.code == OP_START) {
        pc += strtoul(line.operand1, NULL, 16);
        pcnow = pcfir = pc;
        printf("H%-6s",line.symbol);
        char temp[] = "";
        sprintf(temp, "%06x%06x", pc, ptr->pc - pc);
        char *str = charupper(temp);
        printf("%s\n",str);
        c = process_line(&line);
      }


      // SIC T
      // while (line.code != OP_END) {
      //   char temp[6] = "",temp2[6] = "", temp4[6] = "", *result, l_c[2] = "";
      //   int temp3;
      //   int x_register = 0;
      //   int next_line = 0;
      //   if (c == LINE_COMMENT){
      //     continue;
      //   }
      //   if (line.code == OP_BYTE) {
      //     if (line.operand1[0] == 'C') {
      //       for (k = 2; k < strlen(line.operand1) - 1; k++) {
      //         sprintf(temp2, "%x", line.operand1[k]);
      //         strcat(temp, temp2);
      //       }
      //       strcat(row, temp);
      //       l += strlen(temp);
      //       pcnow += strlen(temp) / 2;
      //     }else if (line.operand1[0] == 'X') {
      //       strncpy(temp, line.operand1 + 2, strlen(line.operand1) - 3);
      //       strcat(row, temp);
      //       l += strlen(temp);
      //       pcnow += strlen(temp) / 2;
      //     }
      //     flag = 0;
      //   } else if (line.code == OP_WORD) {
      //     temp3 = atoi(line.operand1);
      //     sprintf(temp4, "%06x", temp3);
      //     strcat(row, temp4);
      //     l += 6;
      //     pcnow += 3;
      //     flag = 0;
      //   } else if (line.code == OP_RESW) {
      //     pctemp = FMT3 * atoi(line.operand1);
      //     pcnow += pctemp;
      //     flag += 1;
      //   } else if (line.code == OP_RESB) {
      //     pctemp = FMT1 * atoi(line.operand1);
      //     pcnow += pctemp;
      //     flag += 1;
      //   } else {
      //     if (line.operand2[0] == 'X') {
      //       x_register = 1;
      //     }
      //     result = SIC(line.code, x_register, line.operand1, sptr);
      //     strcat(row, result);
      //     if (strlen(line.operand1)) {
      //       mptr = m_insert(mptr, pcnow - pcfir + 1, 4);
      //     }
      //     l += 6;
      //     pcnow += 3;
      //     flag = 0;
      //   }
      //   if ((l + 6) > 60 || line.code == OP_RESB || line.code == OP_RESW) {
      //     m_l += l + pctemp;
      //     if (flag <= 1) {
      //       char temp[] = "";
      //       sprintf(temp, "%06x%02x", pc, l /= 2);
      //       char *str = charupper(temp);
      //       printf("T%s", str);
      //       str = charupper(row);
      //       printf("%s\n",str);
      //     }
      //     pc = pc + l + pctemp;
      //     l = 0;
      //     pctemp = 0;
      //     strcpy(row, "\0");
      //   }
      //   c = process_line(&line);
      // }


      // SIC/XE T
      while (line.code != OP_END) {
        char temp[6] = "",temp2[6] = "", temp4[6] = "", *result, l_c[2] = "";
        int temp3;
        int x_register = 0;
        int next_line = 0;
        unsigned ta;
        if (c == LINE_COMMENT){
          continue;
        }
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
        pcnow += line.fmt;
        if (line.code == OP_BYTE) {
          if (line.operand1[0] == 'C') {
            for (k = 2; k < strlen(line.operand1) - 1; k++) {
              sprintf(temp2, "%x", line.operand1[k]);
              strcat(temp, temp2);
            }
            strcat(row, temp);
            l += strlen(temp);
          }else if (line.operand1[0] == 'X') {
            strncpy(temp, line.operand1 + 2, strlen(line.operand1) - 3);
            strcat(row, temp);
            l += strlen(temp);
          }
          flag = 0;
        } else if (line.code == OP_WORD) {
          temp3 = atoi(line.operand1);
          sprintf(temp4, "%06x", temp3);
          strcat(row, temp4);
          l += 6;
          flag = 0;
        } else if (line.code == OP_RESW) {
          pctemp = FMT3 * atoi(line.operand1);
          flag += 1;
        } else if (line.code == OP_RESB) {
          pctemp = FMT1 * atoi(line.operand1);
          flag += 1;
        } else {
          if (line.operand2[0] == 'X') {
            x_register = 1;
          }
          ptr = sptr;
          while (strcmp(ptr->symbol, "PROGRAM_LENGTH")) {
            if (!strcmp(ptr->symbol, line.operand1)) {
              ta = ptr->pc;
              break;
            }
            ptr = ptr->nextPtr;
          }
          if (line.code == OP_BASE) {
            tempb = b = ta;
          } else if (line.code == OP_NOBASE) {
            b = 0;
          }
          result = SICXE(line.fmt, line.code, x_register, 0, line.addressing, line.operand1, line.operand2, sptr, pcnow);
          if ((ta > pcnow && ((ta - pcnow) > 2047)) || (ta < pcnow && ((max - ta + pcnow + 1) > 2048))) {
            b = tempb;
            result = SICXE(line.fmt, line.code, x_register, b, line.addressing, line.operand1, line.operand2, sptr, pcnow);
            b = 0;
          }
          if (line.fmt == FMT4) {
            if(((line.addressing & ADDR_INDIRECT) == ADDR_INDIRECT) || ((line.addressing & ADDR_SIMPLE) == ADDR_SIMPLE)) {
              //format4 disp為 20bit(5個half byte)
              mptr = m_insert(mptr, (m_l + l) / 2 + 1, 5);
            }
          }
          strcat(row, result);
          l += strlen(result);
          flag = 0;
        }
        if ((l + 6) > 60 || line.code == OP_RESB || line.code == OP_RESW) {
          m_l += l;
          if (flag <= 1) {
            char temp[] = "";
            sprintf(temp, "%06x%02x", pc, l /= 2);
            char *str = charupper(temp);
            printf("T%s", str);
            str = charupper(row);
            printf("%s\n",str);
          }
          pc = pc + l + pctemp;
          l = 0;
          pctemp = 0;
          strcpy(row, "\0");
        }
        c = process_line(&line);
      }

      //the last line and E
      char temp[] = "";
      sprintf(temp, "%06x%02x", pc, l /= 2);
      char *str = charupper(temp);
      printf("T%s", str);
      str = charupper(row);
      printf("%s\n",str);
      while (mptr != NULL) {
        printf("M%06x%02x\n", mptr->pc, mptr->len);
        mptr = mptr->nextPtr;
      }
      printf("E%06x\n", pcfir);
      ASM_close();
    }
  }
}
