%{
  int valor;
%}

OCTAL          [0-7]+O

%%

{OCTAL}        { sscanf(yytext,"%o",&valor); printf("%d",valor); }
.              ECHO;

%%

