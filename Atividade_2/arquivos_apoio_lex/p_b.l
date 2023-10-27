%{
	int h;
%}
AM	[ ](00|01|02|03|04|05|06|07|08|09|10|11)[:]
PM	[ ](12|13|14|15|16|17|18|19|20|21|22|23)[:]
%%
{PM}	{sscanf(yytext," %d", &h); 
         if (h == 12) printf(" PM 12:"); 
         else printf(" PM %02d:", h-12);}
{AM}	{sscanf(yytext," %d", &h); 
         if (h==0) printf(" AM 12:");
         else printf(" AM %02d:", h);}
.	ECHO;
