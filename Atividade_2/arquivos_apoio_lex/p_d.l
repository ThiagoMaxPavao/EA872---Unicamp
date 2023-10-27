%{
int lines=0,characs=0,words=0,pages=1;
%}
 
%START                  Palavra
NovaLinha               [\n]
Espaco                  [\t ]
NovaPagina              [\f]
   
%%
{Espaco}            { BEGIN 0; characs++; }
{NovaLinha}         { BEGIN 0; characs++; lines++; }
{NovaPagina}        { BEGIN 0; characs++; pages++; }
<Palavra>.	    { characs++; }
.                   { BEGIN Palavra; characs++; words++; }
%%

main()
{
  while (yylex());
  printf("\nResultados: \n%d pagina(s)\n%d linha(s)\n%d palavra(s)\n%d caracter(es)\n",
            pages,lines,words,characs);
}
