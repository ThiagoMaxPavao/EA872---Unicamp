%START NaoQuebrarLinha
%%
^#.*$ {
    if(yytext[1] == '!') ECHO;
    else BEGIN NaoQuebrarLinha;
}
[^\n$]#.*$ {
    if(yytext[2] == '!') ECHO;
    else printf("%c", yytext[0]);
}
<NaoQuebrarLinha>\n     BEGIN 0;
.                       ECHO;
