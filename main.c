#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <raylib.h>

/* tokenizing things below */

enum type
{
	KEYWORD,
	COMMA,
	NEWLINE,
	ID,
	ADDRESS,
	POINTER,
	CONST
};

struct token
{
	char *lexeme;
	enum type type;
};

enum type assignType(char *str, char **keywords, int numOfKeywords)
{
	enum type t = ID;
	
	int state = 0, i;
	for(i = 0; str[i] != '\0'; i++)
	{
		switch(state)
		{
			case 0:
				t = ID;
				if(str[i] == '$') state = 1;
				if(isdigit(str[i]))
				{
					t = ADDRESS;
					state = 2;
				}
				if(str[i] == ',') return COMMA;
				if(str[i] == '\n') return NEWLINE;
				if(str[0] == 'p' && str[1] == '\0') return POINTER;
			break;
			
			case 1:
				t = ID;
				if(isdigit(str[i])) t = CONST;
			break;
			
			case 2:
				t = ID;
				if(isdigit(str[i])) t = ADDRESS;
			break;
		}
	}
	
	int j;
	for(j = 0; j<numOfKeywords; j++)
	{
		if(strcmp(str, keywords[j]) == 0) return KEYWORD;
	}
	
	return t;
}

struct token getNextToken(char *str, int *index, char **keywords, int numOfKeywords)
{
	struct token temp;
	temp.lexeme = calloc(30, sizeof(char));
	int i = 0;
	
	int state = 0;
	while(str[*index] != '\0')
	{
		switch(state)
		{
			case 0:
				state = 1;
				
				if(str[*index] == ' ' || str[*index] == '\t')
				{
					(*index)++;
					state = 0;
				}
				
				if(str[*index] == ',' || str[*index] == '\n')
				{
					temp.lexeme[0] = str[*index];
					(*index)++;
					state = 2;
				}
			break;
			
			case 1:
				if(str[*index] == ' ' || str[*index] == '\t' || str[*index] == ',' || str[*index] == '\n')
				{
					state = 2;
				} else
				{
					temp.lexeme[i] = str[*index];
					i++;
					(*index)++;
				}
			break;
			
			case 2:
				temp.type = assignType(temp.lexeme, keywords, numOfKeywords);
				return temp;
			break;
		}
	}
	
	if(strlen(temp.lexeme) == 0)
	{
		temp.lexeme = NULL;
	} else
	{
		temp.type = assignType(temp.lexeme, keywords, numOfKeywords);
	}
	
	return temp;
}

/* parsing related things below */

struct node
{
	struct token tk;
	struct node *left;
	struct node *right;
};

struct node * createNode(struct token token)
{
	struct node *temp = malloc(sizeof(struct node));
	
	temp->tk = token;
	temp->left = NULL;
	temp->right = NULL;
	
	return temp;
}

struct node * parse(char *str, char **keywords, int numOfKeywords, int *error)
{
	struct node *head = NULL, **p = &head;
	
	*error = 0;
	
	int index = 0;
	struct token temp = getNextToken(str, &index, keywords, numOfKeywords);	
	
	int state = 0;
	while(temp.lexeme != NULL)
	{
		switch(state)
		{
			case 0:
				if(temp.lexeme[0] == '\n')
				{
					state = 5;
				} else if(temp.lexeme[0] == ',')
				{
					state = 6;
				} else
				{
					*p = createNode(temp);
					p = &((*p)->left);
					state = 1;
				}
				temp = getNextToken(str, &index, keywords, numOfKeywords);
			break;
			
			case 1:
				if(temp.lexeme[0] == '\n')
				{
					state = 5;
				} else if(temp.lexeme[0] == ',')
				{
					state = 6;
				} else
				{
					*p = createNode(temp);
					p = &((*p)->left);
					state = 2;
				}
				temp = getNextToken(str, &index, keywords, numOfKeywords);
			break;
			
			case 2:
				state = 6;
				if(temp.lexeme[0] == ',') state = 3;
				if(temp.lexeme[0] == '\n') state = 5;
				temp = getNextToken(str, &index, keywords, numOfKeywords);
			break;
			
			case 3:
				if(temp.lexeme[0] == '\n')
				{
					state = 6;
				} else if(temp.lexeme[0] == ',')
				{
					state = 6;
				} else
				{
					*p = createNode(temp);
					p = &((*p)->left);
					state = 4;
				}
				temp = getNextToken(str, &index, keywords, numOfKeywords);
			break;
			
			case 4:
				state = 6;
				if(temp.lexeme[0] == ',') state = 3;
				if(temp.lexeme[0] == '\n') state = 5;
				temp = getNextToken(str, &index, keywords, numOfKeywords);
			break;
			
			case 5:
				for(p = &head; *p != NULL; p = &((*p)->right));
				state = 0;
			break;
			
			case 6: /* errors */
				*error = 1;
				return NULL;
			break;
		}
	}
	if(state == 3) state = 6;
	
