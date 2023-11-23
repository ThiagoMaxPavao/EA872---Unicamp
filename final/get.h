/*
Configura a variável serverPagesPath do módulo utilizando o caminho do programa,
este pode ser obtido através de argv[0].
*/
void configureServerPagesPath(char *programPath);

/*
Abre o arquivo de erro, de acordo com o status fornecido.
Acessa o arquivo error_{status}.html e abre, inserindo o valor no ponteiro fd.
Também copia o nome para filename e retorna o status.
*/
int openAndReturnError(int status, int* fd, char* filename);

/*
Acessa webspace/resource no sistema de arquivos e abre o arquivo
o file descriptor é armazenado no endereco fd.
Retorna o código de erro, se tiver ocorrido, ou 200 se tudo ocorreu bem. (status)
Também procura um arquivo index.html ou welcome.html se webspace/resource for um diretório.
Retorna o file descriptor desses arquivos nesses casos, se forem encontrados.
Se ocorrer erro, chama openAndReturnError e retorna o status. Caso exista o arquivo
error_{status}.html, ele é aberto e retornado.
*/
int get(char* webspace, char* resource, int* fd, char* filename, char* authBase64);
