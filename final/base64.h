
/*
Codifica dados em base64, recebe os dados e o tamanho da entrada.
Retorna o tamanho da saída, em output_length
e retorna um ponteiro para uma string com os dados codificados.
Esta string é alocada dinamicamente e deve ser liberada (free).
*/
char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

/*
Decodifica dados em base64, recebe os dados codificados e o tamanho da entrada.
Retorna o tamanho da saída, em output_length
e retorna um ponteiro para uma string com os dados decodificados.
Esta string é alocada dinamicamente e deve ser liberada (free).
*/
unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);

/*
Funcão de construção de tabela utilizada no processo de (de)codificação.
Deve ser chamada antes de base64_encode e base64_decode.
Aloca memória dinamicamente, então deve ser liberada com base64_cleanup.
*/
void build_decoding_table();

/*
Libera a memória da estrutura alocada na inicialização.
*/
void base64_cleanup();