	if(state == 6) 
	{
		*error = 1;
		return NULL;
	}
	
	return head;
}

/* execution related things below */

struct stack
{
	int *arr;
	int top;
};

void push(struct stack *st, int val)
{
	st->arr = realloc(st->arr, (st->top+2) * sizeof(int));
	st->arr[st->top+1] = val;
	st->top++;
}


void pop(struct stack *st)
{
	st->arr = realloc(st->arr, (st->top+1) * sizeof(int));
	st->top--;
}

struct dot
{
	int x;
	int y;
};

void drawDots(struct dot *dots, int numOfDots)
{
	int i;
	for(i = 0; i<numOfDots; i++)
	{
		if(dots[i].x < 32  || dots[i].y < 32) DrawRectangle(dots[i].x*8+640, dots[i].y*8+64, 8, 8, GREEN);
	}
}

void run(char **keywords, int numOfKeywords)
{
	struct node *head;
	struct node *rside, *lside = rside;
	int pc = 0; /* program counter */
	
	struct stack *subStack = malloc(sizeof(struct stack));
	subStack->arr = malloc(sizeof(int));
	subStack->top = -1;
	
	int testMem[1024] = {0};
	int pointerReg = 0;
	int flag = 0;
	
	int **args = malloc(sizeof(int *));
	int *temp = malloc(sizeof(int)); /* for constants */
	
	struct dot *dots = malloc(sizeof(struct dot));
	int numOfDots = 0;
	
	InitWindow(1024, 512, "Assemblage"); /*init*/
	
	Color frstbutton = BLACK;
	Color sndbutton = BLACK;
	Color Switch = GREEN;
	
	int state = 0; /* which window */
	int start = 0; /* begin executions */
	int focused = 0; /* focused on the text box? */
	
	char *str = calloc(1, sizeof(char));
	
	char *tempstr = calloc(2, sizeof(char));
	tempstr[0] = '|';
	
	int letterCount = 0;
	int numOfNl = 0;
	int scrollPos = 0;
	
	int error = 0;
	
	char *startbuttontext = " RUN";
	
	while(!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(WHITE);
	
		if(state == 0)
		{
			DrawRectangle(640, 64, 256, 256, BLACK); /* screen */
			drawDots(dots, numOfDots); /* dots */
			
			DrawRectangle(640, 332, 256, 32, Switch); /* start */
			Rectangle startbuttonrec = {640, 332, 256, 32};
			DrawText(startbuttontext, 736, 340, 20, BLACK);
			
			DrawRectangle(640, 376, 256, 72, LIGHTGRAY); /* errors */
			
			DrawRectangle(32, 64, 576, 384, LIGHTGRAY); /* text box */
			Rectangle textboxrec = {32, 64, 576, 384};
			
			scrollPos -= (int)GetMouseWheelMove();
			if(scrollPos < 0) scrollPos = 0;
			
			int countedLines = 0;
			int i;
			for(i = 0; str[i] != '\0' && countedLines < scrollPos; i++)
			{
				if(str[i] == '\n') countedLines++;
			}
			
			int j, currentChr = 0, twn3after = 0;
			for(j = i; str[j] != '\0' && twn3after <= 23; j++)
			{
				tempstr = realloc(tempstr, (currentChr+3) * sizeof(char));
				tempstr[currentChr] = str[j];
				tempstr[currentChr+1] = '|';
				tempstr[currentChr+2] = '\0';
				currentChr++;
				if(str[j] == '\n') twn3after++;
			}
			
			DrawText(tempstr, 32+8, 64+8, 20, BLACK);
			
			DrawRectangle(32, 40, 64, 16, frstbutton); /* editor button */
			Rectangle button1rec = {32, 40, 64, 16};
			DrawText("EDIT", 32+16, 40+2, 12, WHITE);
			
			DrawRectangle(104, 40, 64, 16, sndbutton); /* docs button */
			Rectangle button2rec = {104, 40, 64, 16};
			DrawText("DOCS", 104+16, 40+2, 12, WHITE);
			
			frstbutton = BLACK;
			sndbutton = BLACK;
			
			if(CheckCollisionPointRec(GetMousePosition(), button1rec))
			{
				frstbutton = RED;
				if(IsMouseButtonPressed(0)) state = 0;
			}
			
			if(CheckCollisionPointRec(GetMousePosition(), button2rec))
			{
				sndbutton = RED;
				if(IsMouseButtonPressed(0)) state = 1;
			}
			
			if(CheckCollisionPointRec(GetMousePosition(), startbuttonrec) && IsMouseButtonPressed(0) && start == 0)
			{
				Switch = RED;
				startbuttontext = "STOP";
				head = parse(str, keywords, numOfKeywords, &error);
				rside = head;
				lside = head;
				start = 1;
			} else if(CheckCollisionPointRec(GetMousePosition(), startbuttonrec) && IsMouseButtonPressed(0) && start == 1)
			{
				Switch = GREEN;
				startbuttontext = " RUN";
				start = 0;
				
				/* reset all variables */
				
				pc = 0;
				
				subStack->arr = realloc(subStack->arr, sizeof(int));
				subStack->top = -1;
				
				int k;
				for(k = 0; k<1024; k++) testMem[k] = 0;
				pointerReg = 0;
				flag = 0;
				
				dots = realloc(dots, sizeof(struct dot));
				numOfDots = 0;
			}
			
			if(head == NULL && error == 1)
			{
				DrawText("ERROR: malformed instruction", 640+16, 376+28, 16, BLACK);
			}
			
			if(CheckCollisionPointRec(GetMousePosition(), textboxrec))
			{
				SetMouseCursor(MOUSE_CURSOR_IBEAM);
			} else
			{
				SetMouseCursor(MOUSE_CURSOR_DEFAULT);
			}
			
			if(CheckCollisionPointRec(GetMousePosition(), textboxrec) && IsMouseButtonPressed(0))
			{
				focused = 1;	
			} else if(!CheckCollisionPointRec(GetMousePosition(), textboxrec) && IsMouseButtonPressed(0))
			{
				focused = 0;
			}
			
			if(focused == 1)
			{
				// Get char pressed (unicode character) on the queue
				int key = GetCharPressed();

				// Check if more characters have been pressed on the same frame
				while (key > 0)
				{
					str = realloc(str, (letterCount+2) * sizeof(char));
					str[letterCount] = (char)key;
					str[letterCount+1] = '\0'; // Add null terminator at the end of the string.
					letterCount++;
	
					key = GetCharPressed();  // Check next character in the queue
				}
				
				if(IsKeyPressed(KEY_ENTER))
				{
					str = realloc(str, (letterCount+2) * sizeof(char));
					str[letterCount] = '\n';
					str[letterCount+1] = '\0'; // Add null terminator at the end of the string.
					letterCount++;
					numOfNl++;
					if(numOfNl > scrollPos+23) scrollPos++;
	
					key = GetCharPressed();  // Check next character in the queue
				} else if(IsKeyPressed(KEY_BACKSPACE) && letterCount >= 1)
				{
					if(str[letterCount] == '\n') numOfNl--;
					
					str = realloc(str, letterCount+1 * sizeof(char));
					letterCount--;
					
					str[letterCount] = '\0';
				}
				
				if(IsKeyPressed(KEY_TAB))
				{
					str = realloc(str, (letterCount+2) * sizeof(char));
					str[letterCount] = '\t';
					str[letterCount+1] = '\0'; // Add null terminator at the end of the string.
					letterCount++;
	
					key = GetCharPressed();  // Check next character in the queue
				}
			}
		
			if(start == 1 && rside != NULL)
			{
				int numOfArgs = 0;
				int numOfConsts = 0;
				
				lside = rside;
				lside = lside->left;
				
				while(lside != NULL)
				{
					if(lside->tk.type == CONST || lside->tk.type == ADDRESS || lside->tk.type == POINTER)
					{
						numOfArgs++;
						args = realloc(args, numOfArgs * sizeof(int *));
						if(lside->tk.type == CONST)
						{
							numOfConsts++;
							temp = realloc(temp, numOfConsts * sizeof(int));
							temp[numOfConsts-1] = atoi(lside->tk.lexeme+1);
							args[numOfArgs-1] = temp+(numOfConsts-1);
						}
						if(lside->tk.type == ADDRESS) args[numOfArgs-1] = testMem+atoi(lside->tk.lexeme);
						if(lside->tk.type == POINTER) args[numOfArgs-1] = testMem+pointerReg;
					}
					
					lside = lside->left;
				}
				lside = rside;
				
				int jumpCond = 0;
				char id[30] = {0};
				
				if(rside->tk.type == KEYWORD)
				{
					int i;
					for(i = 0; strcmp(rside->tk.lexeme, keywords[i]) != 0; i++);
					
					switch(i)
					{
						case 0: /* ld */
							*args[0] = *args[1];
						break;
						
						case 1: /* add */
							*args[0] += *args[1];
						break;
						
						case 2: /* sub */
							*args[0] -= *args[1];
						break;
						
						case 3: /* ldp */
							pointerReg = *args[0];
						break;
						
						case 4: /* addp */
							pointerReg += *args[0];
						break;
						
						case 5: /* subp */
							pointerReg -= *args[0];
						break;
						
						case 6: /* jmp */
							jumpCond = 1;
							strcat(id, lside->left->tk.lexeme);
						break;
						
						case 7: /* je */
							if(flag == 1) jumpCond = 1;
							strcat(id, lside->left->tk.lexeme);
						break;
						
						case 8: /* jlt */
							if(flag == 2) jumpCond = 1;
							strcat(id, lside->left->tk.lexeme);
						break;
						
						case 9: /* jgt */
							if(flag == 3) jumpCond = 1;
							strcat(id, lside->left->tk.lexeme);
						break;
						
						case 10: /* jky */
							if(IsKeyDown(*args[0])) jumpCond = 1;
							strcat(id, lside->left->left->tk.lexeme);
						break;
						
						case 11: /* cmp */
							if(*args[0] == *args[1]) flag = 1;
							if(*args[0] < *args[1]) flag = 2;
							if(*args[0] > *args[1]) flag = 3;
						break;
						
						case 12: /* call */
							jumpCond = 1;
							strcat(id, lside->left->tk.lexeme);
							push(subStack, pc);
						break;
						
						case 13: /* ret */
							rside = head;
							for(pc = 0; pc<subStack->arr[subStack->top]; pc++) rside = rside->right;
							pop(subStack);
						break;
						
						case 14: /* dot */
							numOfDots++;
							dots = realloc(dots, sizeof(struct dot) * numOfDots);
							dots[numOfDots-1].x = *args[0];
							dots[numOfDots-1].y = *args[1];
						break;
						
						case 15: /* wait*/
							
						break;
						
						case 16: /* cls */
							numOfDots = 0;
							dots = realloc(dots, sizeof(struct dot));
						break;
					}
				} else if(rside->tk.type == ID)
				{
					
				} else
				{
					
				}
				
				if(jumpCond == 1)
				{
					pc = 0;
					for(rside = head; strcmp(rside->tk.lexeme, id) != 0; rside = rside->right) pc++;
				}
				
				rside = rside->right;
				pc++;
			}
		} else if(state == 1)
		{
			DrawRectangle(32, 40, 64, 16, frstbutton); /* editor button */
			Rectangle button1rec = {32, 40, 64, 16};
			DrawText("EDIT", 32+16, 40+2, 12, WHITE);
			
			DrawRectangle(104, 40, 64, 16, sndbutton); /* editor button */
			Rectangle button2rec = {104, 40, 64, 16};
			DrawText("DOCS", 104+16, 40+2, 12, WHITE);
			
			frstbutton = BLACK;
			sndbutton = BLACK;
			
			DrawText("ld dest, var\nadd dest, var\nsub dest, var\nldp var\naddp var\nsubp var\njmp label\nje label\njlt label\njgt label\njky var, label\ncmp var1, var2\ncall label\nret\ndot var1, var2\nwait var (N/A)\ncls\n", 32, 64, 20, BLACK);
			DrawText("dest = var\ndest += var\ndest -= var\np = var\np += var\np -= var\npc = label\nif(flag == EQUALS) pc = label\nif(flag == LESSTHAN) pc = label\nif(flag == GREATERTHAN) pc = label\nif(KEYPRESS(var)) pc = label\nsetflag(var1, var2)\npush pc, pc = label\npop pc, pc = top\nplace dot at (x, y)\nwait time in milliseconds\nclear screen\n", 256, 64, 20, BLACK);
			DrawText("There is 1024 bytes of memory, below are the ways of addressing it:\ndirect address (0-1023): specific address to perform the operation on\npointer (p): a register which points to a value in memory to perform the operation on\nconstants ($0-$255): a constant value used by the instruction on a memory location\n\nThere are some registers the interpreter uses for execution, they are:\nprogram counter (pc): the position of the current instruction, used for jumps\nflag register: flags are set with the cmp operation, if the corresponding flag is set\n                  when a specific jump is called, the jump executes\n", 32, 352, 20, BLACK);
			
			if(CheckCollisionPointRec(GetMousePosition(), button1rec))
			{
				frstbutton = RED;
				if(IsMouseButtonPressed(0)) state = 0;
			}
			
			if(CheckCollisionPointRec(GetMousePosition(), button2rec))
			{
				sndbutton = RED;
				if(IsMouseButtonPressed(0)) state = 1;
			}
		}
		EndDrawing();
	}
	CloseWindow();
}

int main(void)
{	
	char *keywords[17] = {"ld", "add", "sub", "ldp", "addp", "subp", "jmp", "je",
						 "jlt", "jgt", "jky", "cmp", "call", "ret", "dot", "wait",
						 "cls"};
	
	run(keywords, 17);
	
	return 0;
}