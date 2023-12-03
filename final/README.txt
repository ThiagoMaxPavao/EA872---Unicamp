Este projeto se trata de um servidor web que projeta uma pasta do computador para a internet.

Para fazer a compilação, execute os comandos no script compila.sh ou utilize o Makefile com 'make'.

Uso: ./server <Web Space> <Porta> <Arquivo de Log> <URL de troca de senha> <Max threads> [charset (tipo de codificacao)]

Exemplo: ./server ~/webspace 1234 registro.txt change_password.html 10 utf-8

O exemplo:

- Projeta para a web a pasta na home do usuário: webspace
- Abre o servidor na porta 1234
- Salva todos os logs em um arquivo chamado registro.txt
- Envia o formulário de troca de senha na URL terminada em change_password.html para qualquer subdiretório protegido
- Abre no máximo 10 threads para atender clientes
- Envia como codificação para arquivos do tipo texto charset=utf-8, no cabeçalho Content-Type
