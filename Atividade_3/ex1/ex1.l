%{
    int c, p, l;
%}
%%
\n		    { l++; c++; }
[^ \t\n]+	{ p++, c += yyleng; }
.		    { c++; }
%%
int main(void)	{
    yylex();
    printf("c=%d\tp=%d\tl=%d\n",c,p,l);
    return 0;
}
