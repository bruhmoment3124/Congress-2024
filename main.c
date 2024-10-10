#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
	enum type t;
	
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

struct node * parse(char *str, char **keywords, int numOfKeywords)
{
	struct node *head = NULL, **p = &head;
	
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
			
			case 6:
				fprintf(stderr, "ERROR: malformed instruction\n");
				exit(EXIT_FAILURE);
			break;
		}
	}
	if(state == 3) state = 6;
	
	if(state == 6)
	{
		fprintf(stderr, "ERROR: malformed instruction\n");
		exit(EXIT_FAILURE);
	}
	
	return head;
}

/* execution related things below */

void run(struct node *head, char **keywords, int numOfKeywords)
{
	struct node *rside = head, *lside = rside;
	
	int testMem[1024] = {0};
	int pointerReg = 0;
	int flag = 0;
	
	int **args = malloc(sizeof(int *));
	int temp; /* for constants */
	
	while(rside != NULL)
	{
		int numOfArgs = 0;
		
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
					temp = atoi(lside->tk.lexeme+1);
					args[numOfArgs-1] = &temp;
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
				
				break;
				
				case 11: /* cmp */
					if(*args[0] == *args[1]) flag = 1;
					if(*args[0] < *args[1]) flag = 2;
					if(*args[0] > *args[1]) flag = 3;
				break;
				
				case 12: /* call */
				
				break;
				
				case 13: /* ret */
				
				break;
				
				case 14: /* dot */
				
				break;
				
				case 15: /* wait*/
				
				break;
				
				case 16: /* cls */
				
				break;
			}
		} else if(rside->tk.type == ID)
		{
			
		} else
		{
			
		}
		
		if(jumpCond == 1)
		{
			for(rside = head; strcmp(rside->tk.lexeme, id) != 0; rside = rside->right);
		}
		
		//printf("%d, %d\n", pointerReg, *args[0]);
		rside = rside->right;
	}
	
	printf("%d\n", testMem[10]);
}

int main(void)
{
	char *str = "add 0, $0\n cmp 0, $0\n je test1\n jmp test2\n test1\n add 10, $5\n jmp after\n test2\n add 10, $10\n after";
	
	/*
	add 10, $3
	jmp id
	add 10, $2
	id
	add 10, $3
	*/
	
	char *keywords[17] = {"ld", "add", "sub", "ldp", "addp", "subp", "jmp", "je",
						 "jlt", "jgt", "jky", "cmp", "call", "ret", "dot", "wait",
						 "cls"};
	
	struct node *head = parse(str, keywords, 17);
	run(head, keywords, 17);
	
	getchar();
	
	return 0;
}