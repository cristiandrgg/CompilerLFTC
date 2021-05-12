#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX 10001
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

typedef enum
{
	END = 0, ID, CT_INT, CT_REAL, CT_CHAR, CT_STRING,
	ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
    COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
    BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE
} ids;

int getNextToken();
int line = 0;
char *pCrtCh;

void err(const char *fmt, ...)
	{
		va_list va;
		va_start(va,fmt);
		fprintf(stderr,"Error: ");
		vfprintf(stderr,fmt,va);
		fputc('\n',stderr);
		va_end(va);
		exit(-1);
	}

typedef struct _Token{
	int code;
	union {
		char *text;                 // used for ID, CT_STRING (dynamic allocation)
		long int i;                 // used for CT_INT, CT_CHAR
		double r;                   // used for CT_REAL
		};
	int line;                           // line from entry file
	struct _Token *next;               // next element in chain
	} Token;

Token *lastToken = NULL, *firstToken = NULL;

Token *addTk(int code)
{
	Token *tk;
	SAFEALLOC(tk,Token)
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if(lastToken)
	{
	lastToken->next = tk;
	}else
	{
        firstToken = tk;
	}
	lastToken = tk;
	return tk;
}

char *getTokenCode(int code)
{
	switch(code)
	{
		case END : return "END"; break;
		case ID : return "ID"; break;
		case CT_INT : return "CT_INT"; break;
		case CT_REAL : return "CT_REAL"; break;
		case CT_STRING : return "CT_STRING"; break;
		case CT_CHAR : return "CT_CHAR"; break;
		case ADD : return "ADD"; break;
		case SUB : return "SUB"; break;
		case DIV : return "DIV"; break;
		case MUL : return "MUL"; break;
		case DOT : return "DOT"; break;
		case AND : return "AND"; break;
		case OR : return "OR"; break;
		case NOT : return "NOT"; break;
		case ASSIGN : return "ASSIGN"; break;
		case EQUAL : return "EQUAL"; break;
		case NOTEQ : return "NOTEQ"; break;
		case LESS : return "LESS"; break;
		case LESSEQ : return "LESSEQ"; break;
		case GREATER : return "GREATER"; break;
		case GREATEREQ : return "GREATEREQ"; break;
		case COMMA : return "COMMA"; break;
		case SEMICOLON : return "SEMICOLON"; break;
		case LPAR : return "LPAR"; break;
		case RPAR : return "RPAR"; break;
		case LBRACKET : return "LBRACKET"; break;
		case RBRACKET : return "RBRACKET"; break;
		case LACC : return "LACC"; break;
		case RACC : return "RACC"; break;
		case BREAK : return "BREAK"; break;
		case CHAR : return "CHAR"; break;
		case DOUBLE : return "DOUBLE"; break;
		case ELSE : return "ELSE"; break;
		case FOR : return "FOR"; break;
		case IF : return "IF"; break;
		case INT : return "INT"; break;
		case RETURN : return "RETURN"; break;
		case STRUCT : return "STRUCT"; break;
		case VOID : return "VOID"; break;
		case WHILE : return "WHILE"; break;

		default : return "> ! Invalid code value! <"; break;

	}
}

void displayTokens()
{
	Token *currentToken;
	for(currentToken = firstToken; currentToken != NULL; currentToken = currentToken->next)
	{
		printf(" %d %s ", currentToken->line, getTokenCode((currentToken->code)));
		switch(currentToken->code)
		{
			case ID: printf(" :  %s", currentToken->text);
			         break;
			case CT_INT: printf(" :  %ld", currentToken->i);
				     break;
			case CT_CHAR: printf(" :  %c", currentToken->i);
				      break;
			case CT_REAL: printf(" :  %g", currentToken->r);
				      break;
			case CT_STRING:	printf(" :  %s", currentToken->text);
					break;

			default : break;
		}
		printf("\n");
	}
}

void tkerr(const Token *tk,const char *fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	fprintf(stderr,"> ! Error at line: %d ! <",tk->line);
	vfprintf(stderr,fmt,va);
	fputc('\n',stderr);
	va_end(va);
	exit(-1);
}

