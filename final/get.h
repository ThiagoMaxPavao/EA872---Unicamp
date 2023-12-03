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
Retorna o código de erro, se tiver ocorrido, ou 200 se tudo ocorreu bem.
Também procura um arquivo index.html ou welcome.html se webspace/resource for um diretório.
Retorna o file descriptor desses arquivos nesses casos no parâmetro fd, se forem encontrados.
Se ocorrer erro, chama openAndReturnError e retorna o status. Caso exista o arquivo
error_{status}.html, ele é aberto e retornado.
Verifica se o recurso é protegido por algum .htaccess, no mesmo diretório ou em algum
diretório pai. Neste caso procura o arquivo requisitado apenas se o usuário enviou 
autenticação correta.
Em filename retorna o nome do arquivo aberto.
*/
int get(char* webspace, char* resource, int* fd, char* filename, char* authBase64);

/*
Processa a requisição de POST, retorna o status da resposta, e possivelmente uma página em fd.
Funciona seguindo os passos a seguir.
 - Verifica se o post está sendo feito em um URL que termine em changePasswordFilename (enviado 
   como parametro para o programa), se não estiver retorna status 405, pois o POST foi feito em 
   um URL inválido. Se estiver continua.
 - Verifica que o diretório possui restrição de acesso com algum .htaccess,
   verifica diretórios mais acima até alcançar a raíz, continuando se achar qualquer htacess.
   Se não encontrar nenhum, então o diretório está em uma sub árvore desprotegida e o servidor
   retorna erro, o status é 400 Bad Request
 - Separa os valores de cada campo do formulário sem variáveis separadas, caso ocorra algum erro
   (por exemplo, um campo faltando) retorna 400 Bad Request
 - Verifica se a nova senha e a senha de confirmação são iguais.
   Retorna 400 Bad Request e uma página informando que que as senhas não podem ser diferentes
   A página também contém um link de volta para change_password.
 - Utiliza o usuário e senha atual para verificar se o usuário tem permissão de acesso no
   diretório em questão. Retorna 403 Forbidden e uma página informando que o usuário ou a senha
   estão incorretos. A página também contém um link de volta para change_password.
 - Só após tudo isso a senha é efetivamente alterada, então é retornada a página de sucesso
   na alteração da senha e o status é 200.
*/
int processPost(char* webspace, char* resource, int* fd, char* filename, char *requestContent);