char *createString(char *startCh, char *endCh)
{
    char *result = (char*)malloc((endCh-startCh)*sizeof(char));
    int index = 0;
    char aux;

    while((endCh-startCh) > 0)
    {
        aux = startCh[1];
		if((*startCh) == '\\')
		{
			if(aux == 'a') result[index] = '\a';
			if(aux == 'b') result[index] = '\b';
			if(aux == 'f') result[index] = '\f';
			if(aux == 'n') result[index] = '\n';
			if(aux == 'r') result[index] = '\r';
			if(aux == 't') result[index] = '\t';
			if(aux == 'v') result[index] = '\v';
			if(aux == 'r') result[index] = '\r';
			if(aux == '\'') result[index] = '\'';
			if(aux == '?') result[index] = '\?';
			if(aux == '"') result[index] = '\"';
			if(aux == '\\') result[index] = '\\';

			if(!isalpha(aux)) result[index] = aux;

			startCh++;
		}
		else result[index] = *startCh;
        index++;
        startCh++;
    }
    result[index] = '\0';

    return result;
}

int getNextToken()
{
	char ch, *ptrStart, prevChar;
	int state = 0;
	//int lenChar;
	int n;
	int isHexFlag = 0;
	int isOctFlag = 0;

	Token *tk;

    //https://sites.google.com/site/razvanaciu/atomc---reguli-lexicale

	for(;;) // bucla infinita //sau while(1)
	{
		ch = *pCrtCh;

		switch(state)
		{
            case 0:                             // testare tranzitii posibile din starea 0
                if(isalpha(ch) || ch == '_')    // ID --> sa inceapa cu a-z A-Z sau _
				{
					ptrStart = pCrtCh;          // memoreaza inceputul ID-ului
					pCrtCh++;                   // consuma caracterul
					state = 1;                  // trece la noua stare //si din 1 vom ajunge in case 2
				}
                else if(ch == ',')              //DELIMITATORII INCEP AICI
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 3;
				}
                else if(ch == ';')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 4;
				}
                else if(ch == '(')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 5;
				}
                else if(ch == ')')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 6;
				}
                else if(ch == '[')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 7;
				}
                else if(ch == ']')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 8;
				}
                else if(ch == '{')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 9;
				}
                else if(ch == '}')                  //DELIMITATORII SE TERMINA AICI
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 10;
				}
                else if(ch == '+')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 11;
				}
                else if(ch == '-')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 12;
				}
                else if(ch == '*')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 13;
				}
                else if(ch == '/')                      //COMBINAT CU COMENTARIILE !!!
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 14;
				}
                else if(ch == '.')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 19;
				}
                else if(ch == '&')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 20;
				}
                else if(ch == '|')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 22;
				}
                else if(ch == '!')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 24;
				}
                else if(ch == '=')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 27;
				}
                else if(ch == '<')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 30;
				}
                else if(ch == '>')
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 33;
				}
                else if(ch == ' ' || ch == '\r' || ch == '\t')  //SPATIU
                    pCrtCh++;
                else if(ch == '\n')                             //NEW LINE (ENTER)
				{
					pCrtCh++;
					line++;
				}
                else if(ch == '\0'){							//END
					addTk (END);
					return END;
				}
                else if(ch == '0')                              //pt CT INT & CT REAL
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 44;
					isOctFlag = 1;
				}
                else if(ch >= '1' && ch <= '9')                 //pt CT INT & CT REAL
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 36;
				}
                else if(ch == '\'')                             //e doar > ' < , NU > \' <  // pt CT CHAR
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 48;
				}
                else if(ch == '\"')                             //e doar > " < , NU > \" <  // pt CT STRING
				{
					ptrStart = pCrtCh;
					pCrtCh++;
					state = 52;
				}
                else                                            //in caz ca nu e niciun caracter, eroare
                {
					perror("\n !! > INVALID CHARACTER < !! \n");
					exit(-1);
				}
				break;                                          //gata Case0

            case 1:
                if(ch == '_' || isalnum(ch)) pCrtCh++;          //continuarea de la ID a-z A-Z 0-9 _
					else state = 2;
				break;

            case 2:                                             //ID si CUVINTE CHEIE
                n = pCrtCh- ptrStart;                           //lungime cuvant cheie
				if(n == 5 && (!memcmp(ptrStart,"break",5))) tk = addTk(BREAK);
				else if (n == 5 && (!memcmp(ptrStart,"while",5))) tk = addTk(WHILE);
				else if (n == 4 && (!memcmp(ptrStart,"char",4))) tk = addTk(CHAR);
				else if (n == 6 && (!memcmp(ptrStart,"double",n))) tk = addTk(DOUBLE);
				else if (n == 4 && (!memcmp(ptrStart,"else",n))) tk = addTk(ELSE);
				else if (n == 3 && (!memcmp(ptrStart,"for",n))) tk = addTk(FOR);
				else if (n == 2 && (!memcmp(ptrStart,"if",n))) tk = addTk(IF);
				else if (n == 3 && (!memcmp(ptrStart,"int",n))) tk = addTk(INT);
				else if (n == 6 && (!memcmp(ptrStart,"return",n))) tk = addTk(RETURN);
				else if (n == 6 && (!memcmp(ptrStart,"struct",n))) tk = addTk(STRUCT);
				else if (n == 4 && (!memcmp(ptrStart,"void",n))) tk = addTk(VOID);
				else if (n == 5 && (!memcmp(ptrStart,"while",n))) tk = addTk(WHILE);
				else
				{
					tk = addTk(ID);
					tk->text = (char*)malloc(pCrtCh - ptrStart + 1);
					strcpy(tk->text,createString(ptrStart, pCrtCh));
				}
				return ID;                                      //ID final si corect
				break;

            case 3: addTk(COMMA);                               //DELIMITATORII INCEP AICI
				return COMMA;
				break;

            case 4: addTk(SEMICOLON);
				return SEMICOLON;
				break;

            case 5: addTk(LPAR);
				return LPAR;
				break;

            case 6: addTk(RPAR);
				return RPAR;
				break;

            case 7: addTk(LBRACKET);
				return LBRACKET;
				break;

            case 8: addTk(RBRACKET);
				return RBRACKET;
				break;

            case 9: addTk(LACC);
				return LACC;
				break;

            case 10: addTk(RACC);                                //DELIMITATORII SE TERMINA AICI
				return RACC;
				break;

            case 11: addTk(ADD);                                //OPERATORII INCEP AICI
				return ADD;
				break;

            case 12: addTk(SUB);
				return SUB;
				break;

            case 13: addTk(MUL);
				return MUL;
				break;

            //CASE cu / (DIV si COMMENTS) lasam pe mai incolo

            case 19: addTk(DOT);
				return DOT;
				break;

            case 20:
                if(ch == '&')                                   //ne trb doua & pt AND
			    {
					pCrtCh++;
					state = 21;
				}
				else                                            //daca e doar un & nu e ok
				{
					perror("\n > ! INVALID CHARACTER ! < \n");
					exit(-1);
				}
				break;

            case 21: addTk(AND);
					return AND;
					break;

            case 22:
                if(ch == '|')                          //fix acelasi principiu ca la AND
				{
					pCrtCh++;
					state = 23;
				}
				else
				{
					perror("\n > ! INVALID CHARACTER ! < \n");
					exit(-1);
				}
				break;

            case 23: addTk(AND);
					return AND;
					break;

            case 24:                                            //NOT si NOTEQ cu cele doua ramuri 25-26
                if(ch == '=')
				{
					pCrtCh++;
					state = 26;
				}
				else
				{
					pCrtCh++;
					state = 25;
				}
				break;

            case 25: addTk(NOT);
					return NOT;
					break;

			case 26: addTk(NOTEQ);
					return NOTEQ;
					break;

			case 27:                                            //EQUAL si ASSIGN cu cele doua ramuri 28-29
                if(ch == '=')
				{
					pCrtCh++;
					state = 28;
				}
				else
				{
					pCrtCh++;
					state = 29;
				}
				break;

            case 28: addTk(EQUAL);
					return EQUAL;
					break;

			case 29: addTk(ASSIGN);
					return ASSIGN;
					break;

			case 30: if(ch == '=')								//LESS si LESSEQ cu cele doua ramuri 31-32
					{
						pCrtCh++;
						state = 31;
					}
					else state = 32;
					break;

			case 31: addTk(LESSEQ);
					return LESSEQ;
					break;

			case 32: addTk(LESS);
					return LESS;
					break;

			case 33: if(ch == '=')								//GREATER si GREATEREQ cu cele doua ramuri 34-35
					{
						pCrtCh++;
						state = 34;
					}
				else state = 35;
					break;

			case 34: addTk(GREATEREQ);
					return GREATEREQ;
					break;

			case 35: addTk(GREATER);
					return GREATER;
					break;

			//fac aici case14 cu / (DIV si COMENTARII)

			case 14: if(ch == '*')
					{
						line++;
						pCrtCh++;
						state = 16;
					}
					else if(ch == '/')
					{
						line++;
						pCrtCh++;
						state = 18;
					}
					else state = 15;										//state 15 e DIV
					break;

			case 15: addTk(DIV);
					return DIV;
					break;

			case 18: if(ch != '\n' && ch != '\r' && ch != '\0')
						pCrtCh++;
					else												//ajungem inapoi in starea 0 de aici
					{
						pCrtCh++;
						state = 0;
					}
					break;

			case 16: if(ch != '*')
						pCrtCh++;
					else if(ch == '*')
					{
						pCrtCh++;
						state = 17;
					}
					else
					{
						perror("\n > ! INVALID CHARACTER ! < \n");
						exit(-1);
					}
					break;

			case 17: if(ch == '*')
						pCrtCh++;
					else if(ch != '/')
					{
						pCrtCh++;
						state = 16;
					}
					else if(ch == '/')									//ajungem inapoi in starea 0 de aici
					{
						pCrtCh++;
						state = 0;
						line++;
					}
					else
					{
						perror("\n > ! INVALID CHARACTER ! < \n");
						exit(-1);
					}
					break;

																			//CT de aici !!!!!!!!!
			case 36: if(ch >= '0' && ch <= '9') pCrtCh++;					//deasupra de 0 pe diagrama (pagina 2)
					else if(ch == '.')
					{
						pCrtCh++;
						state = 388;
					}
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 41;
					}
					else state = 37;										// 37 = CT_INT
					break;

			case 38: if(isdigit(ch))
					{
						pCrtCh++;
						state = 39;
					}
					else
					{
						perror("\n > !INVALID CHARACTER! <\n");
						exit(-1);
					}
					break;

			case 388:

					state = 38;

					break;

			case 39: if(isdigit(ch))
						pCrtCh++;
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 41;
					}
					else state = 40;										// 40 = CT_REAL
					break;

			case 44: if(ch >= '0' && ch <= '7')
					{
						pCrtCh++;
						state = 45;
					}
					else if(ch == '.')
					{
                        pCrtCh++;
						state = 38;
					}
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 41;
					}
					else if(ch == 'x')
					{
						isHexFlag = 1;										//inseamna ca e Hex
						pCrtCh++;
						state = 46;
					}
					else if(ch == '8' || ch == '9')
					{
						pCrtCh++;
						state = 388;
					}
					else state = 37;										// 37 = CT_INT
					break;

			case 41: if(isdigit(ch))
					{
						pCrtCh++;
						state = 43;
					}
					else if(ch == '-' || ch == '+')
					{
						pCrtCh++;
						state = 42;
					}
					else
					{
						perror("\n > ! INVALID CHARACTER ! < \n");
						exit(-1);
					}
					break;

			case 45: if(ch >= '0' && ch <= '7') pCrtCh++;
					else if(ch == '8' || ch == '9')
					{
						pCrtCh++;
						state = 388;
					}
					else if(ch == '.')
					{
                        pCrtCh++;
						state = 38;
					}
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 41;
					}
					else
					{
						pCrtCh++;
						state = 37;											// 37 = CT_INT
					}
					break;

			case 43: if(isdigit(ch)) pCrtCh++;
					else state = 40;										// 40 = CT_REAL
					break;

			case 42: if(isdigit(ch))
					{
						pCrtCh++;
						state = 43;
					}
					else
					{
						perror("\n > ! INVALID CHARACTER AFTER +/- ! <\n");
						exit(-1);
					}
					break;

			case 46: if((isdigit(ch)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
					{
						pCrtCh++;
						state = 47;
					}
					else
					{
						perror("\n ! > INVALID CHARACTER IN BASE 16! CH MUST BE BETWEEN 0-F / 0-f ! < \n");
						exit(-1);
					}
					break;

			case 47: if((isdigit(ch)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) pCrtCh++;
					else state = 37;											// 37 = CT_INT
					break;

			case 48: if(ch == '\\')
					{
						pCrtCh++;
						state = 50;
					}
					else if(ch == '\'')
					{
					 	perror("\n ! > YOU CAN'T HAVE AN EMPTY CHARACTER ! < \n");
						exit(-1);
					}
					else
					{
						prevChar = pCrtCh[0];
						pCrtCh++;
						state = 51;
					}
					break;

			case 50: switch(ch){
							case 'a': prevChar = '\a'; break;
							case 'b': prevChar = '\b'; break;
							case 'f': prevChar = '\f'; break;
							case 'n': prevChar = '\\'; break;
							case 'r': prevChar = '\r'; break;
							case 't': prevChar = '\t'; break;
							case 'v': prevChar = '\v'; break;
							case '\\': prevChar = '\\'; break;
							case '"': prevChar = '\"'; break;
							case '\'': prevChar = '\''; break;
							case '?': prevChar = '\?'; break;
							case '0': prevChar = '\0'; break;
							default:prevChar = ch;
					 }

					pCrtCh++;
					state = 51;
					break;

			case 51: if(ch == '\'')
					{
						pCrtCh++;
						state = 49;														//49 = CT_CHAR
					}
					else
					{
						perror("\nToo many characters!\n");
						exit(-1);
					}
					break;

			case 52: if(ch == '\\')
					{
						pCrtCh++;
						state = 53;
					}
					else if(ch == '\"') state = 55;										//55 = CT_STRING
					else pCrtCh++;
					break;

			case 53: if(ch == 'a' || ch == 'n' || ch == 'b' || ch == 'f' || ch == 'r' || ch == 't' ||
					    ch == 'v' || ch == '\'' || ch == '\"' || ch == '?' || ch == '\\' || ch == '0')
					{
						pCrtCh++;
						state = 52;
					}
					else if(ch == '\"') state = 55;
					else
					{
						perror("\n > ! INVALID CHARACTER ! <\n");
						exit(-1);
					}
					break;

			case 37: tk = addTk(CT_INT);
					if(isHexFlag == 1) tk->i=(int)strtol(createString(ptrStart+2, pCrtCh) , NULL, 16);	//converts to a long int according to the base value
					else if(isOctFlag == 1) tk->i=(int)strtol(createString(ptrStart+1,pCrtCh), NULL, 8);
					else tk->i = atoi(createString(ptrStart, pCrtCh));
					return CT_INT;
					break;

			case 40: tk = addTk(CT_REAL);
					tk->r = atof(createString(ptrStart, pCrtCh));
					return CT_REAL;
					break;

			case 49: tk = addTk(CT_CHAR);
					tk->i = prevChar;
					return CT_CHAR;
					break;

			case 55: tk = addTk(CT_STRING);
					tk->text = (char*)malloc(pCrtCh - ptrStart + 1);
					strcpy(tk->text,createString(ptrStart+1, pCrtCh));
					pCrtCh++;
					return CT_STRING;
					break;

			/*case 56: addTk(END);
					return END;
					break;*/

        }

    }

}

int main(int argc, char **argv)
{
	char *text = (char*)malloc(MAX);
	int n;


	int fd = open(argv[1],O_RDONLY);				// primul argument orice test file nume.c

	//int fd = open("8.c", O_RDONLY);		// sau asa dat direct argumentul

	if( (n = read(fd, text, MAX-1)) == -1)
	{
		perror("\n > ! Could not read file ! < \n");
		exit(-2);
	}

   	text[n] = '\0';
	printf("%s\n\n\n", text);

	pCrtCh = text;

	while(getNextToken() != END);

	close(fd);
	displayTokens();

	return 0;
}
